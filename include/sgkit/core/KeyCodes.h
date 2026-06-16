#pragma once

#include <cstdint>

namespace sgkit {
namespace core {

enum class KeyCode : uint16_t
{
    k_Unknown = 0,

    // Printable
    k_Space, k_Apostrophe, k_Comma, k_Minus, k_Period, k_Slash,
    k_0, k_1, k_2, k_3, k_4, k_5, k_6, k_7, k_8, k_9,
    k_Semicolon, k_Equal,
    k_A, k_B, k_C, k_D, k_E, k_F, k_G, k_H, k_I, k_J,
    k_K, k_L, k_M, k_N, k_O, k_P, k_Q, k_R, k_S, k_T,
    k_U, k_V, k_W, k_X, k_Y, k_Z,
    k_LeftBracket, k_Backslash, k_RightBracket, k_GraveAccent,

    // Control
    k_Escape, k_Enter, k_Tab, k_Backspace, k_Insert, k_Delete,
    k_Right, k_Left, k_Down, k_Up,
    k_PageUp, k_PageDown, k_Home, k_End,
    k_CapsLock, k_ScrollLock, k_NumLock, k_PrintScreen, k_Pause,

    // Function keys
    k_F1, k_F2, k_F3, k_F4, k_F5, k_F6, k_F7, k_F8, k_F9, k_F10,
    k_F11, k_F12, k_F13, k_F14, k_F15, k_F16, k_F17, k_F18, k_F19, k_F20,
    k_F21, k_F22, k_F23, k_F24, k_F25,

    // Numpad
    k_KeyPad0, k_KeyPad1, k_KeyPad2, k_KeyPad3, k_KeyPad4,
    k_KeyPad5, k_KeyPad6, k_KeyPad7, k_KeyPad8, k_KeyPad9,
    k_KeyPadDecimal, k_KeyPadDivide, k_KeyPadMultiply,
    k_KeyPadSubtract, k_KeyPadAdd, k_KeyPadEnter, k_KeyPadEqual,

    // Modifiers
    k_LeftShift, k_LeftCtrl, k_LeftAlt, k_LeftSuper,
    k_RightShift, k_RightCtrl, k_RightAlt, k_RightSuper,

    // Mouse
    k_MouseLeft, k_MouseMiddle, k_MouseRight,
    k_MouseButton4, k_MouseButton5,

    k_Count
};

} // namespace core
} // namespace sgkit
