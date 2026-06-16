#pragma once

#include <sgkit/core/KeyCodes.h>

#include <array>
#include <cstdint>

namespace sgkit {
namespace core {

class Input
{
public:
    Input();
    ~Input();

    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;

    bool Initialize(void* nativeWindowHandle);
    void Shutdown();

    // Called by Window's event system
    bool OnEvent(unsigned int msg, unsigned long long wParam, long long lParam);

    // Must be called once per frame after all input processing
    void EndFrame();

    // -- Keyboard polling ----------------------------------------
    bool IsKeyDown(KeyCode key) const;
    bool IsKeyPressed(KeyCode key) const;   // transition: up→down this frame
    bool IsKeyReleased(KeyCode key) const;  // transition: down→up this frame

    // -- Mouse polling -------------------------------------------
    bool  IsMouseButtonDown(int button) const;
    bool  IsMouseButtonPressed(int button) const;
    bool  IsMouseButtonReleased(int button) const;

    float GetMouseX() const;
    float GetMouseY() const;
    float GetMouseDeltaX() const;
    float GetMouseDeltaY() const;
    float GetScrollDelta() const;

private:
    bool RegisterRawInput(void* hwnd);

    static constexpr size_t k_KeyCount = static_cast<size_t>(KeyCode::k_Count);
    static constexpr size_t k_MouseButtonCount = 5;

    std::array<uint8_t, k_KeyCount>      m_currentKeys{};
    std::array<uint8_t, k_KeyCount>      m_previousKeys{};
    std::array<uint8_t, k_MouseButtonCount> m_currentMouse{};
    std::array<uint8_t, k_MouseButtonCount> m_previousMouse{};

    float m_mouseX        = 0.0f;
    float m_mouseY        = 0.0f;
    float m_mouseDeltaX   = 0.0f;
    float m_mouseDeltaY   = 0.0f;
    float m_scrollDelta   = 0.0f;

    bool  m_initialized   = false;

    // Platform-specific helpers
    KeyCode MapWin32Key(unsigned int vkCode) const;
};

} // namespace core
} // namespace sgkit
