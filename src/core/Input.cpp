#include <sgkit/core/Input.h>

#ifdef SGK_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

namespace sgkit {
namespace core {

// ===================================================================
//  Key mapping: Win32 VK → SGKit KeyCode
// ===================================================================

KeyCode Input::MapWin32Key(unsigned int vkCode) const
{
    // clang-format off
    switch (vkCode)
    {
    case VK_SPACE:   return KeyCode::k_Space;
    case VK_OEM_COMMA: return KeyCode::k_Comma;
    case VK_OEM_MINUS: return KeyCode::k_Minus;
    case VK_OEM_PERIOD: return KeyCode::k_Period;
    case VK_OEM_2:   return KeyCode::k_Slash;     // / ?
    case '0': return KeyCode::k_0;
    case '1': return KeyCode::k_1;
    case '2': return KeyCode::k_2;
    case '3': return KeyCode::k_3;
    case '4': return KeyCode::k_4;
    case '5': return KeyCode::k_5;
    case '6': return KeyCode::k_6;
    case '7': return KeyCode::k_7;
    case '8': return KeyCode::k_8;
    case '9': return KeyCode::k_9;
    case VK_OEM_1:   return KeyCode::k_Semicolon; // ; :
    case VK_OEM_PLUS: return KeyCode::k_Equal;     // = +
    case 'A': return KeyCode::k_A; case 'B': return KeyCode::k_B;
    case 'C': return KeyCode::k_C; case 'D': return KeyCode::k_D;
    case 'E': return KeyCode::k_E; case 'F': return KeyCode::k_F;
    case 'G': return KeyCode::k_G; case 'H': return KeyCode::k_H;
    case 'I': return KeyCode::k_I; case 'J': return KeyCode::k_J;
    case 'K': return KeyCode::k_K; case 'L': return KeyCode::k_L;
    case 'M': return KeyCode::k_M; case 'N': return KeyCode::k_N;
    case 'O': return KeyCode::k_O; case 'P': return KeyCode::k_P;
    case 'Q': return KeyCode::k_Q; case 'R': return KeyCode::k_R;
    case 'S': return KeyCode::k_S; case 'T': return KeyCode::k_T;
    case 'U': return KeyCode::k_U; case 'V': return KeyCode::k_V;
    case 'W': return KeyCode::k_W; case 'X': return KeyCode::k_X;
    case 'Y': return KeyCode::k_Y; case 'Z': return KeyCode::k_Z;
    case VK_OEM_4:   return KeyCode::k_LeftBracket;
    case VK_OEM_5:   return KeyCode::k_Backslash;
    case VK_OEM_6:   return KeyCode::k_RightBracket;
    case VK_OEM_3:   return KeyCode::k_GraveAccent;

    case VK_ESCAPE:  return KeyCode::k_Escape;
    case VK_RETURN:  return KeyCode::k_Enter;
    case VK_TAB:     return KeyCode::k_Tab;
    case VK_BACK:    return KeyCode::k_Backspace;
    case VK_INSERT:  return KeyCode::k_Insert;
    case VK_DELETE:  return KeyCode::k_Delete;
    case VK_RIGHT:   return KeyCode::k_Right;
    case VK_LEFT:    return KeyCode::k_Left;
    case VK_DOWN:    return KeyCode::k_Down;
    case VK_UP:      return KeyCode::k_Up;
    case VK_PRIOR:   return KeyCode::k_PageUp;
    case VK_NEXT:    return KeyCode::k_PageDown;
    case VK_HOME:    return KeyCode::k_Home;
    case VK_END:     return KeyCode::k_End;
    case VK_CAPITAL: return KeyCode::k_CapsLock;
    case VK_SCROLL:  return KeyCode::k_ScrollLock;
    case VK_NUMLOCK: return KeyCode::k_NumLock;
    case VK_SNAPSHOT:return KeyCode::k_PrintScreen;
    case VK_PAUSE:   return KeyCode::k_Pause;

    case VK_F1: case VK_F2: case VK_F3: case VK_F4: case VK_F5:
    case VK_F6: case VK_F7: case VK_F8: case VK_F9: case VK_F10:
    case VK_F11: case VK_F12:
        return static_cast<KeyCode>(
            static_cast<uint16_t>(KeyCode::k_F1) + (vkCode - VK_F1));

    case VK_F13: return KeyCode::k_F13; case VK_F14: return KeyCode::k_F14;
    case VK_F15: return KeyCode::k_F15; case VK_F16: return KeyCode::k_F16;
    case VK_F17: return KeyCode::k_F17; case VK_F18: return KeyCode::k_F18;
    case VK_F19: return KeyCode::k_F19; case VK_F20: return KeyCode::k_F20;
    case VK_F21: return KeyCode::k_F21; case VK_F22: return KeyCode::k_F22;
    case VK_F23: return KeyCode::k_F23; case VK_F24: return KeyCode::k_F24;

    case VK_LSHIFT:  return KeyCode::k_LeftShift;
    case VK_LCONTROL:return KeyCode::k_LeftCtrl;
    case VK_LMENU:   return KeyCode::k_LeftAlt;
    case VK_LWIN:    return KeyCode::k_LeftSuper;
    case VK_RSHIFT:  return KeyCode::k_RightShift;
    case VK_RCONTROL:return KeyCode::k_RightCtrl;
    case VK_RMENU:   return KeyCode::k_RightAlt;
    case VK_RWIN:    return KeyCode::k_RightSuper;

    case VK_DECIMAL:  return KeyCode::k_KeyPadDecimal;
    case VK_DIVIDE:   return KeyCode::k_KeyPadDivide;
    case VK_MULTIPLY: return KeyCode::k_KeyPadMultiply;
    case VK_SUBTRACT: return KeyCode::k_KeyPadSubtract;
    case VK_ADD:      return KeyCode::k_KeyPadAdd;
    case VK_NUMPAD0:  return KeyCode::k_KeyPad0;
    case VK_NUMPAD1:  return KeyCode::k_KeyPad1;
    case VK_NUMPAD2:  return KeyCode::k_KeyPad2;
    case VK_NUMPAD3:  return KeyCode::k_KeyPad3;
    case VK_NUMPAD4:  return KeyCode::k_KeyPad4;
    case VK_NUMPAD5:  return KeyCode::k_KeyPad5;
    case VK_NUMPAD6:  return KeyCode::k_KeyPad6;
    case VK_NUMPAD7:  return KeyCode::k_KeyPad7;
    case VK_NUMPAD8:  return KeyCode::k_KeyPad8;
    case VK_NUMPAD9:  return KeyCode::k_KeyPad9;

    case VK_LBUTTON:  return KeyCode::k_MouseLeft;
    case VK_MBUTTON:  return KeyCode::k_MouseMiddle;
    case VK_RBUTTON:  return KeyCode::k_MouseRight;
    case VK_XBUTTON1: return KeyCode::k_MouseButton4;
    case VK_XBUTTON2: return KeyCode::k_MouseButton5;

    default: return KeyCode::k_Unknown;
    }
    // clang-format on
}

// ===================================================================

Input::Input() {}
Input::~Input() { Shutdown(); }

bool Input::Initialize(void* nativeWindowHandle)
{
    if (!nativeWindowHandle)
        return false;

    RegisterRawInput(nativeWindowHandle);
    m_initialized = true;
    return true;
}

void Input::Shutdown()
{
    m_initialized = false;
}

bool Input::RegisterRawInput(void* hwnd)
{
    // Register Raw Input for mouse (relative movement)
    RAWINPUTDEVICE rid = {};
    rid.usUsagePage = 0x01;  // HID_USAGE_PAGE_GENERIC
    rid.usUsage     = 0x02;  // HID_USAGE_GENERIC_MOUSE
    rid.dwFlags     = RIDEV_INPUTSINK;  // receive even when window not focused
    rid.hwndTarget  = static_cast<HWND>(hwnd);

    return RegisterRawInputDevices(&rid, 1, sizeof(rid)) != FALSE;
}

bool Input::OnEvent(unsigned int msg, unsigned long long wParam, long long lParam)
{
    switch (msg)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        KeyCode key = MapWin32Key(static_cast<unsigned int>(wParam));
        if (key != KeyCode::k_Unknown)
        {
            size_t idx = static_cast<size_t>(key);
            if (idx < k_KeyCount)
                m_currentKeys[idx] = 1;
        }
        return true;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        KeyCode key = MapWin32Key(static_cast<unsigned int>(wParam));
        if (key != KeyCode::k_Unknown)
        {
            size_t idx = static_cast<size_t>(key);
            if (idx < k_KeyCount)
                m_currentKeys[idx] = 0;
        }
        return true;
    }

    case WM_LBUTTONDOWN:
        m_currentMouse[0] = 1; return true;
    case WM_LBUTTONUP:
        m_currentMouse[0] = 0; return true;
    case WM_RBUTTONDOWN:
        m_currentMouse[1] = 1; return true;
    case WM_RBUTTONUP:
        m_currentMouse[1] = 0; return true;
    case WM_MBUTTONDOWN:
        m_currentMouse[2] = 1; return true;
    case WM_MBUTTONUP:
        m_currentMouse[2] = 0; return true;
    case WM_XBUTTONDOWN:
        m_currentMouse[GET_XBUTTON_WPARAM(wParam) == 1 ? 3 : 4] = 1;
        return true;
    case WM_XBUTTONUP:
        m_currentMouse[GET_XBUTTON_WPARAM(wParam) == 1 ? 3 : 4] = 0;
        return true;

    case WM_MOUSEMOVE:
    {
        float newX = static_cast<float>(GET_X_LPARAM(lParam));
        float newY = static_cast<float>(GET_Y_LPARAM(lParam));
        m_mouseDeltaX = newX - m_mouseX;
        m_mouseDeltaY = newY - m_mouseY;
        m_mouseX = newX;
        m_mouseY = newY;
        return false;  // don't consume, let window handle it
    }

    case WM_MOUSEWHEEL:
    {
        m_scrollDelta += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / 120.0f;
        return true;
    }

    case WM_INPUT:
    {
        // Raw Input — for high-precision mouse delta
        UINT size = 0;
        GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
                        nullptr, &size, sizeof(RAWINPUTHEADER));
        if (size == 0)
            return false;

        RAWINPUT raw;
        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
                            &raw, &size, sizeof(RAWINPUTHEADER)) != size)
            return false;

        if (raw.header.dwType == RIM_TYPEMOUSE)
        {
            m_mouseDeltaX += static_cast<float>(raw.data.mouse.lLastX);
            m_mouseDeltaY += static_cast<float>(raw.data.mouse.lLastY);
        }
        return true;
    }

    default:
        return false;
    }
}

void Input::EndFrame()
{
    m_previousKeys   = m_currentKeys;
    m_previousMouse  = m_currentMouse;
    m_mouseDeltaX    = 0.0f;
    m_mouseDeltaY    = 0.0f;
    m_scrollDelta    = 0.0f;
}

// -- Keyboard polling -----------------------------------------------

bool Input::IsKeyDown(KeyCode key) const
{
    size_t idx = static_cast<size_t>(key);
    if (idx >= k_KeyCount) return false;
    return m_currentKeys[idx] != 0;
}

bool Input::IsKeyPressed(KeyCode key) const
{
    size_t idx = static_cast<size_t>(key);
    if (idx >= k_KeyCount) return false;
    return m_currentKeys[idx] != 0 && m_previousKeys[idx] == 0;
}

bool Input::IsKeyReleased(KeyCode key) const
{
    size_t idx = static_cast<size_t>(key);
    if (idx >= k_KeyCount) return false;
    return m_currentKeys[idx] == 0 && m_previousKeys[idx] != 0;
}

// -- Mouse polling --------------------------------------------------

bool Input::IsMouseButtonDown(int button) const
{
    if (button < 0 || button >= static_cast<int>(k_MouseButtonCount)) return false;
    return m_currentMouse[button] != 0;
}

bool Input::IsMouseButtonPressed(int button) const
{
    if (button < 0 || button >= static_cast<int>(k_MouseButtonCount)) return false;
    return m_currentMouse[button] != 0 && m_previousMouse[button] == 0;
}

bool Input::IsMouseButtonReleased(int button) const
{
    if (button < 0 || button >= static_cast<int>(k_MouseButtonCount)) return false;
    return m_currentMouse[button] == 0 && m_previousMouse[button] != 0;
}

float Input::GetMouseX() const       { return m_mouseX; }
float Input::GetMouseY() const       { return m_mouseY; }
float Input::GetMouseDeltaX() const  { return m_mouseDeltaX; }
float Input::GetMouseDeltaY() const  { return m_mouseDeltaY; }
float Input::GetScrollDelta() const  { return m_scrollDelta; }

} // namespace core
} // namespace sgkit

#endif // SGK_PLATFORM_WINDOWS
