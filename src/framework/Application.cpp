#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <sgkit/framework/Application.h>
#include <sgkit/framework/Timing.h>

#include <sgkit/core/Window.h>
#include <sgkit/core/ThreadPool.h>
#include <sgkit/core/Input.h>
#include <sgkit/scene/Renderer.h>
#include <sgkit/scene/Scene.h>

#include <sgkit/framework/DebugOut.h>

#include <cstdio>
#include <memory>

using namespace sgkit;

#ifdef _DEBUG
static void AttachConsole()
{
#ifdef _WINDOWS
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
#ifdef _WINDOWS
    fclose(stdout);
    fclose(stderr);
    fclose(stdin);
    FreeConsole();
#endif
}
#else
#define AttachConsole() (void(0))
#define DetachConsole() (void(0))
#endif

static void Fatal(const char* msg)
{
    SGK_LOG_FATAL("App", "%s", msg);
#ifdef _WINDOWS
    MessageBoxA(nullptr, msg, "SGKit Fatal Error", MB_OK | MB_ICONERROR);
#endif
}

static int Run(HINSTANCE hInst, const ApplicationConfig& config)
{
    AttachConsole();
    // -- Init modules in dependency order

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

    core::ThreadPool::Create(config.numThreads);

    if (!core::Input::Create(window.GetNativeHandle()))
    {
        Fatal("Failed to initialize input device.");
        core::Input::Destroy();
        core::ThreadPool::Destroy();
        core::Window::Destroy();
        DetachConsole();
        return 1;
    }
    core::Input& input = core::Input::instance();

    scene::Renderer::Create();
    scene::Renderer& renderer = scene::Renderer::instance();
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
            scene::Renderer::Destroy();
            core::Input::Destroy();
            core::ThreadPool::Destroy();
            core::Window::Destroy();
#ifdef _DEBUG
            DetachConsole();
#endif
            return 1;
        }
    }

    bool previousActive = true;

    while (true)
    {
        window.PollEvents();
        input.Update();
        framework::Clock::Update();

        if (previousActive)
        {
            if (config.fullscreenBolderless && window.isActive() && window.IsFullscreen())
                if (input.IsKeyPressed(core::KeyCode::Escape))
                    window.Restore();

            if (config.onUpdate) config.onUpdate();

            if (window.IsCloseRequest()) break;

            scene.RecomputeWorldTransforms();

            if (config.onRender) config.onRender();

            window.SwapBuffers();
        }
        else
        {
            if (window.IsCloseRequest())
            {
                if (config.onUpdate) config.onUpdate();
                if (window.IsCloseRequest()) break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        previousActive = window.isActive();
    }

    if (config.onShutdown)
        config.onShutdown();

    // Tear down in reverse dependency order - Scene GL resources
    // released before Window destroys the GL context.
    scene::Scene::Destroy();
    scene::Renderer::Destroy();
    core::Input::Destroy();
    core::ThreadPool::Destroy();
    core::Window::Destroy();

    DetachConsole();
    return 0;
}

//  Platform entry point - inside the library, hidden from user

#ifdef _WINDOWS

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
