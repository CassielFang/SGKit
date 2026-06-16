#pragma once

#include <functional>
#include <string>

namespace sgkit {

namespace core     { class Window; class Input; }
namespace graphics { class Renderer; }
namespace scene   { class Scene; }

struct ApplicationConfig
{
    std::string title = "SGKit";
    int  width       = 1280;
    int  height      = 720;
    bool vsync       = true;
    bool fullscreen  = false;
    int  glMajor     = 4;
    int  glMinor     = 6;

    std::function<bool()>           onInit;
    std::function<void(float dt)>   onUpdate;
    std::function<void()>           onRender;
    std::function<void()>           onShutdown;
};

// User must define this in their own code.
// The engine's WinMain() calls it, then hands the config to Run().
ApplicationConfig CreateApplication();

// Called by the engine's platform entry point after CreateApplication().
int Run(const ApplicationConfig& config);

// Engine module access — callable from user callbacks.
core::Window&        GetWindow();
core::Input&         GetInput();
graphics::Renderer&  GetRenderer();
scene::Scene&        GetScene();

float GetDeltaTime();
float GetFPS();

void RequestQuit();

} // namespace sgkit
