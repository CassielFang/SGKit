#pragma once

#include <functional>
#include <memory>
#include <string>

namespace sgkit {
namespace core {

struct WindowDesc
{
    std::string title          = "SGKit";
    int width                  = 1280;
    int height                 = 720;
    bool resizable             = true;
    bool vsync                 = true;
    bool fullscreen            = false;
    int glMajorVersion         = 4;
    int glMinorVersion         = 6;
};

class Window
{
public:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    bool Create(const WindowDesc& desc);
    void Destroy();
    bool IsCreated() const;

    void PollEvents();
    bool IsRunning() const;
    void RequestClose();

    void SwapBuffers();

    void Maximize();
    void Minimize();
    void Restore();

    void SetFullscreen(bool enabled);
    bool IsFullscreen() const;

    void SetIMEEnabled(bool enabled);

    bool HasResized() const;
    void ResetResizeFlag();

    int GetWidth() const;
    int GetHeight() const;
    float GetAspectRatio() const;

    void* GetNativeHandle() const;

    // Event handler hook for Input module.
    // Returns true if the message was handled.
    using EventHandler = std::function<bool(unsigned int msg, unsigned long long wParam, long long lParam)>;
    void AddEventHandler(EventHandler handler);

    // Called by the platform window procedure. Returns true if handled.
    bool HandleWindowMessage(unsigned int msg, unsigned long long wParam, long long lParam);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace core
} // namespace sgkit
