# SGKit - Straightforward Graphics Kit

轻量 C++ 3D 渲染引擎，基于 OpenGL，面向学习与实验。

## 核心理念

**最小第三方依赖**：glad（OpenGL 加载器）+ stb_image（纹理加载），其余全部基于 C++20 标准库 + 平台 API 手写。

## 模块架构

```
Math -- 向量/矩阵/四元数（无外部依赖）
  |
Core -- Window（Win32+GL 上下文）/ Input（轮询键盘鼠标）/ FileSystem（文件读写+路径工具）/ ThreadPool（固定大小线程池）
  |
Graphics -- Shader / VBO / VAO / Texture / FBO / VertexLayout，纯 RAII GL 原子资源包装
  |
Scene -- 稀疏集 ECS（Entity + ComponentPool），Transform/Camera/Light/MeshRenderer 组件
         Material / Mesh / RenderQueue / Renderer（拥有完整渲染管线）
  |
Framework -- 胶水层：WinMain 内嵌库中，用户只需实现 CreateApplication()
```

- **Math** - 向量/矩阵/四元数，列主序存储，直通 OpenGL
- **Core** - Window（PIMPL, Win32 + WGL, 4.6->3.3 自动回退）、Input（Raw Input + Win32 消息）、FileSystem（文件读写 + 路径工具）、ThreadPool（固定大小线程池，`Enqueue` 返回 `TaskHandle`）
- **Graphics** - Shader / VBO / VAO / Texture / FBO / VertexLayout，纯 RAII GL 原子包装，无场景层概念。Texture 通过 stb_image 支持 PNG/JPG/BMP/TGA 等多种格式
- **Scene** - 稀疏集 ECS（Entity + ComponentPool<T>），Transform 层级（含父子关系）/ Camera / Light / MeshRenderer 组件。Material / Mesh / RenderQueue / Renderer 构成完整渲染管线（BuildRenderQueue -> Sort -> Execute，不透明+透明双通道）
- **Framework** - 胶水层：`WinMain` 内嵌库中，用户只需实现 `CreateApplication()`

## 快速开始

### 环境

- Visual Studio 2022 及以上（需"C++ CMake 工具"组件）
- CMake 3.20+

### 构建

用 Visual Studio（建议 2022 及以上版本）打开文件夹，选择 x64-Debug 或 x64-Release 配置后生成即可。

### 五分钟写一个窗口

```cpp
#include <sgkit/sgkit.h>

sgkit::ApplicationConfig sgkit::CreateApplication()
{
    sgkit::ApplicationConfig cfg;
    cfg.title = "Hello SGKit";

    cfg.onInit = []() -> bool {
        // 初始化场景、加载资源
        return true;
    };

    cfg.onUpdate = []() {
        // 每帧逻辑
        if (sgkit::core::Input::instance().IsKeyPressed(sgkit::core::KeyCode::Escape))
            sgkit::core::Window::instance().RequestClose();
    };

    cfg.onRender = []() {
        // 渲染
    };

    return cfg;
}
```

库自带的 `WinMain` 会调用 `CreateApplication()` 并启动游戏循环。

### 引擎模块访问

引擎模块均为单例，通过 `::instance()` 访问：

| 访问方式 | 返回 | 说明 |
|------|------|------|
| `core::Window::instance()` | `core::Window&` | 窗口（尺寸、关闭、全屏等） |
| `core::Input::instance()` | `core::Input&` | 输入（键盘/鼠标状态轮询） |
| `scene::Renderer::instance()` | `scene::Renderer&` | 渲染器（清屏、GL 状态切换、执行渲染队列） |
| `scene::Scene::instance()` | `scene::Scene&` | 场景（实体创建、组件管理、调用 Render） |
| `core::ThreadPool::instance()` | `core::ThreadPool&` | 线程池（异步任务提交） |
| `framework::Clock::GetFrameDeltaSeconds()` | `float` | 当前帧间隔（秒） |
| `framework::Clock::GetFPS()` | `float` | 帧率 |

### Example 示例

`examples/Light/main.cpp` 是光照演示：10 个漫反射+镜面反射立方体 + 1 个点光源 + 自由移动相机（WASD 移动、鼠标拖拽视角）。辅助函数封装在 `objects.h`/`objects.cpp` 中。使用 stb_image 加载 PNG 纹理。

## 项目结构

```
SGKit/
├── CMakeLists.txt                    # 根 CMake
├── CMakePresets.json                 # x64-Debug / x64-Release
├── external/
│   ├── glad/                         # OpenGL 加载器（静态库）
│   └── stb/                          # stb_image 纹理加载（静态库）
├── include/sgkit/                    # 公共头文件
│   ├── sgkit.h                       #   聚合头
│   ├── math/                         #   Vector2/3/4, Matrix4, Quaternion, MathUtils
│   ├── core/                         #   Window, Input, KeyCodes, FileSystem, ThreadPool
│   ├── graphics/                     #   Shader, VBO, VAO, Texture, FBO, VertexLayout（纯 GL 原子）
│   ├── scene/                        #   Entity, ComponentPool, Components, Material, Mesh, RenderQueue, Renderer, Scene
│   └── framework/                    #   Application, Timing
├── src/                              # 实现文件（按模块对应）
├── examples/                         # 演示示例
├── tests/                            # 单元测试
├── lib/                              # 构建产物（sgkit.lib / sgkit_d.lib）
│   ├── sgkit_d.lib                   #   Debug 静态库
│   ├── sgkit_d.pdb                   #   Debug 调试符号
│   └── sgkit.lib                     #   Release 静态库
└── icon/                             # 应用图标
```

更多文档：[docs/MATH_MODULE.md](docs/MATH_MODULE.md)（数学库详细 API）、[docs/THREAD_POOL.md](docs/THREAD_POOL.md)（线程池用法）。

## 技术规格

| 项目 | 值 |
|------|-----|
| 语言 | C++20 |
| 平台 | Windows 10+（架构预留 Linux/macOS） |
| 图形 | OpenGL 4.6 Core（自动回退 3.3 -> Legacy） |
| 构建 | CMake 3.20+ / Visual Studio 2022 及以上 |
| 第三方依赖 | glad（OpenGL 加载器）+ stb_image（纹理加载） |

## 在其他项目中使用

**方式一：链接预编译库**（不需要 SGKit 源码，只需 `include/` + `lib/`）

```cmake
target_include_directories(YourApp PRIVATE path/to/SGKit/include)
target_link_directories(YourApp PRIVATE path/to/SGKit/lib)
# Debug -> sgkit_d.lib, Release -> sgkit.lib。glad 和 stb 已内嵌，无需单独链接。
target_link_libraries(YourApp PRIVATE sgkit_d gdi32 user32 opengl32 imm32)
```

**方式二：源码集成**（可自定义引擎）

```cmake
add_subdirectory(external/SGKit)
target_link_libraries(YourApp PRIVATE sgkit)
# sgkit 会传递链接 glad、stb 和平台库（gdi32 user32 opengl32 imm32）
```

## 许可证

[MIT](LICENSE)
