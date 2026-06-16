#include <sgkit/core/Window.h>

#ifdef SGK_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <imm.h>
#include <gl/GL.h>

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

struct sgkit::core::Window::Impl
{
    HWND      hwnd         = nullptr;
    HDC       dc           = nullptr;
    HGLRC     glContext    = nullptr;
    HINSTANCE hInstance    = nullptr;

    int  width            = 0;
    int  height           = 0;
    bool isRunning        = false;
    bool hasGLContext     = false;
    bool isFullscreen     = false;
    bool maximizeIsFullscreen = false;  // if true, maximize = borderless fullscreen

    // IME
    HIMC defaultIMC = nullptr;

    // Pre-fullscreen state (for restore)
    LONG_PTR prevStyle    = 0;
    RECT     prevRect     = {};

    // Callbacks for Input module
    std::vector<EventHandler> eventHandlers;
};

// -- Forward declarations (local helpers) --------------------------
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool     CreateDummyGLWindow(HWND* outHwnd, HDC* outDc, HGLRC* outRc, HINSTANCE hInst);
static void     DestroyDummyGLWindow(HWND hwnd, HDC dc, HGLRC rc);

// WGL extension function types
typedef HGLRC (WINAPI *PFN_wglCreateContextAttribsARB)(HDC, HGLRC, const int*);
typedef BOOL (WINAPI *PFN_wglChoosePixelFormatARB)(HDC, const int*, const float*, UINT, int*, UINT*);
typedef BOOL (WINAPI *PFN_wglSwapIntervalEXT)(int);

// -- External pointer to Window instance (for WindowProc callback) -

static sgkit::core::Window* g_currentWindow = nullptr;

// ===================================================================
//  PUBLIC API
// ===================================================================

namespace sgkit {
namespace core {

Window::Window() : m_impl(std::make_unique<Impl>()) {}

Window::~Window()
{
    Destroy();
}

static void WinLog(const char* msg)
{
    std::fprintf(stderr, "%s", msg);
}

bool Window::Create(const WindowDesc& desc)
{
    if (m_impl->hwnd)
        Destroy();

    m_impl->width  = desc.width;
    m_impl->height = desc.height;
    m_impl->hInstance = GetModuleHandle(nullptr);
    m_impl->isRunning  = false;

    // -- 1. Register window class --------------------------------
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = m_impl->hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = k_WindowClassName;

    // Load application icon from executable resources (ID 101 = IDI_MAIN_ICON).
    // If the user placed app.ico in icon/ and linked app.rc, this picks it up.
    // On failure the window gets the default icon — no harm.
    HICON hIcon = LoadIconW(m_impl->hInstance, MAKEINTRESOURCEW(101));
    wc.hIcon   = hIcon;
    wc.hIconSm = hIcon;

    RegisterClassExW(&wc);
    WinLog("SGKit Window: class registered\n");

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
        WinLog("SGKit Window: FAILED dummy window\n");
        return false;
    }

    // Get WGL extension functions
    auto wglChoosePixelFormatARB = (PFN_wglChoosePixelFormatARB)
        wglGetProcAddress("wglChoosePixelFormatARB");
    auto wglCreateContextAttribsARB = (PFN_wglCreateContextAttribsARB)
        wglGetProcAddress("wglCreateContextAttribsARB");

    WinLog("SGKit Window: dummy window created, WGL extensions loaded\n");

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
        DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);
        WinLog("SGKit Window: FAILED CreateWindowExW\n");
        return false;
    }

    m_impl->dc = GetDC(m_impl->hwnd);
    WinLog("SGKit Window: real window created\n");

    // -- 5. Set pixel format --------------------------------------
    bool pixelFormatOk = false;

    if (wglChoosePixelFormatARB)
    {
        // Try with MSAA first
        const int pixelAttribs[] = {
            0x2001, GL_TRUE,        // WGL_DRAW_TO_WINDOW_ARB
            0x2002, GL_TRUE,        // WGL_SUPPORT_OPENGL_ARB
            0x2011, GL_TRUE,        // WGL_DOUBLE_BUFFER_ARB
            0x2013, 0x202B,         // WGL_PIXEL_TYPE_ARB → WGL_TYPE_RGBA_ARB
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
                WinLog("SGKit Window: pixel format set (with MSAA)\n");
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
                    WinLog("SGKit Window: pixel format set (no MSAA)\n");
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
            WinLog("SGKit Window: pixel format set (legacy ChoosePixelFormat)\n");
        }
    }

    if (!pixelFormatOk)
    {
        DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);
        WinLog("SGKit Window: FAILED pixel format\n");
        return false;
    }

    // -- 6. Create OpenGL context ---------------------------------
    if (wglCreateContextAttribsARB)
    {
        const int contextAttribs[] = {
            0x2091, desc.glMajorVersion,
            0x2092, desc.glMinorVersion,
            0x9126, 0x0001,         // WGL_CONTEXT_PROFILE_MASK → CORE
#ifdef _DEBUG
            0x2094, 0x0001,         // WGL_CONTEXT_FLAGS → DEBUG
#endif
            0
        };

        m_impl->glContext = wglCreateContextAttribsARB(m_impl->dc, 0, contextAttribs);
        if (!m_impl->glContext)
        {
            WinLog("SGKit Window: 4.x core context failed, trying 3.3\n");
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
        WinLog("SGKit Window: 3.3 context failed, trying legacy\n");
        // Legacy fallback
        m_impl->glContext = wglCreateContext(m_impl->dc);
    }

    if (!m_impl->glContext)
    {
        DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);
        WinLog("SGKit Window: FAILED all GL context attempts\n");
        return false;
    }

    // Destroy dummy window — WGL extensions no longer needed
    DestroyDummyGLWindow(dummyWnd, dummyDc, dummyRc);

    // Make our context current
    wglMakeCurrent(m_impl->dc, m_impl->glContext);

    WinLog("SGKit Window: GL context created and made current\n");

    // -- 7. VSync -------------------------------------------------
    if (desc.vsync)
    {
        auto wglSwapIntervalEXT = (PFN_wglSwapIntervalEXT)
            wglGetProcAddress("wglSwapIntervalEXT");
        if (wglSwapIntervalEXT)
            wglSwapIntervalEXT(1);
    }

    m_impl->hasGLContext = true;
    m_impl->isRunning    = true;
    g_currentWindow      = this;

    ShowWindow(m_impl->hwnd, SW_SHOW);
    UpdateWindow(m_impl->hwnd);

    // Disable IME by default (raw game input); user can re-enable with SetIMEEnabled(true)
    m_impl->defaultIMC = ImmGetContext(m_impl->hwnd);
    ImmReleaseContext(m_impl->hwnd, m_impl->defaultIMC);
    SetIMEEnabled(false);

    m_impl->maximizeIsFullscreen = desc.fullscreen;
    if (desc.fullscreen)
        SetFullscreen(true);

    return true;
}

void Window::Destroy()
{
    if (!m_impl->hwnd)
        return;

    g_currentWindow = nullptr;

    if (m_impl->hasGLContext)
    {
        wglMakeCurrent(m_impl->dc, nullptr);
        if (m_impl->glContext)
        {
            wglDeleteContext(m_impl->glContext);
            m_impl->glContext = nullptr;
        }
        m_impl->hasGLContext = false;
    }

    if (m_impl->dc)
    {
        ReleaseDC(m_impl->hwnd, m_impl->dc);
        m_impl->dc = nullptr;
    }

    DestroyWindow(m_impl->hwnd);
    m_impl->hwnd = nullptr;

    UnregisterClassW(k_WindowClassName, m_impl->hInstance);
}

bool Window::IsCreated() const
{
    return m_impl->hwnd != nullptr;
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

bool Window::IsRunning() const
{
    return m_impl->isRunning;
}

void Window::RequestClose()
{
    PostMessage(m_impl->hwnd, WM_CLOSE, 0, 0);
}

void Window::SwapBuffers()
{
    if (m_impl->dc)
        ::SwapBuffers(m_impl->dc);
}

void Window::Maximize()
{
    if (!m_impl->hwnd)
        return;
    if (m_impl->maximizeIsFullscreen)
        SetFullscreen(true);
    else
        ShowWindow(m_impl->hwnd, SW_MAXIMIZE);
}

void Window::Minimize()
{
    if (m_impl->hwnd)
        ShowWindow(m_impl->hwnd, SW_MINIMIZE);
}

void Window::Restore()
{
    if (m_impl->hwnd)
        ShowWindow(m_impl->hwnd, SW_RESTORE);
}

void Window::SetFullscreen(bool enabled)
{
    if (!m_impl->hwnd)
        return;
    if (m_impl->isFullscreen == enabled)
        return;

    if (enabled)
    {
        // Save current state
        m_impl->prevStyle = GetWindowLongPtrW(m_impl->hwnd, GWL_STYLE);
        GetWindowRect(m_impl->hwnd, &m_impl->prevRect);

        // Get monitor dimensions
        HMONITOR monitor = MonitorFromWindow(m_impl->hwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(monitor, &mi);

        // Remove borders, cover entire monitor
        SetWindowLongPtrW(m_impl->hwnd, GWL_STYLE,
            static_cast<LONG_PTR>(WS_POPUP | WS_VISIBLE));
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

void Window::SetIMEEnabled(bool enabled)
{
    if (!m_impl->hwnd) return;
    if (enabled)
        ImmAssociateContext(m_impl->hwnd, m_impl->defaultIMC);
    else
        ImmAssociateContext(m_impl->hwnd, nullptr);
}

int Window::GetWidth() const  { return m_impl->width; }
int Window::GetHeight() const { return m_impl->height; }

float Window::GetAspectRatio() const
{
    return (m_impl->height > 0) ? (float)m_impl->width / (float)m_impl->height : 1.0f;
}

void* Window::GetNativeHandle() const
{
    return static_cast<void*>(m_impl->hwnd);
}

void Window::AddEventHandler(EventHandler handler)
{
    m_impl->eventHandlers.push_back(std::move(handler));
}

bool Window::HandleWindowMessage(unsigned int msg, unsigned long long wParam, long long lParam)
{
    // Dispatch to registered event handlers (Input module)
    for (auto& handler : m_impl->eventHandlers)
    {
        if (handler && handler(msg, wParam, lParam))
            return true;
    }

    switch (msg)
    {
    case WM_CLOSE:
        m_impl->isRunning = false;
        return true;

    case WM_SYSCOMMAND:
        if (m_impl->maximizeIsFullscreen && (wParam & 0xFFF0) == SC_MAXIMIZE)
        {
            SetFullscreen(true);
            return true;
        }
        if (m_impl->maximizeIsFullscreen && (wParam & 0xFFF0) == SC_RESTORE && m_impl->isFullscreen)
        {
            SetFullscreen(false);
            return true;
        }
        break;

    case WM_SIZE:
        m_impl->width  = LOWORD(static_cast<LPARAM>(lParam));
        m_impl->height = HIWORD(static_cast<LPARAM>(lParam));
        return true;
    }

    return false;
}

} // namespace core
} // namespace sgkit

// ===================================================================
//  INTERNAL: Window Procedure + Dummy Window
// ===================================================================

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    sgkit::core::Window* window = g_currentWindow;

    if (window && window->HandleWindowMessage(msg, wParam, lParam))
        return 0;

    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static bool CreateDummyGLWindow(HWND* outHwnd, HDC* outDc, HGLRC* outRc, HINSTANCE hInst)
{
    HWND hwnd = CreateWindowExW(
        0, k_WindowClassName, L"Dummy",
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
