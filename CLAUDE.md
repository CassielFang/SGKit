# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SGKit (Straightforward Graphics Kit) - lightweight C++20 3D engine, OpenGL 4.6 Core. External dependencies: glad (OpenGL loader) and stb_image (texture loading). Target Windows 10+, architecture allows Linux/macOS.

## Build

```bash
# VS 2022 or 2026 "Open Folder" - select x64-Debug / x64-Release and Build

# Run example (Debug)
out/build/x64-Debug/examples/Light/Debug/Light.exe

# Run tests
cd out/build/x64-Debug && ctest -C Debug
```

## Architecture

5 modules, bottom-up dependencies:

```
Framework (WinMain + game loop + init/shutdown orchestration, hidden in library)
  |
Scene (sparse-set ECS + Material/Mesh/RenderQueue/Renderer - owns the render pipeline)
  |
Graphics (pure OpenGL RAII wrappers: Shader/VBO/VAO/Texture/Framebuffer/VertexLayout)
  |
Core (Window/Input/FileSystem/ThreadPool)
  |
Math (Vector2/3/4, Matrix4 column-major, Quaternion, MathUtils)
```

- **Math** - value types, column-major Matrix4, `Data()` passes directly to `glUniformMatrix4fv`
- **Core** - Window (PIMPL, Win32+WGL, 4.6->3.3->legacy fallback), Input (polled keyboard/mouse, Win32 Raw Input + WM messages), FileSystem (file I/O + path utilities), ThreadPool (fixed-size, `std::thread`)
- **Graphics** - RAII GL object wrappers only (Shader, VertexBuffer, IndexBuffer, VertexArray, VertexLayout, Texture via stb_image, Framebuffer). No scene-level concepts.
- **Scene** - sparse-set ECS (Entity/ComponentPool/Transform/Camera/Light/MeshRenderer components), plus Material/Mesh/RenderQueue/Renderer that together form the full render pipeline (`BuildRenderQueue` -> `Sort` -> `Execute` in opaque+transparent passes)
- **Framework** - `ApplicationConfig` with `onInit/onUpdate/onRender/onShutdown` lambdas, `Clock` for delta time/FPS, `WinMain` inside the library

**User-facing API**: User defines `sgkit::CreateApplication()` returning `ApplicationConfig`. The library's `WinMain` calls it, then orchestrates init -> loop -> shutdown. Engine modules are singletons accessed via `::instance()`:

| Module | Access |
|--------|--------|
| Window | `core::Window::instance()` |
| Input | `core::Input::instance()` |
| Renderer | `scene::Renderer::instance()` |
| Scene | `scene::Scene::instance()` |
| ThreadPool | `core::ThreadPool::instance()` |
| Delta time | `framework::Clock::GetFrameDeltaSeconds()` |
| FPS | `framework::Clock::GetFPS()` |

## Key Conventions

- C++20, `/W4` (MSVC), `-Wall -Wextra -Wpedantic` (GCC/Clang)
- Namespaces `snake_case`, classes `PascalCase`, members `m_camelCase`, enums `PascalCase`
- `#pragma once`, Allman braces (functions/classes), 4-space indent
- RAII for GL objects, move-only, `SGK_ASSERT(cond, msg)` for programmer errors (defined in `DebugOut.h`, triggers `__debugbreak` in Debug, no-op in Release)
- Column-major Matrix4, `Data()` passes directly to `glUniformMatrix4fv`
- Platform code: PIMPL in headers, `#ifdef SGK_PLATFORM_WINDOWS` in `.cpp`
- Debug output: `SGK_LOG_INFO/WARN/ERROR/FATAL(category, format, ...)` macros from `<sgkit/framework/DebugOut.h>` (available via `sgkit.h`). No-ops in Release. `SGK_ASSERT(cond, msg)` for programmer errors. These macros are not thread-safe. Output goes to both `stderr` and `OutputDebugStringA` in Debug builds. Console attached via `AllocConsole()` at startup.
- Texture loading via stb_image (PNG/JPG/BMP/TGA etc.); slot-based binding
