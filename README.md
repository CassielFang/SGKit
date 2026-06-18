# SGKit — Straightforward Graphics Kit

轻量 C++ 3D 渲染引擎，基于 OpenGL，面向学习与实验。

## 核心理念

**零第三方依赖**（除 glad OpenGL 加载器外），全部基于 C++17 标准库 + 平台 API 手写。

## 模块架构

```
 +-----------+      +-----------+
 |   Scene   |      |  FileIO   |
 +-----+-----+      +-----+-----+
       |                   |
 +-----+-----+      +-----+-----+
 |  Graphics |      | Threading |
 +-----+-----+      +-----------+
       |
 +-----+-----+
 |   Input   |
 +-----+-----+
       |
 +-----+-----+      +-----------+
 |  Window   |      |   Math    |
 +-----------+      +-----------+
```

- **Math** — 向量/矩阵/四元数/变换，列主序存储，直通 OpenGL
- **Threading** — 固定大小线程池，`Enqueue` / `WaitAll`
- **Window** — Win32 窗口 + WGL OpenGL 上下文（4.6 → 3.3 自动回退），PIMPL 隐藏平台类型
- **FileIO** — 文件读写 + 路径工具 + BMP 纹理加载器
- **Input** — 键盘/鼠标轮询（Raw Input + Win32 消息），`IsKeyDown` / `IsKeyPressed` / `IsKeyReleased`
- **Graphics** — Shader / VBO / VAO / Texture / Material / Mesh / Renderer，RAII 管理 GL 对象
- **Scene** — 稀疏集 ECS（Entity + ComponentPool<T>），Transform 层级 / Camera / Light / MeshRenderer
- **Framework** — 胶水层：`WinMain` 内嵌库中，用户只需实现 `CreateApplication()`

## 快速开始

### 环境

- Visual Studio 2022（需"C++ CMake 工具"组件）
- CMake 3.20+
- glad OpenGL 加载器（已内置在 `external/glad/`）

### 构建

用 Visual Studio（建议2022及以上版本） 打开文件夹，选择配置后生成即可

### 五分钟写一个窗口

```cpp
#include <sgkit/sgkit.h>

sgkit::ApplicationConfig sgkit::CreateApplication()
{
    sgkit::ApplicationConfig cfg;
    cfg.title = "Hello SGKit";

    cfg.onInit = []() -> bool {
        // 创建场景,加载资源
        return true;
    };

    cfg.onUpdate = [](float dt) {
        // 每帧逻辑
        if (sgkit::GetInput().IsKeyPressed(sgkit::core::KeyCode::k_Escape))
            sgkit::RequestQuit();
    };

    cfg.onRender = []() {
        // 渲染
    };

    return cfg;
}
```

库自带的 `WinMain` 会调用 `CreateApplication()` 并启动游戏循环。

### 引擎模块访问

在回调中通过以下自由函数获取引擎模块：

| 函数 | 返回 |
|------|------|
| `GetWindow()` | 窗口（尺寸,关闭,全屏等） |
| `GetInput()` | 输入（键盘/鼠标状态） |
| `GetRenderer()` | 渲染器（清屏,状态切换） |
| `GetScene()` | 场景（实体,组件,Transform 层级） |
| `GetDeltaTime()` | 当前帧间隔（秒） |
| `GetFPS()` | 帧率 |
| `RequestQuit()` | 请求退出 |

### Example 示例

`examples/sandbox/main.cpp` 是一个完整 demo：旋转棋盘纹理立方体 + WASD 移动 + 右键拖拽视角。所有代码在 `CreateApplication()` 中完成，不依赖任何外部资源。

## 项目结构

```
SGKit/
├── CMakeLists.txt                    # 根 CMake
├── CMakePresets.json                 # x64-Debug / x64-Release
├── cmake/                            # 编译设置,平台检测
├── external/glad/                    # glad 加载器（静态库）
├── include/sgkit/                    # 公共头文件
│   ├── sgkit.h                       #   聚合头
│   ├── math/                         #   向量/矩阵/四元数
│   ├── core/                         #   窗口/输入/线程池/文件
│   ├── graphics/                     #   Shader/VBO/纹理/渲染器
│   ├── scene/                        #   实体/组件/场景
│   └── framework/                    #   应用框架
├── src/                              # 实现文件（对应上述模块）
├── examples/                         # 示例应用
├── tests/                            # 单元测试
├── lib/                              # 构建产物（sgkit.lib / sgkit_d.lib）
│   ├── sgkit_d.lib                   #   Debug 静态库
│   ├── sgkit_d.pdb                   #   Debug 调试符号
│   └── sgkit.lib                     #   Release 静态库
└── icon/                             # 应用图标
```

## 技术规格

| 项目 | 值 |
|------|-----|
| 语言 | C++17 |
| 平台 | Windows 10+（架构预留 Linux/macOS） |
| 图形 | OpenGL 4.6 Core（自动回退 3.3 → Legacy） |
| 构建 | CMake 3.20+ / Visual Studio 2022 |
| 唯一第三方依赖 | glad（OpenGL 加载器） |

## 在其他项目中使用

**方式一：链接预编译库**（不需要 SGKit 源码）

```cmake
# 把 SGKit 的 include/ 和 lib/ 拷到你的项目
target_include_directories(YourApp PRIVATE path/to/SGKit/include)
target_link_directories(YourApp PRIVATE path/to/SGKit/lib)
# Debug → sgkit_d.lib, Release → sgkit.lib
target_link_libraries(YourApp PRIVATE sgkit_d)   # or sgkit
```

**方式二：源码集成**（可自定义引擎）

```cmake
add_subdirectory(external/SGKit)
target_link_libraries(YourApp PRIVATE sgkit)
```

## 许可证

MIT
