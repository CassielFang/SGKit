#pragma once

#include <string>

namespace sgkit {
namespace scripting {

// Bootstraps the CoreCLR runtime via nethost/hostfxr and resolves managed
// [UnmanagedCallersOnly] entry points by (type, method) name.
//
// This header stays free of any hostfxr/nethost includes on purpose: all
// runtime handles are held as opaque void* so consumers of the public API
// never need the vendored .NET host headers on their include path. The
// actual hosting machinery lives entirely in ScriptHost.cpp.
//
// Windows-only for now (uses LoadLibraryW under the hood).
class ScriptHost
{
public:
    // Boot the runtime described by runtimeConfigPath (the managed assembly's
    // *.runtimeconfig.json) and remember managedAssemblyPath as the assembly
    // that GetFunction() resolves methods from. Returns false on any failure;
    // the host is then simply "not ready" and the engine runs without scripts.
    bool Init(const std::wstring& runtimeConfigPath,
              const std::wstring& managedAssemblyPath);

    void Shutdown();

    // Resolve a managed static [UnmanagedCallersOnly] method to a raw,
    // native-callable function pointer. typeName is the assembly-qualified
    // type ("Namespace.Type, AssemblyName"), methodName the method name.
    // Returns nullptr on failure.
    void* GetFunction(const wchar_t* typeName, const wchar_t* methodName);

    bool IsReady() const { return m_ready; }

private:
    std::wstring m_assemblyPath;
    void*        m_lagfp    = nullptr; // load_assembly_and_get_function_pointer_fn
    void*        m_ctx      = nullptr; // hostfxr_handle
    void*        m_hostfxr  = nullptr; // HMODULE of hostfxr.dll
    bool         m_ready    = false;
};

}
}
