#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <sgkit/scripting/ScriptHost.h>
#include <sgkit/framework/DebugOut.h>

// Vendored .NET hosting headers (external/dotnet/include, PRIVATE to sgkit).
#include <nethost.h>
#include <coreclr_delegates.h>
#include <hostfxr.h>

namespace sgkit {
namespace scripting {

namespace {

// hostfxr exports, resolved once from the loaded hostfxr.dll.
hostfxr_initialize_for_runtime_config_fn  s_init   = nullptr;
hostfxr_get_runtime_delegate_fn           s_getDel = nullptr;
hostfxr_close_fn                          s_close  = nullptr;

} // namespace

bool ScriptHost::Init(const std::wstring& runtimeConfigPath,
                      const std::wstring& managedAssemblyPath)
{
    m_assemblyPath = managedAssemblyPath;

    // 1. Locate hostfxr.dll for the installed .NET runtime.
    wchar_t hostfxrPath[MAX_PATH];
    size_t  bufSize = MAX_PATH;
    int rc = get_hostfxr_path(hostfxrPath, &bufSize, nullptr);
    if (rc != 0)
    {
        SGK_LOG_ERROR("Script", "get_hostfxr_path failed: 0x%x", rc);
        return false;
    }

    // 2. Load hostfxr and resolve the three exports we need.
    HMODULE lib = ::LoadLibraryW(hostfxrPath);
    if (!lib)
    {
        SGK_LOG_ERROR("Script", "LoadLibrary(hostfxr) failed");
        return false;
    }
    m_hostfxr = lib;

    s_init = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
        ::GetProcAddress(lib, "hostfxr_initialize_for_runtime_config"));
    s_getDel = reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
        ::GetProcAddress(lib, "hostfxr_get_runtime_delegate"));
    s_close = reinterpret_cast<hostfxr_close_fn>(
        ::GetProcAddress(lib, "hostfxr_close"));

    if (!s_init || !s_getDel || !s_close)
    {
        SGK_LOG_ERROR("Script", "hostfxr exports not found");
        return false;
    }

    // 3. Initialize the runtime from the managed assembly's runtimeconfig.json.
    hostfxr_handle ctx = nullptr;
    rc = s_init(runtimeConfigPath.c_str(), nullptr, &ctx);
    // 0 = Success, 1 = Success_HostAlreadyInitialized,
    // 2 = Success_DifferentRuntimeProperties. Anything else is a real failure.
    if ((rc != 0 && rc != 1 && rc != 2) || ctx == nullptr)
    {
        SGK_LOG_ERROR("Script", "hostfxr_initialize failed: 0x%x", rc);
        if (ctx) s_close(ctx);
        return false;
    }
    m_ctx = ctx;

    // 4. Ask for the load_assembly_and_get_function_pointer delegate.
    void* lagfp = nullptr;
    rc = s_getDel(ctx, hdt_load_assembly_and_get_function_pointer, &lagfp);
    if (rc != 0 || lagfp == nullptr)
    {
        SGK_LOG_ERROR("Script", "get_runtime_delegate failed: 0x%x", rc);
        return false;
    }
    m_lagfp = lagfp;
    m_ready = true;

    SGK_LOG_INFO("Script", "CoreCLR host initialized");
    return true;
}

void* ScriptHost::GetFunction(const wchar_t* typeName, const wchar_t* methodName)
{
    if (!m_ready) return nullptr;

    auto lagfp = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(m_lagfp);

    void* fn = nullptr;
    int rc = lagfp(
        m_assemblyPath.c_str(),
        typeName,
        methodName,
        UNMANAGEDCALLERSONLY_METHOD, // method is [UnmanagedCallersOnly]
        nullptr,
        &fn);

    if (rc != 0 || fn == nullptr)
    {
        SGK_LOG_ERROR("Script", "GetFunction(%ls) failed: 0x%x", methodName, rc);
        return nullptr;
    }
    return fn;
}

void ScriptHost::Shutdown()
{
    if (m_ctx && s_close)
    {
        s_close(static_cast<hostfxr_handle>(m_ctx));
        m_ctx = nullptr;
    }
    m_lagfp = nullptr;
    m_ready = false;
    // hostfxr.dll is intentionally left loaded: CoreCLR cannot be fully
    // unloaded/reloaded within a process, so there is nothing to gain by
    // FreeLibrary here and doing so risks destabilizing a second Init().
}

}
}
