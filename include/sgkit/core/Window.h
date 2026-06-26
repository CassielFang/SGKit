#pragma once

#include <functional>
#include <memory>
#include <string>

namespace sgkit {
namespace core {

struct WindowDesc
{
    std::string title            = "SGKit";
    int width                    = 1280;
    int height                   = 720;
    bool resizable               = true;
    bool vsync                   = true;
    bool fullscreenBolderless    = false;
    bool fullscreen              = false;
    bool cursorVisible           = true;
    int glMajorVersion           = 4;
    int glMinorVersion           = 6;
};

class Window
{
public:
    static bool Create(void*, const WindowDesc& desc);
    static void Destroy();
    static Window& instance();

    void PollEvents();
    void RequestClose(bool request = true);
    bool IsCloseRequest() const;

    void SwapBuffers();

    void Maximize();
    void Minimize();
    void Restore();
    bool isActive() const;
    void SetFullscreen(bool enabled);
    bool IsFullscreen() const;

    void SetCursorVisible(bool enabled);
    bool isCursorVisible() const;

    int GetWidth() const;
    int GetHeight() const;
    float GetAspectRatio() const;

    void* GetNativeHandle() const;

    // Event handler hook for Input module.
    // Returns true if the message was handled.
    using EventHandler = std::function<void(unsigned int msg, unsigned long long wParam, long long lParam)>;
    void AddEventHandler(EventHandler handler);

    // Called by the platform window procedure. Returns true if handled.
    int64_t HandleWindowMessage(unsigned int msg, unsigned long long wParam, long long lParam);

private:
    Window();
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace core
} // namespace sgkit
