# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SGKit (Straightforward Graphics Kit) — lightweight C++17 3D engine, OpenGL 4.6 Core, only third-party dependency is glad. Target Windows 10+, architecture allows Linux/macOS.

## Build

```bash
# VS 2022 or 2026 "Open Folder" — select x64-Debug / x64-Release and Build

# Run sandbox (Debug)
out/build/x64-Debug/examples/sandbox/Debug/sandbox.exe

# Run tests
cd out/build/x64-Debug && ctest -C Debug
```

## Architecture

8 modules, bottom-up dependencies:

```
Framework (WinMain + game loop, hidden in library)
  |
Scene (sparse-set ECS: Entity/ComponentPool/Transform/Camera/Light)
  |
Graphics (Shader/VBO/VAO/Texture with BMP loader/Material/Mesh/Renderer)
  |
Input (polled keyboard/mouse, Win32 Raw Input + WM messages)
  |
Window (PIMPL, Win32 + WGL, 4.6→3.3→legacy fallback)
  |
Math (Vector2/3/4, Matrix4 column-major, Quaternion, Transform)
  |
Threading (fixed-size thread pool, std::thread)
```

**User-facing API**: User defines `sgkit::CreateApplication()` returning `ApplicationConfig` with lambdas (`onInit`, `onUpdate`, `onRender`, `onShutdown`). The library's `WinMain` calls it. Engine modules accessed via global functions: `GetWindow()`, `GetInput()`, `GetRenderer()`, `GetScene()`.

## Key Conventions

- C++17, `/W4` (MSVC), `-Wall -Wextra -Wpedantic` (GCC/Clang)
- Namespaces `snake_case`, classes `PascalCase`, members `m_camelCase`, enums `k_PascalCase`
- `#pragma once`, Allman braces (functions/classes), 4-space indent
- RAII for GL objects, move-only, `std::optional` for failable, `SGK_ASSERT` for programmer errors
- Column-major Matrix4, `Data()` passes directly to `glUniformMatrix4fv`
- Platform code: PIMPL in headers, `#ifdef SGK_PLATFORM_WINDOWS` in `.cpp`
- No `OutputDebugStringA` — use `std::printf`/`fprintf`, console attached via `AllocConsole()` at startup
