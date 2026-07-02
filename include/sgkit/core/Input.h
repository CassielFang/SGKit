#pragma once

#include <sgkit/core/KeyCodes.h>

#include <array>
#include <cstdint>

namespace sgkit {
namespace core {

class Input
{
public:
    static bool Create(void*);
    static void Destroy();
    static Input& instance();

    // Call once per frame at the start of the game loop
    void Update();

    // -- Keyboard polling
    bool IsKeyDown(KeyCode key) const;
    bool IsKeyPressed(KeyCode key) const;   // transition: up->down this frame
    bool IsKeyReleased(KeyCode key) const;  // transition: down->up this frame

    // -- Mouse polling
    bool  IsMouseButtonDown(MouseButton button) const;
    bool  IsMouseButtonPressed(MouseButton button) const;
    bool  IsMouseButtonReleased(MouseButton button) const;

    float GetMouseX() const;
    float GetMouseY() const;
    float GetMouseDeltaX() const;
    float GetMouseDeltaY() const;
    float GetScrollDelta() const; // positive when scrolling forward

private:
    Input() = default;
    ~Input() = default;

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    Input(const Input&&) = delete;
    Input& operator=(const Input&&) = delete;

    static constexpr size_t k_KeyCount = static_cast<size_t>(KeyCode::k_Count);
    static constexpr size_t k_MouseButtonCount = 5;

    int m_current = 0;
    uint8_t m_Keys[2][k_KeyCount]{};
    uint8_t m_Mouse[2][k_MouseButtonCount]{};

    float m_mouseX        = 0.0f;
    float m_mouseY        = 0.0f;
    float m_mouseDeltaX   = 0.0f;
    float m_mouseDeltaY   = 0.0f;
    float m_scrollDelta   = 0.0f;

    void* m_hWindowHandle = nullptr;
};

}
}
