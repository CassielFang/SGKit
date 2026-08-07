# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SGKit (Straightforward Graphics Kit) - lightweight C++20 3D engine, OpenGL 4.6 Core. Windows 10+ (architecture allows Linux/macOS).

External dependencies (vendored under `external/`): glad (OpenGL loader), stb_image (texture loading), assimp (model import).

## Build

VS 2022 "Open Folder" with CMake integration: select x64-Debug or x64-Release, build. CMake 3.20+ required.

```
# Run an example (Debug)
out/build/x64-Debug/examples/Light/Debug/SGKitLight.exe
out/build/x64-Debug/examples/AssimpExample/Debug/SGKitAssimp.exe

# Run tests
cd out/build/x64-Debug && ctest -C Debug
```

## Build System (CMake)

- **Library** (`src/`): `sgkit` static library, auto-discovers `.cpp` via `GLOB_RECURSE`. Links assimp statically, copies assimp DLL post-build. Public headers under `include/`.
- **Examples** (`examples/`): each is a subdirectory with its own `CMakeLists.txt`. The shared helper `sgkit_setup_example(NAME)` links `sgkit`, adds app icon, sets debugger working dir to source (so shader edits don't need rebuild), copies assimp DLL and `assets/` to output.
- **Tests** (`tests/`): each `.cpp` becomes an executable auto-registered with `add_test`. Run via `ctest`.

Output paths: libs -> `lib/<Config>/sgkit.lib`, executables -> `out/build/x64-<Config>/`.

## Architecture

5 modules, bottom-up:

```
Framework   (WinMain + game loop + ApplicationConfig lifecycle + Clock/timing)
  ↑
Scene       (sparse-set ECS + Material/Mesh/RenderQueue/Renderer - owns the render pipeline)
  ↑
Graphics    (pure OpenGL RAII wrappers: Shader/VBO/VAO/Texture/Framebuffer/VertexLayout)
  ↑
Core        (Window/Input/FileSystem/ThreadPool)
  ↑
Math        (Vector2/3/4, Matrix4 column-major, Quaternion, MathUtils)
```

All engine modules are **singletons** created/destroyed in strict dependency order by the framework's `Run()`. The user accesses them via `::instance()`:

| Module | Access |
|--------|--------|
| Window | `core::Window::instance()` |
| Input | `core::Input::instance()` |
| Renderer | `scene::Renderer::instance()` |
| Scene | `scene::Scene::instance()` |
| ThreadPool | `core::ThreadPool::instance()` |

Delta time & FPS via static methods: `framework::Clock::GetFrameDeltaSeconds()`, `framework::Clock::GetFPS()`.

### Framework - Application lifecycle

The user defines `sgkit::CreateApplication()` returning an `ApplicationConfig`. The library's `WinMain` calls it, then orchestrates the game loop:

```
onInit -> (onUpdate -> RecomputeWorldTransforms -> onRender -> SwapBuffers) × per-frame -> onShutdown
```

`ApplicationConfig` fields: `title`, `width`, `height`, `resizable`, `vsync`, `fullscreenBolderless`, `fullscreen`, `cursorVisible`, `glMajor`/`glMinor` (default 4.6), `numThreads`, and the four lifecycle lambdas.

### Scene - ECS + Render Pipeline

**ECS** uses sparse-set component pools (`ComponentPool<T>`) for O(1) add/remove/get and efficient linear iteration. Entity is a `uint32_t` handle (max 10,000).

Four component types live in `sgkit::scene::component`:
- **Transform** - position, rotation (quaternion), scale, parent/child hierarchy
- **Camera** - FOV, near/far planes; generates view + projection matrices
- **Light** - Directional/Point/SpotLight, Phong parameters, attenuation
- **MeshRenderer** - shared_ptr to a Mesh, enabled flag

**Render pipeline** each frame (orchestrated by `Scene::Render(cameraEntity)`):
1. `RecomputeWorldTransforms()` - walks hierarchy, caches world matrices
2. `CollectLights()` - gathers light data for all Light entities
3. `BuildRenderQueue()` - iterates MeshRenderer entities, submits Mesh + world matrix
4. `RenderQueue::Sort(cameraPos)` - groups by shader/material/VAO, sorts transparent batches back-to-front
5. `Renderer::Execute(queue)` - two-pass: opaque batches then transparent batches

**Material** (`scene::Material`): shared_ptr to Shader + `LightingModel` enum (BlinnPhong or PBR). Blinn-Phong: diffuse/specular textures + shininess. PBR (Cook-Torrance metallic-roughness): albedo/metallic/roughness/normal/ao textures (slots 0-4), plus `metallicFactor`/`roughnessFactor` multipliers. Also `BlendMode` (Opaque/AlphaBlend/Additive), `CullMode` (Back/Front/None), `DepthMode` (ReadWrite/ReadOnly/None), `renderQueue` integer for manual sort order.

**Mesh** (`scene::Mesh`): shared_ptr to VertexArray + Material.

**Model loading** (`scene::Model::Load(path, blinnPhongShader, pbrShader)`) uses assimp. Auto-detects per-sub-mesh lighting model (PBR via `aiShadingMode_PBR_BRDF` or `AI_MATKEY_METALLIC_FACTOR`, Blinn-Phong otherwise) and assigns the correct shader. Convenience overload `Load(path, shader)` uses the same shader for all meshes. Returns `Model::Result` with a `root` Entity (Transform only - move/scale/hide/destroy the whole model) and `entities` vector (one per sub-mesh, each with Transform + MeshRenderer, parented to root). Supports OBJ, FBX, GLB, glTF, DAE, 3DS, PLY, STL, Blend.

### Graphics - OpenGL RAII wrappers

Pure wrappers, no scene-level concepts. Key classes: `Shader` (compile from file/source, uniform caching), `VertexBuffer`/`IndexBuffer`, `VertexArray` (binds VBO+IBO+layout, issues draw calls), `VertexLayout` (attribute descriptor), `Texture` (stb_image, slot-based binding), `Framebuffer`.

### Math - Column-major

`Matrix4::Data()` returns `float*` suitable for direct `glUniformMatrix4fv` call. Column-major convention matches OpenGL. `Quaternion` for rotations, `Vector2/3/4` with typical operator overloads.

## Conventions

- C++20, `/W4` (MSVC), `-Wall -Wextra -Wpedantic` (GCC/Clang)
- `#pragma once`, Allman braces, 4-space indent
- Namespaces `snake_case`, classes `PascalCase`, members `m_camelCase`
- **PIMPL**: `Window` and `Input` use PIMPL in headers, platform `#ifdef SGK_PLATFORM_WINDOWS` in `.cpp`
- **RAII**: GL objects are move-only; `Create()`/`Destroy()` or ctor/dtor manage lifetimes
- **Debug logging**: `SGK_LOG_INFO/WARN/ERROR/FATAL(category, format, ...)` and `SGK_ASSERT(cond, msg)` from `<sgkit/framework/DebugOut.h>` (available via `sgkit.h`). No-op in Release. Output to both stderr and `OutputDebugStringA`. Not thread-safe.
- **Static CRT**: `/MT` (Release) `/MTd` (Debug) - no VC++ redist needed
- `sgkit.h` is the umbrella header users should include

## Examples

| Example | Description |
|---|---|
| Light | 10 rotating textured cubes with spot light + Blinn-Phong shading |
| AssimpExample | Loads backpack.obj via assimp with Phong shading, WASD+mouse camera |
| PBRExample | PBR material showcase (5×5 roughness×metallic grid) + optional model loading with auto PBR/Blinn-Phong shader selection |

## Add a new example

1. Create `examples/MyExample/main.cpp` defining `sgkit::CreateApplication()`
2. Create `examples/MyExample/CMakeLists.txt`: `add_executable(NAME WIN32 main.cpp)` then `sgkit_setup_example(NAME)`
3. Add `add_subdirectory(MyExample)` to `examples/CMakeLists.txt`
4. Place shaders under `examples/MyExample/assets/shaders/` (the debugger working dir is the source dir, so relative paths work)

## Tests

Two tests exist: `test_math` and `test_threadpool`. Each `.cpp` in `tests/` is auto-discovered and registered. Tests link `sgkit` and use no external test framework - they assert via `SGK_ASSERT` and return 0 on success.
