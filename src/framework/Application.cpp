#ifdef SGK_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <sgkit/framework/Application.h>
#include <sgkit/framework/Timing.h>

#include <sgkit/core/Window.h>
#include <sgkit/core/Input.h>
#include <sgkit/core/KeyCodes.h>
#include <sgkit/graphics/Renderer.h>
#include <sgkit/scene/Scene.h>

#include <cstdio>
#include <memory>

using namespace sgkit;

static void AttachConsole()
{
#if defined(SGK_PLATFORM_WINDOWS) && defined(_DEBUG)
    if (AllocConsole())
    {
        FILE* dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        freopen_s(&dummy, "CONOUT$", "w", stderr);
        freopen_s(&dummy, "CONIN$",  "r", stdin);
    }
#endif
}

static void DetachConsole()
{
#if defined(SGK_PLATFORM_WINDOWS) && defined(_DEBUG)
    fclose(stdout);
    fclose(stderr);
    fclose(stdin);
    FreeConsole();
#endif
}

static void Fatal(const char* msg)
{
    std::fprintf(stderr, "[[ SGKit FATAL  ]]: %s\n", msg);

#ifdef SGK_PLATFORM_WINDOWS
    int wlen = MultiByteToWideChar(CP_UTF8, 0, msg, -1, nullptr, 0);
    int tlen = MultiByteToWideChar(CP_UTF8, 0, "SGKit Fatal Error", -1, nullptr, 0);
    if (wlen > 0 && tlen > 0)
    {
        std::wstring wmsg(wlen - 1, L'\0');
        std::wstring wtitle(tlen - 1, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, msg, -1, wmsg.data(), wlen);
        MultiByteToWideChar(CP_UTF8, 0, "SGKit Fatal Error", -1, wtitle.data(), tlen);
        MessageBoxW(nullptr, wmsg.c_str(), wtitle.c_str(), MB_OK | MB_ICONERROR);
    }
#endif
}

static int Run(HINSTANCE hInst, const ApplicationConfig& config)
{
    AttachConsole();

    // -- Init modules in dependency order ---------------------------------

    core::WindowDesc wd;
    wd.title                = config.title;
    wd.width                = config.width;
    wd.height               = config.height;
    wd.resizable            = config.resizable;
    wd.vsync                = config.vsync;
    wd.fullscreenBolderless = config.fullscreenBolderless;
    wd.fullscreen           = config.fullscreen;
    wd.cursorVisible        = config.cursorVisible;
    wd.glMajorVersion       = config.glMajor;
    wd.glMinorVersion       = config.glMinor;

    if (!core::Window::Create(hInst, wd))
    {
        Fatal("Failed to create window. Please check you device.");
        core::Window::Destroy();
        DetachConsole();
        return 1;
    }
    core::Window& window = core::Window::instance();

    if (!core::Input::Create(window.GetNativeHandle()))
    {
        Fatal("Failed to initialize input device.");
        core::Input::Destroy();
        core::Window::Destroy();
        DetachConsole();
        return 1;
    }
    core::Input& input = core::Input::instance();

    graphics::Renderer::Create();
    graphics::Renderer& renderer = graphics::Renderer::instance();
    renderer.SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
    renderer.SetDepthTest(true);
    renderer.SetCullFace(true);
    renderer.SetBlend(true);

    window.AddEventHandler(
        [&window, &renderer](unsigned int msg, unsigned long long, long long)
        {
            if (msg == WM_SIZE)
                renderer.SetViewport(0, 0, window.GetWidth(), window.GetHeight());
        });

    scene::Scene::Create();
    scene::Scene& scene = scene::Scene::instance();

    if (config.onInit)
    {
        if (!config.onInit())
        {
            Fatal("onInit() returned false.");
            scene::Scene::Destroy();
            graphics::Renderer::Destroy();
            core::Input::Destroy();
            core::Window::Destroy();
            DetachConsole();
            return 1;
        }
    }

    while (true)
    {
        window.PollEvents();
        input.Update();

        if (config.fullscreenBolderless && window.isActive() && window.IsFullscreen())
            if (input.IsKeyPressed(core::KeyCode::Escape))
                window.Restore();

        framework::Clock::Update();

        if (config.onUpdate)
            config.onUpdate();

        if (window.IsCloseRequest())
            break;

        scene.RecomputeWorldTransforms();

        renderer.Clear();

        if (config.onRender)
            config.onRender();

        window.SwapBuffers();
    }

    if (config.onShutdown)
        config.onShutdown();

    // Tear down in reverse dependency order — Scene GL resources
    // released before Window destroys the GL context.
    scene::Scene::Destroy();
    graphics::Renderer::Destroy();
    core::Input::Destroy();
    core::Window::Destroy();

    DetachConsole();
    return 0;
}

// ===================================================================
//  Platform entry point — inside the library, hidden from user
// ===================================================================

#ifdef SGK_PLATFORM_WINDOWS

#ifndef _In_
#define _In_
#define _In_opt_
#endif
#define UNUSED(P) (P)

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNUSED(hPrevInstance); UNUSED(lpCmdLine); UNUSED(nShowCmd);
    SetProcessDPIAware();
    auto config = sgkit::CreateApplication();
    return Run(hInstance, config);
}
int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nShowCmd)
{
    UNUSED(hPrevInstance); UNUSED(lpCmdLine); UNUSED(nShowCmd);
    SetProcessDPIAware();
    auto config = sgkit::CreateApplication();
    return Run(hInstance, config);
}

#else
int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    auto config = sgkit::CreateApplication();
    return sgkit::Run(GetModuleHandle(nullptr), config);
}
#endif
