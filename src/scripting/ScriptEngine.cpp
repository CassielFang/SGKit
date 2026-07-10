#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <sgkit/scripting/ScriptEngine.h>
#include <sgkit/scripting/Interop.h>

#include <sgkit/scene/Scene.h>
#include <sgkit/scene/Components.h>
#include <sgkit/framework/DebugOut.h>

#include <string>

namespace sgkit {
namespace scripting {

static ScriptEngine* g_engine = nullptr;

// Managed type that hosts every native-facing entry point.
static const wchar_t* k_BridgeType = L"SGKit.ScriptBridge, SGKit.Managed";

// Directory of the running executable - the managed DLLs, runtimeconfig.json
// and nethost.dll are copied next to the exe by the example's CMake POST_BUILD.
static std::wstring ExeDir()
{
    wchar_t buf[MAX_PATH];
    DWORD n = ::GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring path(buf, n);
    size_t slash = path.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? std::wstring(L".") : path.substr(0, slash);
}

void ScriptEngine::Create()
{
    if (g_engine) return;
    g_engine = new ScriptEngine();

    g_engine->m_exeDir = ExeDir();
    const std::wstring dll = g_engine->m_exeDir + L"\\SGKit.Managed.dll";
    const std::wstring cfg = g_engine->m_exeDir + L"\\SGKit.Managed.runtimeconfig.json";

    if (!g_engine->m_host.Init(cfg, dll))
    {
        SGK_LOG_ERROR("Script", "ScriptEngine init failed - scripting disabled");
        return; // g_engine stays allocated but IsReady() == false
    }

    if (!g_engine->ResolveEntryPoints())
    {
        SGK_LOG_ERROR("Script", "Failed to resolve managed entry points");
        return;
    }

    // Hand the native function-pointer table to the managed side.
    FillNativeApi(g_engine->m_api);
    if (g_engine->m_bootstrap)
    {
        int rc = g_engine->m_bootstrap(&g_engine->m_api);
        g_engine->m_bootstrapped = (rc == 0);
        if (rc != 0)
            SGK_LOG_ERROR("Script", "Managed Bootstrap returned %d", rc);
    }

    SGK_LOG_INFO("Script", "ScriptEngine created");
}

bool ScriptEngine::ResolveEntryPoints()
{
    m_ping      = reinterpret_cast<PingFn>     (m_host.GetFunction(k_BridgeType, L"Ping"));
    m_bootstrap = reinterpret_cast<BootstrapFn>(m_host.GetFunction(k_BridgeType, L"Bootstrap"));
    m_loadAsm   = reinterpret_cast<LoadAsmFn>  (m_host.GetFunction(k_BridgeType, L"LoadScriptAssembly"));
    m_create    = reinterpret_cast<CreateFn>   (m_host.GetFunction(k_BridgeType, L"CreateScript"));
    m_update    = reinterpret_cast<UpdateFn>   (m_host.GetFunction(k_BridgeType, L"UpdateScript"));
    m_destroy   = reinterpret_cast<DestroyFn>  (m_host.GetFunction(k_BridgeType, L"DestroyScript"));
    m_shutdown  = reinterpret_cast<ShutdownFn> (m_host.GetFunction(k_BridgeType, L"Shutdown"));

    return m_bootstrap && m_loadAsm && m_create && m_update && m_destroy;
}

void ScriptEngine::Destroy()
{
    if (!g_engine) return;
    if (g_engine->m_shutdown) g_engine->m_shutdown(); // OnDestroy every live script
    g_engine->m_host.Shutdown();
    delete g_engine;
    g_engine = nullptr;
    SGK_LOG_INFO("Script", "ScriptEngine destroyed");
}

ScriptEngine& ScriptEngine::instance()
{
    return *g_engine;
}

bool ScriptEngine::IsReady() const
{
    return m_host.IsReady() && m_bootstrapped;
}

void ScriptEngine::LoadScriptAssembly(const std::string& dllFileName)
{
    if (!m_loadAsm) return;

    // Resolve relative to the exe directory and widen to UTF-16.
    std::wstring wide(dllFileName.begin(), dllFileName.end()); // ASCII file names
    std::wstring full = m_exeDir + L"\\" + wide;

    int rc = m_loadAsm(full.c_str());
    if (rc != 0)
        SGK_LOG_ERROR("Script", "LoadScriptAssembly(%s) failed: %d", dllFileName.c_str(), rc);
    else
        SGK_LOG_INFO("Script", "Loaded script assembly %s", dllFileName.c_str());
}

void ScriptEngine::Update(float deltaSeconds)
{
    if (!IsReady() || !m_create || !m_update) return;

    scene::Scene& scene = scene::Scene::instance();

    // Copy the entity list: CreateScript could (in future) touch the scene.
    // Cheap for the script counts we expect.
    const std::vector<scene::Entity> ents =
        scene.ComponentEntities<scene::component::Script>();

    for (scene::Entity e : ents)
    {
        scene::component::Script* sc =
            scene.GetComponent<scene::component::Script>(e);
        if (!sc) continue;

        if (sc->handle < 0)
        {
            sc->handle = m_create(e.m_id, sc->typeName.c_str());
            if (sc->handle < 0) continue; // creation failed; skip this frame
        }

        m_update(sc->handle, deltaSeconds);
    }
}

void ScriptEngine::SelfTest()
{
    if (m_ping)
    {
        int r = m_ping();
        SGK_LOG_INFO("Script", "C# Ping() returned %d", r);
    }
    else
    {
        SGK_LOG_WARN("Script", "Ping entry point unavailable");
    }
}

}
}
