#include <sgkit/core/Input.h>

#include <sgkit/core/DebugOut.h>

#ifdef _WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#include <cstring>

static sgkit::core::Input* g_Input = nullptr;

namespace sgkit {
namespace core {

// VK lookup table  -  KeyCode index -> Win32 virtual-key code

static constexpr unsigned int k_KeyCodeToVK[] = {
    /* Unknown        0 */ 0,
    /* Space          1 */ VK_SPACE,
    /* Apostrophe     2 */ VK_OEM_7,
    /* Comma          3 */ VK_OEM_COMMA,
    /* Minus          4 */ VK_OEM_MINUS,
    /* Period         5 */ VK_OEM_PERIOD,
    /* Slash          6 */ VK_OEM_2,
    /* k_0 .. k_9  7-16 */ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    /* Semicolon     17 */ VK_OEM_1,
    /* Equal         18 */ VK_OEM_PLUS,
    /* A .. Z     19-44 */ 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                           'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                           'U', 'V', 'W', 'X', 'Y', 'Z',
    /* LeftBracket   45 */ VK_OEM_4,
    /* Backslash     46 */ VK_OEM_5,
    /* RightBracket  47 */ VK_OEM_6,
    /* GraveAccent   48 */ VK_OEM_3,
    /* Escape        49 */ VK_ESCAPE,
    /* Enter         50 */ VK_RETURN,
    /* Tab           51 */ VK_TAB,
    /* Backspace     52 */ VK_BACK,
    /* Insert        53 */ VK_INSERT,
    /* Delete        54 */ VK_DELETE,
    /* Right         55 */ VK_RIGHT,
    /* Left          56 */ VK_LEFT,
    /* Down          57 */ VK_DOWN,
    /* Up            58 */ VK_UP,
    /* PageUp        59 */ VK_PRIOR,
    /* PageDown      60 */ VK_NEXT,
    /* Home          61 */ VK_HOME,
    /* End           62 */ VK_END,
    /* CapsLock      63 */ VK_CAPITAL,
    /* ScrollLock    64 */ VK_SCROLL,
    /* NumLock       65 */ VK_NUMLOCK,
    /* PrintScreen   66 */ VK_SNAPSHOT,
    /* Pause         67 */ VK_PAUSE,
    /* F1 .. F24  68-91 */ VK_F1,  VK_F2,  VK_F3,  VK_F4,  VK_F5,  VK_F6,
                           VK_F7,  VK_F8,  VK_F9,  VK_F10, VK_F11, VK_F12,
                           VK_F13, VK_F14, VK_F15, VK_F16, VK_F17, VK_F18,
                           VK_F19, VK_F20, VK_F21, VK_F22, VK_F23, VK_F24,
    /* KeyPad0..9 92-101*/ VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4,
                           VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9,
    /* KeyPadDecimal  102 */ VK_DECIMAL,
    /* KeyPadDivide   103 */ VK_DIVIDE,
    /* KeyPadMultiply 104 */ VK_MULTIPLY,
    /* KeyPadSubtract 105 */ VK_SUBTRACT,
    /* KeyPadAdd      106 */ VK_ADD,
    /* KeyPadEnter    107 */ VK_RETURN,   // no separate VK for numpad Enter
    /* KeyPadEqual    108 */ 0,           // no standard VK for numpad =
    /* LeftShift      109 */ VK_LSHIFT,
    /* LeftCtrl       110 */ VK_LCONTROL,
    /* LeftAlt        111 */ VK_LMENU,
    /* LeftSuper      112 */ VK_LWIN,
    /* RightShift     113 */ VK_RSHIFT,
    /* RightCtrl      114 */ VK_RCONTROL,
    /* RightAlt       115 */ VK_RMENU,
    /* RightSuper     116 */ VK_RWIN,
    /* MouseLeft      117 */ VK_LBUTTON,
    /* MouseMiddle    118 */ VK_MBUTTON,
    /* MouseRight     119 */ VK_RBUTTON,
    /* MouseButton4   120 */ VK_XBUTTON1,
    /* MouseButton5   121 */ VK_XBUTTON2,
};
static_assert(std::size(k_KeyCodeToVK) == static_cast<size_t>(KeyCode::k_Count),
              "k_KeyCodeToVK must match KeyCode enum order");

bool Input::Create(void* hWindowHandle)
{
    if (g_Input) return false;
    g_Input = new Input;

    HWND hwnd = reinterpret_cast<HWND>(hWindowHandle);
    g_Input->m_hWindowHandle = hWindowHandle;

    // Register mouse Raw Input device - needed for scroll polling via GetRawInputBuffer
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;  // HID_USAGE_PAGE_GENERIC
    rid.usUsage = 0x02;      // HID_USAGE_GENERIC_MOUSE
    rid.dwFlags = RIDEV_INPUTSINK;  // receive even when window not focused
    rid.hwndTarget = hwnd;

    if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) != FALSE)
    {
        core::DebugOut("[  SGKit Input   ]: module created.");
        return true;
    }
    else
    {
        core::DebugOut("[  SGKit Input   ]: failed to create input module.");
        return false;
    }
}

void Input::Destroy()
{
    if (!g_Input) return;
    delete g_Input;
    g_Input = nullptr;
    core::DebugOut("[  SGKit Input   ]: module destroyed.");
}

Input& Input::instance()
{
    return *g_Input;
}

//  PollScroll - drain buffered Raw Input for wheel data

static void PollScroll(float& scrollDelta)
{
    // Query buffer size
    UINT size = 0;
    UINT count = GetRawInputBuffer(nullptr, &size, sizeof(RAWINPUTHEADER));
    if (count == 0 || count == UINT_MAX) return;

    // Single RAWINPUT for mouse is 48 bytes; a generous stack buffer handles
    // typical per-frame volume without a heap allocation.
    BYTE stackBuf[1024]{};
    BYTE* buf = (size <= sizeof(stackBuf)) ? stackBuf : new BYTE[size];

    count = GetRawInputBuffer(reinterpret_cast<PRAWINPUT>(buf), &size,
                              sizeof(RAWINPUTHEADER));
    if (count != UINT_MAX && count > 0)
    {
        RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(buf);
        for (UINT i = 0; i < count; ++i)
        {
            if (raw[i].header.dwType == RIM_TYPEMOUSE &&
                (raw[i].data.mouse.usButtonFlags & RI_MOUSE_WHEEL))
            {
                short delta = static_cast<short>(raw[i].data.mouse.usButtonData);
                scrollDelta += static_cast<float>(delta) / 120.0f;
            }
        }
    }

    if (buf != stackBuf)
        delete[] buf;
    buf = nullptr;
}

//  Update - called once per frame at the start of the game loop

void Input::Update()
{
    // 1. Swap double-buffer & reset per-frame accumulators
    m_current = 1 - m_current;
    std::memset(m_Keys[m_current], 0, sizeof(m_Keys[m_current]));
    std::memset(m_Mouse[m_current], 0, sizeof(m_Mouse[m_current]));
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
    m_scrollDelta = 0.0f;

    HWND hwnd = static_cast<HWND>(m_hWindowHandle);

    // 2. Poll absolute mouse position & compute frame-to-frame delta
    POINT pt;
    if (GetCursorPos(&pt) && ScreenToClient(hwnd, &pt))
    {
        float prevX = m_mouseX;
        float prevY = m_mouseY;
        m_mouseX = static_cast<float>(pt.x);
        m_mouseY = static_cast<float>(pt.y);
        m_mouseDeltaX = m_mouseX - prevX;
        m_mouseDeltaY = m_mouseY - prevY;
    }

    // 3. Poll mouse buttons via GetAsyncKeyState
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)  m_Mouse[m_current][0] = 1;
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)  m_Mouse[m_current][1] = 1;
    if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)  m_Mouse[m_current][2] = 1;
    if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) m_Mouse[m_current][3] = 1;
    if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) m_Mouse[m_current][4] = 1;

    // 4. Poll keyboard keys via GetAsyncKeyState
    for (size_t i = 1; i < k_KeyCount; ++i)  // skip Unknown
    {
        unsigned int vk = k_KeyCodeToVK[i];
        if (vk != 0 && (GetAsyncKeyState(vk) & 0x8000))
            m_Keys[m_current][i] = 1;
    }

    // 5. Poll scroll via Raw Input buffer
    PollScroll(m_scrollDelta);
}

// -- Keyboard polling

bool Input::IsKeyDown(KeyCode key) const
{
    size_t idx = static_cast<size_t>(key);
    if (idx >= k_KeyCount) return false;
    return m_Keys[m_current][idx] != 0;
}

bool Input::IsKeyPressed(KeyCode key) const
{
    size_t idx = static_cast<size_t>(key);
    if (idx >= k_KeyCount) return false;
    return m_Keys[m_current][idx] != 0 && m_Keys[1 - m_current][idx] == 0;
}

bool Input::IsKeyReleased(KeyCode key) const
{
    size_t idx = static_cast<size_t>(key);
    if (idx >= k_KeyCount) return false;
    return m_Keys[m_current][idx] == 0 && m_Keys[1 - m_current][idx] != 0;
}

// -- Mouse polling

bool Input::IsMouseButtonDown(MouseButton button) const
{
    int bcode = static_cast<int>(button);
    if (bcode < 0 || bcode >= static_cast<int>(k_MouseButtonCount)) return false;
    return m_Mouse[m_current][bcode] != 0;
}

bool Input::IsMouseButtonPressed(MouseButton button) const
{
    int bcode = static_cast<int>(button);
    if (bcode < 0 || bcode >= static_cast<int>(k_MouseButtonCount)) return false;
    return m_Mouse[m_current][bcode] != 0 && m_Mouse[1 - m_current][bcode] == 0;
}

bool Input::IsMouseButtonReleased(MouseButton button) const
{
    int bcode = static_cast<int>(button);
    if (bcode < 0 || bcode >= static_cast<int>(k_MouseButtonCount)) return false;
    return m_Mouse[m_current][bcode] == 0 && m_Mouse[1 - m_current][bcode] != 0;
}

float Input::GetMouseX() const       { return m_mouseX; }
float Input::GetMouseY() const       { return m_mouseY; }
float Input::GetMouseDeltaX() const  { return m_mouseDeltaX; }
float Input::GetMouseDeltaY() const  { return m_mouseDeltaY; }
float Input::GetScrollDelta() const  { return m_scrollDelta; }

}
}

#endif
