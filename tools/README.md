# tools/

## generate_bindings.py

Generates both sides of the C#/C++ scripting interop table from a single source
of truth, `include/sgkit/scripting/Interop.h`:

| Generated file | Contents |
|---|---|
| `include/sgkit/scripting/NativeApi.gen.h` | `struct NativeApi` (function-pointer table) + inline `FillNativeApi()` |
| `managed/SGKit.Managed/Generated/Bindings.gen.cs` | matching `NativeApi` struct + typed `Native` wrappers |

### Why

SGKit is a static lib linked into the exe, so the engine singletons live in the
exe and managed scripts must call native code through a function-pointer table
(not a P/Invoke DLL, which would duplicate those singletons). That means three
things have to stay byte-for-byte in sync: the C++ struct, the C++ fill
function, and the C# struct. This tool derives all three from `Interop.h` so
they can't drift.

### Run it

Whenever you add/change a function in `Interop.h`:

```bash
python tools/generate_bindings.py
```

Or build the **SGKitBindings** target in Visual Studio.

The generated files are committed to the repo, so a normal build never needs
Python — only re-run this after editing the interop surface.

### Interop.h conventions the parser understands

- Exported functions live in the `extern "C"` block, one per line, prefixed
  `SGK_`, using only: `void`, `int`, `unsigned int`, `float`, `const char*`,
  and pointers to the POD structs declared in the same header.
- `const T*` is an **input** → the C# wrapper takes `T` by value and passes its
  address.
- non-const `T*` is an **output** → becomes the C# wrapper's return value (the
  native function must return `void`; at most one such parameter).
- `const char*` is a UTF-8 input string.
- The C# method name is the function name with the `SGK_` prefix stripped.
