#pragma once

#include <sgkit/scripting/ScriptHost.h>
#include <sgkit/scripting/NativeApi.gen.h>

#include <string>

namespace sgkit {
namespace scripting {

// Owns the CoreCLR host and drives managed scripts. Singleton, created by the
// framework right after Scene when ApplicationConfig::enableScripting is set.
//
// Per frame the framework calls Update(dt), which scans the scene for Script
// components, lazily instantiates the named C# type on first sight, and calls
// its OnUpdate. Managed scripts reach back into the engine through the
// NativeApi function-pointer table handed over at Bootstrap.
class ScriptEngine
{
public:
    static void Create();
    static void Destroy();
    static ScriptEngine& instance();

    bool IsReady() const;

    // Load a C# assembly (built from user scripts) so its Script subclasses
    // become instantiable by name. `dllFileName` is resolved next to the exe.
    void LoadScriptAssembly(const std::string& dllFileName);

    // Drive every Script component in the scene for this frame.
    void Update(float deltaSeconds);

    // Phase 0 smoke test (kept for diagnostics).
    void SelfTest();

private:
    ScriptEngine() = default;
    ~ScriptEngine() = default;
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    bool ResolveEntryPoints();

    ScriptHost   m_host;
    std::wstring m_exeDir;
    NativeApi    m_api{};

    // Cached managed [UnmanagedCallersOnly] entry points.
    using PingFn      = int  (*)();
    using BootstrapFn = int  (*)(void*);              // Bootstrap(NativeApi*)
    using LoadAsmFn   = int  (*)(const wchar_t*);     // LoadScriptAssembly(utf16 path)
    using CreateFn    = int  (*)(unsigned int, const char*); // CreateScript(entity, utf8 typeName)
    using UpdateFn    = void (*)(int, float);         // UpdateScript(handle, dt)
    using DestroyFn   = void (*)(int);                // DestroyScript(handle)
    using ShutdownFn  = void (*)();                   // Shutdown()

    PingFn      m_ping      = nullptr;
    BootstrapFn m_bootstrap = nullptr;
    LoadAsmFn   m_loadAsm   = nullptr;
    CreateFn    m_create    = nullptr;
    UpdateFn    m_update    = nullptr;
    DestroyFn   m_destroy   = nullptr;
    ShutdownFn  m_shutdown  = nullptr;

    bool m_bootstrapped = false;
};

}
}
