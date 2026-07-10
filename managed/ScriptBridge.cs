using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace SGKit
{
    // Native-facing entry points. Every method exposed to the C++ host is static
    // and [UnmanagedCallersOnly] with a blittable signature; the native
    // ScriptHost resolves them by name via
    // load_assembly_and_get_function_pointer.
    public static class ScriptBridge
    {
        private static readonly Dictionary<int, Script> _scripts = new Dictionary<int, Script>();
        private static readonly List<Assembly> _scriptAssemblies = new List<Assembly>();
        private static int _nextHandle = 1;

        // Checkpoint A smoke test.
        [UnmanagedCallersOnly]
        public static int Ping()
        {
            Console.WriteLine("[C#] Hello from SGKit.Managed! Ping received.");
            return 42;
        }

        // Receive the native function-pointer table. Must run before any script
        // touches the engine.
        [UnmanagedCallersOnly]
        public static int Bootstrap(IntPtr apiPtr)
        {
            try
            {
                NativeApi api = Marshal.PtrToStructure<NativeApi>(apiPtr);
                Native.Bind(api);
                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine("[C#] Bootstrap failed: " + ex);
                return -1;
            }
        }

        // Load a user script assembly (UTF-16 path) so its Script subclasses
        // become resolvable by name.
        [UnmanagedCallersOnly]
        public static int LoadScriptAssembly(IntPtr utf16Path)
        {
            try
            {
                string path = Marshal.PtrToStringUni(utf16Path);
                if (string.IsNullOrEmpty(path)) return -1;

                // Load into the SAME context that hosts SGKit.Managed. The
                // native host loads us into an isolated component ALC (not
                // Default); loading user scripts into Default would give
                // SGKit.Managed a second identity, so Script/base-type
                // resolution and casts would silently fail.
                AssemblyLoadContext alc =
                    AssemblyLoadContext.GetLoadContext(typeof(ScriptBridge).Assembly)
                    ?? AssemblyLoadContext.Default;

                Assembly asm = alc.LoadFromAssemblyPath(path);
                _scriptAssemblies.Add(asm);
                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine("[C#] LoadScriptAssembly failed: " + ex.Message);
                return -1;
            }
        }

        // Instantiate a Script subclass by name, bind it to the entity, run
        // OnCreate. Returns a handle used by UpdateScript/DestroyScript, or -1.
        [UnmanagedCallersOnly]
        public static unsafe int CreateScript(uint entity, byte* typeNameUtf8)
        {
            string typeName = Marshal.PtrToStringUTF8((IntPtr)typeNameUtf8);
            if (string.IsNullOrEmpty(typeName)) return -1;

            Type type = FindType(typeName);
            if (type == null)
            {
                Console.WriteLine("[C#] Script type not found: " + typeName);
                return -1;
            }

            try
            {
                Script s = (Script)Activator.CreateInstance(type);
                s.Entity = entity;
                int handle = _nextHandle++;
                _scripts[handle] = s;
                s.OnCreate();
                return handle;
            }
            catch (Exception ex)
            {
                Console.WriteLine("[C#] CreateScript(" + typeName + ") failed: " + ex);
                return -1;
            }
        }

        [UnmanagedCallersOnly]
        public static void UpdateScript(int handle, float dt)
        {
            if (_scripts.TryGetValue(handle, out Script s))
            {
                try { s.OnUpdate(dt); }
                catch (Exception ex) { Console.WriteLine("[C#] OnUpdate error: " + ex.Message); }
            }
        }

        [UnmanagedCallersOnly]
        public static void DestroyScript(int handle)
        {
            if (_scripts.TryGetValue(handle, out Script s))
            {
                try { s.OnDestroy(); } catch { }
                _scripts.Remove(handle);
            }
        }

        // Called by the native ScriptEngine on shutdown: OnDestroy every live
        // script, then clear.
        [UnmanagedCallersOnly]
        public static void Shutdown()
        {
            foreach (KeyValuePair<int, Script> kv in _scripts)
            {
                try { kv.Value.OnDestroy(); } catch { }
            }
            _scripts.Clear();
            _scriptAssemblies.Clear();
        }

        private static Type FindType(string name)
        {
            foreach (Assembly asm in _scriptAssemblies)
            {
                // Fast path: exact (namespace-qualified) name.
                Type t = asm.GetType(name);
                if (t != null) return t;

                // Fallback: scan all types, match simple Name or FullName. This
                // also surfaces loader errors that GetType() swallows.
                try
                {
                    foreach (Type candidate in asm.GetTypes())
                    {
                        if (candidate.Name == name || candidate.FullName == name)
                            return candidate;
                    }
                }
                catch (ReflectionTypeLoadException ex)
                {
                    foreach (Exception le in ex.LoaderExceptions)
                        Console.WriteLine("[C#]   loader error: " + le.Message);
                }
            }

            // Last resort: any already-loaded assembly (incl. SGKit.Managed).
            return Type.GetType(name);
        }
    }
}
