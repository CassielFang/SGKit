#include <sgkit/core/Window.h>

#ifdef _WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <resource.h>
#include <imm.h>
#include <glad/glad.h>
#include <glad/glad_wgl.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

// -- Helpers ------------------------------------------------------------

static std::wstring ToWideString(const std::string& utf8)
{
    if (utf8.empty())
        return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len <= 0)
        return L"";
    std::wstring result(len - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, result.data(), len);
    return result;
}

// -- Constants ------------------------------------------------------------

static const wchar_t* k_WindowClassName = L"SGKit_WindowClass";

// -- PIMPL definition ----------------------------------------------

class sgkit::core::Window::Impl
{
public:
    HWND      hwnd         = nullptr;
    HDC       dc           = nullptr;
    HGLRC     glContext    = nullptr;
    HINSTANCE hInstance    = nullptr;

    int  width                = 0;
    int  height               = 0;
    bool isCloseRequest       = false;
    bool isActive             = true;
    bool isFullscreen         = false;
    bool fullscreenBolderless = false;  // if true, maximize = borderless fullscreen
    bool cursorVisible        = true;

    // Pre-fullscreen state (for restore)
    LONG_PTR prevStyle    = 0;
    RECT     prevRect     = {};
    RECT     currRect     = {};

    // Callbacks for Input module
    std::vector<EventHandler> eventHandlers;
};

// -- Forward declarations (local helpers) --------------------------
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool    CreateDummyGLWindow(HWND* outHwnd, HDC* outDc, HGLRC* outRc, HINSTANCE hInst);
static void    DestroyDummyGLWindow(HWND hwnd, HDC dc, HGLRC rc);

#ifdef _DEBUG
static void WinLog(const char* msg, bool error = false)
{
    char buff[512]{};
    if (error)
        sprintf_s(buff, 512, "[  SGKit Window  ]: %s. (error code: %d)\n", msg, static_cast<int>(GetLastError()));
    else sprintf_s(buff, 512, "[  SGKit Window  ]: %s.\n", msg);
    std::fprintf(stderr, buff);
    OutputDebugStringA(buff);
}
#else
#define WinLog(...) ((void)0)
#endif

// -- External pointer to Window instance

static sgkit::core::Window* g_currentWindow = nullptr;

// ===================================================================
//  PUBLIC API
// ===================================================================

namespace sgkit {
namespace core {

Window::Window() : m_impl(std::make_unique<Impl>()) {}

Window::~Window()
{
    if (!m_impl->hwnd)
        return;

    wglMakeCurrent(m_impl->dc, nullptr);
    if (m_impl->glContext)
    {
        wglDeleteContext(m_impl->glContext);
        m_impl->glContext = nullptr;
    }

    if (m_impl->dc)
    {
        ReleaseDC(m_impl->hwnd, m_impl->dc);
        m_impl->dc = nullptr;
    }

    DestroyWindow(m_impl->hwnd);
    m_impl->hwnd = nullptr;

    PollEvents();

    UnregisterClassW(k_WindowClassName, m_impl->hInstance);
}

bool Window::Create(void* hInst, const WindowDesc& desc)
{
    if (g_currentWindow) return false;
    g_currentWindow = new Window;
    std::unique_ptr<Window::Impl>& m_impl = g_currentWindow->m_impl;

    m_impl->width  = desc.width;
    m_impl->height = desc.height;
    m_impl->hInstance = reinterpret_cast<HINSTANCE>(hInst);

    // -- 1. Register window class --------------------------------
    WNDCLASSEX wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = m_impl->hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = k_WindowClassName;

    // Load application icon from executable resources.
    // If the user placed app.ico in icon/ and linked app.rc, this picks it up.
    // On failure the window gets the default icon - no harm.
    HICON hIcon = LoadIconW(m_impl->hInstance, MAKEINTRESOURCEW(IDI_MAIN_ICON));
    wc.hIcon   = hIcon;
    wc.hIconSm = hIcon;

    if (RegisterClassEx(&wc)) WinLog("window class registered");
    else
    {
        WinLog("failed to register window class", true);
        return false;
    }

    // -- 2. Calculate window size --------------------------------
    DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    if (desc.resizable)
        dwStyle |= WS_SIZEBOX | WS_MAXIMIZEBOX;

    RECT rect = {0, 0, desc.width, desc.height};
    AdjustWindowRect(&rect, dwStyle, FALSE);
    int winW = rect.right - rect.left;
    int winH = rect.bottom - rect.top;

    // -- 3. Create a dummy GL window to load WGL extensions ------
    HWND  dummyWnd = nullptr;
    HDC   dummyDc  = nullptr;
    HGLRC dummyRc  = nullptr;
    if (!CreateDummyGLWindow(&dummyWnd, &dummyDc, &dummyRc, m_impl->hInstance))
    {
        WinLog("failed to create dummy window", true);
        DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);
        return false;
    }
    WinLog("dummy window created");
    
    // Get WGL extension functions
    if (!gladLoadWGL(dummyDc))
    {
        WinLog("failed to load WGL functions", true);
        DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);
        return false;
    }
    // Destroy dummy window - WGL extensions no longer needed
    DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);

    // -- 4. Create the real window -------------------------------
    std::wstring wideTitle = ToWideString(desc.title);

    m_impl->hwnd = CreateWindowEx(
        0, k_WindowClassName,
        wideTitle.c_str(),
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        nullptr, nullptr, m_impl->hInstance, nullptr
    );

    if (!m_impl->hwnd)
    {
        WinLog("failed to create main window", true);
        return false;
    }

    ShowWindow(m_impl->hwnd, SW_SHOW);
    UpdateWindow(m_impl->hwnd);

    m_impl->fullscreenBolderless = desc.fullscreenBolderless;
    if (desc.fullscreen)
        g_currentWindow->Maximize();

    // Disable IME by default (raw game input);
    ImmReleaseContext(m_impl->hwnd, ImmGetContext(m_impl->hwnd));
    ImmAssociateContext(m_impl->hwnd, nullptr);

    m_impl->dc = GetDC(m_impl->hwnd);
    WinLog("main window created");

    // -- 5. Set pixel format --------------------------------------
    bool pixelFormatOk = false;

    // Try with MSAA first
    const int pixelAttribs[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,       // WGL_DRAW_TO_WINDOW_ARB
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,       // WGL_SUPPORT_OPENGL_ARB
        WGL_DOUBLE_BUFFER_ARB, GL_TRUE,        // WGL_DOUBLE_BUFFER_ARB
        WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, // WGL_PIXEL_TYPE_ARB -> WGL_TYPE_RGBA_ARB
        WGL_COLOR_BITS_ARB,   24,              // WGL_COLOR_BITS_ARB
        WGL_DEPTH_BITS_ARB,   24,              // WGL_DEPTH_BITS_ARB
        WGL_STENCIL_BITS_ARB, 8,               // WGL_STENCIL_BITS_ARB
        WGL_SAMPLES_ARB,      4,               // WGL_SAMPLES_ARB (4x MSAA)
        0
    };

    int pixelFormat[32];
    UINT numFormats;
    BOOL ok = wglChoosePixelFormatARB(m_impl->dc, pixelAttribs, nullptr, 32,
                                        pixelFormat, &numFormats);
    if (ok && numFormats > 0)
    {
        PIXELFORMATDESCRIPTOR pfd;
        DescribePixelFormat(m_impl->dc, pixelFormat[0], sizeof(PIXELFORMATDESCRIPTOR), &pfd);
        if (SetPixelFormat(m_impl->dc, pixelFormat[0], &pfd))
        {
            pixelFormatOk = true;
            WinLog("pixel format set (with MSAA)");
        }
    }
    else
    {
        // Retry without MSAA
        const int noMsaAAttribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,       // WGL_DRAW_TO_WINDOW_ARB
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,       // WGL_SUPPORT_OPENGL_ARB
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,        // WGL_DOUBLE_BUFFER_ARB
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, // WGL_PIXEL_TYPE_ARB -> WGL_TYPE_RGBA_ARB
            WGL_COLOR_BITS_ARB,   24,              // WGL_COLOR_BITS_ARB
            WGL_DEPTH_BITS_ARB,   24,              // WGL_DEPTH_BITS_ARB
            WGL_STENCIL_BITS_ARB, 8,               // WGL_STENCIL_BITS_ARB
            0
        };
        ok = wglChoosePixelFormatARB(m_impl->dc, noMsaAAttribs, nullptr, 32,
                                        pixelFormat, &numFormats);
        if (ok && numFormats > 0)
        {
            PIXELFORMATDESCRIPTOR pfd;
            DescribePixelFormat(m_impl->dc, pixelFormat[0], sizeof(PIXELFORMATDESCRIPTOR), &pfd);
            if (SetPixelFormat(m_impl->dc, pixelFormat[0], &pfd))
            {
                pixelFormatOk = true;
                WinLog("pixel format set (no MSAA)");
            }
        }
    }

    if (!pixelFormatOk)
    {
        // Legacy ChoosePixelFormat fallback
        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion     = 1;
        pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType   = PFD_TYPE_RGBA;
        pfd.cColorBits   = 24;
        pfd.cDepthBits   = 24;
        pfd.cStencilBits = 8;

        int pf = ChoosePixelFormat(m_impl->dc, &pfd);
        if (pf && SetPixelFormat(m_impl->dc, pf, &pfd))
        {
            pixelFormatOk = true;
            WinLog("pixel format set (legacy ChoosePixelFormat)");
        }
    }

    if (!pixelFormatOk)
    {
        WinLog("failed to choose pixel format", true);
        DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);
        return false;
    }

    // -- 6. Create OpenGL context ---------------------------------
    const int contextAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, desc.glMajorVersion,
        WGL_CONTEXT_MINOR_VERSION_ARB, desc.glMinorVersion,
        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, // WGL_CONTEXT_PROFILE_MASK -> CORE
#ifdef _DEBUG
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
#else
        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#endif
        0
    };

    m_impl->glContext = wglCreateContextAttribsARB(m_impl->dc, 0, contextAttribs);
    if (!m_impl->glContext)
    {
        WinLog("4.x core context failed, trying 3.3", true);
        // Fall back to 3.3 core
        const int fallbackAttribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        m_impl->glContext = wglCreateContextAttribsARB(m_impl->dc, 0, fallbackAttribs);
    }

    if (!m_impl->glContext)
    {
        WinLog("3.3 context failed, trying legacy", true);
        // Legacy fallback
        m_impl->glContext = wglCreateContext(m_impl->dc);
    }

    if (!m_impl->glContext)
    {
        WinLog("all GL context attempts falied", true);
        DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);
        return false;
    }

    // Make our context current
    wglMakeCurrent(m_impl->dc, m_impl->glContext);
    WinLog("GL context created and made current");

    // -- 7. VSync -------------------------------------------------
    if (desc.vsync) wglSwapIntervalEXT(1);

    // -- 8. GL ----------------------------------------------------
    if (!gladLoadGL())
    {
        WinLog("failed to load OpenGL functions", true);
        return false;
    }

#ifdef _DEBUG
    if (GLAD_GL_VERSION_4_3 || GLAD_GL_KHR_debug)
    {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(
            [](GLenum, GLenum type, GLuint, GLenum severity, GLsizei, const GLchar* msg, const void*)
            {
                if (severity != GL_DEBUG_SEVERITY_NOTIFICATION)
                {
                    char buff[512]{};
                    sprintf_s(buff, 512, "[       GL       ]: (%u) %s\n", type, msg);
                    std::fprintf(stderr, buff);
                    OutputDebugStringA(buff);
                }
            }, nullptr);
    }
#endif

    std::fprintf(stderr, "[  SGKit Window  ]: OpenGL version %s, GLSL version %s\n",
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    ("module created");
    return true;
}

void Window::Destroy()
{
    if (!g_currentWindow) return;
    delete g_currentWindow;
    g_currentWindow = nullptr;
    WinLog("module destroyed");
}

Window& Window::instance()
{
    return *g_currentWindow;
}

void Window::PollEvents()
{
    MSG msg;
    while (PeekMessage(&msg, m_impl->hwnd, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Window::RequestClose(bool request)
{
    m_impl->isCloseRequest = request;
}

bool Window::IsCloseRequest() const
{
    return m_impl->isCloseRequest;
}

void Window::SwapBuffers()
{
    if (m_impl->dc)
        ::SwapBuffers(m_impl->dc);
}

void Window::Maximize()
{
    if (m_impl->hwnd)
        SetFullscreen(true);
}

void Window::Minimize()
{
    if (m_impl->hwnd)
        ShowWindow(m_impl->hwnd, SW_MINIMIZE);
}

void Window::Restore()
{
    if (m_impl->hwnd)
    {
        if (IsFullscreen())
            SetFullscreen(false);
        else
            ShowWindow(m_impl->hwnd, SW_RESTORE);
    }
}

bool Window::isActive() const
{
    return m_impl->isActive;
}

void Window::SetFullscreen(bool enabled)
{
    if (!m_impl->hwnd || m_impl->isFullscreen == enabled) return;

    if (enabled)
    {
        // Save current state
        m_impl->prevStyle = GetWindowLongPtrW(m_impl->hwnd, GWL_STYLE);
        GetWindowRect(m_impl->hwnd, &m_impl->prevRect);
        if (m_impl->fullscreenBolderless)
        {
            // Remove borders, cover entire monitor
            SetWindowLongPtrW(m_impl->hwnd, GWL_STYLE,
                static_cast<LONG_PTR>(WS_POPUPWINDOW | WS_VISIBLE | WS_MAXIMIZE));
        }
        else
        {
            SetWindowLongPtrW(m_impl->hwnd, GWL_STYLE, m_impl->prevStyle | WS_MAXIMIZE);
        }
        // Get monitor dimensions
        HMONITOR monitor = MonitorFromWindow(m_impl->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(monitor, &mi);
        SetWindowPos(m_impl->hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED);

        m_impl->width  = mi.rcMonitor.right - mi.rcMonitor.left;
        m_impl->height = mi.rcMonitor.bottom - mi.rcMonitor.top;
    }
    else
    {
        // Restore previous style and position
        SetWindowLongPtrW(m_impl->hwnd, GWL_STYLE, m_impl->prevStyle);
        SetWindowPos(m_impl->hwnd, HWND_NOTOPMOST,
            m_impl->prevRect.left, m_impl->prevRect.top,
            m_impl->prevRect.right - m_impl->prevRect.left,
            m_impl->prevRect.bottom - m_impl->prevRect.top,
            SWP_FRAMECHANGED);

        m_impl->width  = m_impl->prevRect.right - m_impl->prevRect.left;
        m_impl->height = m_impl->prevRect.bottom - m_impl->prevRect.top;
    }

    m_impl->isFullscreen = enabled;
}

bool Window::IsFullscreen() const
{
    return m_impl->isFullscreen;
}

void Window::SetCursorVisible(bool enabled)
{
    if (!m_impl->hwnd || m_impl->cursorVisible == enabled) return;
    m_impl->cursorVisible = enabled;
    if (enabled)
    {
        ShowCursor(TRUE);
        ClipCursor(nullptr);
    }
    else
    {
        ShowCursor(FALSE);
        GetWindowRect(m_impl->hwnd, &m_impl->currRect);
        ClipCursor(&m_impl->currRect);
    }
}

bool Window::isCursorVisible() const
{
    return m_impl->cursorVisible;
}

int Window::GetWidth() const  { return m_impl ? m_impl->width : 0; }
int Window::GetHeight() const { return m_impl ? m_impl->height : 0; }

float Window::GetAspectRatio() const
{
    return (m_impl && m_impl->height > 0) ? (float)m_impl->width / (float)m_impl->height : 1.0f;
}

void* Window::GetNativeHandle() const
{
    return static_cast<void*>(m_impl->hwnd);
}

void Window::AddEventHandler(EventHandler handler)
{
    m_impl->eventHandlers.push_back(std::move(handler));
}

int64_t Window::HandleWindowMessage(unsigned int msg, unsigned long long wParam, long long lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        m_impl->isCloseRequest = true;
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_MAXIMIZE)
        {
            Maximize();
            return true;
        }
        else if ((wParam & 0xFFF0) == SC_RESTORE)
        {
            Restore();
            return true;
        }
        else if ((wParam & 0xFFF0) == SC_MINIMIZE)
        {
            Minimize();
            return true;
        }
        break;

    case WM_SIZE:
        m_impl->width  = LOWORD(static_cast<LPARAM>(lParam));
        m_impl->height = HIWORD(static_cast<LPARAM>(lParam));
        break;

    case WM_SETFOCUS:
        m_impl->isActive = true;
        break;
    case WM_KILLFOCUS:
        m_impl->isActive = false;
        break;
    }

    // Dispatch to registered event handlers
    for (auto& handler : m_impl->eventHandlers)
    {
        if (handler)  handler(msg, wParam, lParam);
    }

    return DefWindowProc(m_impl->hwnd, msg, wParam, lParam);
}

} // namespace core
} // namespace sgkit

// ===================================================================
//  INTERNAL: Window Procedure + Dummy Window
// ===================================================================

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_currentWindow && g_currentWindow->GetNativeHandle())
        return g_currentWindow->HandleWindowMessage(msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

static bool CreateDummyGLWindow(HWND* outHwnd, HDC* outDc, HGLRC* outRc, HINSTANCE hInst)
{

    HWND hwnd = CreateWindow(
        k_WindowClassName, L"Dummy",
        WS_POPUP, 0, 0, 1, 1, nullptr, nullptr, hInst, nullptr
    );
    if (!hwnd) return false;

    HDC dc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 24;
    pfd.cAlphaBits   = 8;
    pfd.cDepthBits   = 24;

    int pf = ChoosePixelFormat(dc, &pfd);
    SetPixelFormat(dc, pf, &pfd);

    HGLRC rc = wglCreateContext(dc);
    wglMakeCurrent(dc, rc);

    *outHwnd = hwnd;
    *outDc   = dc;
    *outRc   = rc;
    return true;
}

static void DestroyDummyGLWindow(HWND hwnd, HDC dc, HGLRC rc)
{
    wglMakeCurrent(nullptr, nullptr);
    if (rc) wglDeleteContext(rc);
    if (dc) ReleaseDC(hwnd, dc);
    if (hwnd) DestroyWindow(hwnd);
}

#endif
