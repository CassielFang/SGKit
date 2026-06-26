#include <sgkit/core/Window.h>

#ifdef SGK_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <resource.h>
#include <imm.h>
#include <glad/glad.h>

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
    if (error)
        std::fprintf(stderr, "[  SGKit Window  ]: %s. (error code: %d)\n", msg, static_cast<int>(GetLastError()));
    else std::fprintf(stderr, "[  SGKit Window  ]: %s.\n", msg);
}
#else
#define WinLog(...) ((void)0)
#endif

// WGL extension function types
typedef HGLRC (WINAPI *PFN_wglCreateContextAttribsARB)(HDC, HGLRC, const int*);
typedef BOOL (WINAPI *PFN_wglChoosePixelFormatARB)(HDC, const int*, const float*, UINT, int*, UINT*);
typedef BOOL (WINAPI *PFN_wglSwapIntervalEXT)(int);

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
    // On failure the window gets the default icon — no harm.
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

    // Get WGL extension functions
    auto wglChoosePixelFormatARB = (PFN_wglChoosePixelFormatARB)
        wglGetProcAddress("wglChoosePixelFormatARB");
    auto wglCreateContextAttribsARB = (PFN_wglCreateContextAttribsARB)
        wglGetProcAddress("wglCreateContextAttribsARB");

    WinLog("dummy window created, WGL extensions loaded");

    // -- 4. Create the real window -------------------------------
    std::wstring wideTitle = ToWideString(desc.title);

    m_impl->hwnd = CreateWindowExW(
        0, k_WindowClassName,
        wideTitle.c_str(),
        dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        nullptr, nullptr, m_impl->hInstance, nullptr
    );

    if (!m_impl->hwnd)
    {
        WinLog("failed to create main window", true);
        DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);
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

    if (wglChoosePixelFormatARB)
    {
        // Try with MSAA first
        const int pixelAttribs[] = {
            0x2001, GL_TRUE,        // WGL_DRAW_TO_WINDOW_ARB
            0x2002, GL_TRUE,        // WGL_SUPPORT_OPENGL_ARB
            0x2011, GL_TRUE,        // WGL_DOUBLE_BUFFER_ARB
            0x2013, 0x202B,         // WGL_PIXEL_TYPE_ARB -> WGL_TYPE_RGBA_ARB
            0x2014, 24,             // WGL_COLOR_BITS_ARB
            0x2022, 24,             // WGL_DEPTH_BITS_ARB
            0x2016, 8,              // WGL_STENCIL_BITS_ARB
            0x2010, 4,              // WGL_SAMPLES_ARB (4x MSAA)
            0
        };

        int pixelFormat;
        UINT numFormats;
        BOOL ok = wglChoosePixelFormatARB(m_impl->dc, pixelAttribs, nullptr, 1,
                                          &pixelFormat, &numFormats);
        if (ok && numFormats > 0)
        {
            PIXELFORMATDESCRIPTOR pfd = {};
            DescribePixelFormat(m_impl->dc, pixelFormat, sizeof(pfd), &pfd);
            if (SetPixelFormat(m_impl->dc, pixelFormat, &pfd))
            {
                pixelFormatOk = true;
                WinLog("pixel format set (with MSAA)");
            }
        }
        else
        {
            // Retry without MSAA
            const int noMsaAAttribs[] = {
                0x2001, GL_TRUE,
                0x2002, GL_TRUE,
                0x2011, GL_TRUE,
                0x2013, 0x202B,
                0x2014, 24,
                0x2022, 24,
                0x2016, 8,
                0
            };
            ok = wglChoosePixelFormatARB(m_impl->dc, noMsaAAttribs, nullptr, 1,
                                         &pixelFormat, &numFormats);
            if (ok && numFormats > 0)
            {
                PIXELFORMATDESCRIPTOR pfd = {};
                DescribePixelFormat(m_impl->dc, pixelFormat, sizeof(pfd), &pfd);
                if (SetPixelFormat(m_impl->dc, pixelFormat, &pfd))
                {
                    pixelFormatOk = true;
                    WinLog("pixel format set (no MSAA)");
                }
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
    if (wglCreateContextAttribsARB)
    {
        const int contextAttribs[] = {
            0x2091, desc.glMajorVersion,
            0x2092, desc.glMinorVersion,
            0x9126, 0x0001,         // WGL_CONTEXT_PROFILE_MASK -> CORE
#ifdef _DEBUG
            0x2094, 0x0001,         // WGL_CONTEXT_FLAGS -> DEBUG
#endif
            0
        };

        m_impl->glContext = wglCreateContextAttribsARB(m_impl->dc, 0, contextAttribs);
        if (!m_impl->glContext)
        {
            WinLog("4.x core context failed, trying 3.3", true);
            // Fall back to 3.3 core
            const int fallbackAttribs[] = {
                0x2091, 3,
                0x2092, 3,
                0x9126, 0x0001,
                0
            };
            m_impl->glContext = wglCreateContextAttribsARB(m_impl->dc, 0, fallbackAttribs);
        }
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

    // Destroy dummy window — WGL extensions no longer needed
    DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);

    // Make our context current
    wglMakeCurrent(m_impl->dc, m_impl->glContext);

    WinLog("GL context created and made current");

    // -- 7. VSync -------------------------------------------------
    if (desc.vsync)
    {
        auto wglSwapIntervalEXT = (PFN_wglSwapIntervalEXT)
            wglGetProcAddress("wglSwapIntervalEXT");
        if (wglSwapIntervalEXT)
            wglSwapIntervalEXT(1);
    }

    // -- 8. GL ----------------------------------------------------
    if (!gladLoadGL())
    {
        WinLog("failed to load OpenGL functions",true);
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
                    std::fprintf(stderr, "[GL]              : (%u) %s\n", type, msg);
            }, nullptr);
    }
#endif

    std::fprintf(stderr, "[  SGKit Window  ]: OpenGL version %s, GLSL version %s\n",
        glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    WinLog("module created");
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
    ShowCursor(enabled ? TRUE : FALSE);
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
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize        = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 24;

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

#endif // SGK_PLATFORM_WINDOWS
