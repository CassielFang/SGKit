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
#include <sgkit/core/Debug.h>
#include <sgkit/graphics/Renderer.h>
#include <sgkit/scene/Scene.h>

#include <glad/glad.h>
#include <cstdio>
#include <memory>

namespace sgkit {
namespace {

// Module singletons — unique_ptr so destruction order is explicit
// Scene/Renderer release GL resources before Window tears down the GL context
std::unique_ptr<core::Window>       g_window;
std::unique_ptr<core::Input>        g_input;
std::unique_ptr<graphics::Renderer> g_renderer;
std::unique_ptr<scene::Scene>       g_scene;

// Timing
framework::Clock::TimePoint g_lastFrameTime;
float  g_deltaTime  = 0.0f;
float  g_totalTime  = 0.0f;
float  g_fpsTimer   = 0.0f;
int    g_frameCount = 0;
float  g_fps        = 0.0f;
bool   g_running    = false;

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

static void CalculateFrameTiming()
{
    auto now = framework::Clock::Now();
    auto duration = std::chrono::duration<float>(now - g_lastFrameTime);
    g_deltaTime = duration.count();
    g_lastFrameTime = now;

    g_totalTime += g_deltaTime;
    g_frameCount++;

    g_fpsTimer += g_deltaTime;
    if (g_fpsTimer >= 1.0f)
    {
        g_fps = static_cast<float>(g_frameCount) / g_fpsTimer;
        g_frameCount = 0;
        g_fpsTimer = 0.0f;
    }
}

static void Fatal(const char* msg)
{
    std::fprintf(stderr, "SGKit FATAL: %s\n", msg);

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

} // anonymous namespace

static int Run(const ApplicationConfig& config)
{
    AttachConsole();

    std::printf("SGKit: starting up\n");

    // -- Init modules in dependency order ---------------------------------

    core::WindowDesc wd;
    wd.title          = config.title;
    wd.width          = config.width;
    wd.height         = config.height;
    wd.vsync          = config.vsync;
    wd.fullscreen     = config.fullscreen;
    wd.glMajorVersion = config.glMajor;
    wd.glMinorVersion = config.glMinor;

    g_window = std::make_unique<core::Window>();
    if (!g_window->Create(wd))
    {
        Fatal("Failed to create window. GPU may not support OpenGL 3.3+.");
        g_window.reset();
        DetachConsole();
        return 1;
    }

    if (!gladLoadGL())
    {
        Fatal("Failed to load OpenGL functions.");
        g_window.reset();
        DetachConsole();
        return 1;
    }

#ifdef _DEBUG
    if (GLAD_GL_VERSION_4_3 || GLAD_GL_KHR_debug)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback([](GLenum, GLenum type, GLuint, GLenum severity,
                                  GLsizei, const GLchar* msg, const void*) {
            if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
                std::fprintf(stderr, "GL: (%u) %s\n", type, msg);
        }, nullptr);
    }
#endif

    std::printf("SGKit: OpenGL %s, GLSL %s\n",
                glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

    g_renderer = std::make_unique<graphics::Renderer>();
    g_renderer->SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
    g_renderer->SetDepthTest(true);
    g_renderer->SetCullFace(true);
    g_renderer->SetBlend(true);

    g_input = std::make_unique<core::Input>();
    g_input->Initialize(g_window->GetNativeHandle());
    g_window->AddEventHandler(
        [](unsigned int msg, unsigned long long wParam, long long lParam) -> bool {
            return g_input->OnEvent(msg, wParam, lParam);
        });

    g_scene = std::make_unique<scene::Scene>();

    if (config.onInit)
    {
        if (!config.onInit())
        {
            Fatal("onInit() returned false.");
            g_scene.reset();
            g_renderer.reset();
            g_input.reset();
            g_window.reset();
            DetachConsole();
            return 1;
        }
    }

    g_running = true;
    g_lastFrameTime = framework::Clock::Now();

    while (g_running && g_window->IsRunning())
    {
        g_window->PollEvents();

        if (!g_window->IsRunning())
            break;

        if (g_window->IsFullscreen() && g_input->IsKeyPressed(core::KeyCode::k_Escape))
            g_window->SetFullscreen(false);

        CalculateFrameTiming();

        if (config.onUpdate)
            config.onUpdate(g_deltaTime);

        g_scene->RecomputeWorldTransforms();

        g_renderer->Clear();

        if (g_window->HasResized())
        {
            g_renderer->SetViewport(0, 0, g_window->GetWidth(), g_window->GetHeight());
            g_window->ResetResizeFlag();
        }

        if (config.onRender)
            config.onRender();

        g_input->EndFrame();
        g_window->SwapBuffers();
    }

    if (config.onShutdown)
        config.onShutdown();

    // Tear down in reverse dependency order — Scene GL resources
    // released before Window destroys the GL context.
    g_scene.reset();
    g_renderer.reset();
    g_input.reset();
    g_window.reset();

    DetachConsole();
    return 0;
}

core::Window&        GetWindow()   { return *g_window; }
core::Input&         GetInput()    { return *g_input; }
graphics::Renderer&  GetRenderer() { return *g_renderer; }
scene::Scene&        GetScene()    { return *g_scene; }

float GetDeltaTime() { return g_deltaTime; }
float GetFPS()       { return g_fps; }
void  RequestQuit()  { g_running = false; }

} // namespace sgkit

// ===================================================================
//  Platform entry point — inside the library, hidden from user
// ===================================================================

#ifdef SGK_PLATFORM_WINDOWS

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    SetProcessDPIAware();
    auto config = sgkit::CreateApplication();
    return sgkit::Run(config);
}
int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    SetProcessDPIAware();
    auto config = sgkit::CreateApplication();
    return sgkit::Run(config);
}

#else
int main(int argc, char** argv)
{
    (void)argc; (void)argv;
    auto config = sgkit::CreateApplication();
    return sgkit::Run(config);
}
#endif
