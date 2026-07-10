#pragma once

#include <functional>
#include <string>

namespace sgkit {

struct ApplicationConfig
{
    std::string title                = "SGKit";
    int         width                = 1280;
    int         height               = 720;
    bool        resizable            = true;
    bool        vsync                = true;
    bool        fullscreenBolderless = false;
    bool        fullscreen           = false;
    bool        cursorVisible        = true;
    int         glMajor              = 4;
    int         glMinor              = 6;
    size_t      numThreads           = 4;

    // When true the framework boots the CoreCLR host (ScriptEngine) right after
    // the Scene, so onInit and the game loop can drive C# scripts. Leave false
    // for apps with no managed scripts to skip runtime startup cost.
    bool        enableScripting      = false;

    std::function<bool()> onInit;
    std::function<void()> onUpdate;
    std::function<void()> onRender;
    std::function<void()> onShutdown;
};

// User must define this in their own code.
// The engine's WinMain() calls it, then hands the config to Run().
ApplicationConfig CreateApplication();

}
