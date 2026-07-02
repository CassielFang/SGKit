#pragma once

#include <cstdint>

namespace sgkit {
namespace core {

enum class MouseButton : uint16_t
{
    Left = 0, Right, Middle, Back, Forward
};

enum class KeyCode : uint16_t
{
    Unknown = 0,

    // Printable
    Space, Apostrophe, Comma, Minus, Period, Slash,
    k_0, k_1, k_2, k_3, k_4, k_5, k_6, k_7, k_8, k_9,
    Semicolon, Equal,
    A, B, C, D, E, F, G, H, I, J,
    K, L, M, N, O, P, Q, R, S, T,
    U, V, W, X, Y, Z,
    LeftBracket, Backslash, RightBracket, GraveAccent,

    // Control
    Escape, Enter, Tab, Backspace, Insert, Delete,
    Right, Left, Down, Up,
    PageUp, PageDown, Home, End,
    CapsLock, ScrollLock, NumLock, PrintScreen, Pause,

    // Function keys
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10,
    F11, F12, F13, F14, F15, F16, F17, F18, F19, F20,
    F21, F22, F23, F24,

    // Numpad
    KeyPad0, KeyPad1, KeyPad2, KeyPad3, KeyPad4,
    KeyPad5, KeyPad6, KeyPad7, KeyPad8, KeyPad9,
    KeyPadDecimal, KeyPadDivide, KeyPadMultiply,
    KeyPadSubtract, KeyPadAdd, KeyPadEnter, KeyPadEqual,

    // Modifiers
    LeftShift, LeftCtrl, LeftAlt, LeftSuper,
    RightShift, RightCtrl, RightAlt, RightSuper,

    // Mouse
    MouseLeft, MouseMiddle, MouseRight,
    MouseButton4, MouseButton5,

    k_Count
};

}
}
