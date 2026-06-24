# SGKit 资源管理

梳理库内所有资源的类型、持有方式和释放时机。

## 资源层级

```
第一层：平台资源（Window）
  HWND, HDC, HGLRC, HIMC

第二层：GL 原子资源（析构函数调用 glDelete*）
  VertexBuffer(VBO), IndexBuffer(EBO), VertexArray(VAO),
  Shader, Texture, Framebuffer(FBO+深度纹理)

第三层：组合对象（shared_ptr 成员，自身无 GL 句柄）
  Material -- 持有 Shader + Texture
  Mesh     -- 持有 VertexArray + Material

第四层：Scene ECS
  MeshRenderer -- 持有 shared_ptr<Mesh>
  ComponentPool<MeshRenderer> -- 值存储 MeshRenderer

第五层：引擎全局（Application.cpp, unique_ptr）
  g_window   -- Window
  g_renderer -- Renderer（无 GL 资源，只调 GL 状态）
  g_input    -- Input（无 GL 资源）
  g_scene    -- Scene（容器，无自有 GL 句柄）
```

## 构造顺序（Run() 内）

```
1. g_window   = make_unique<Window>()     -- 创建窗口 + GL 上下文
2. gladLoadGL()                           -- 加载 GL 函数指针
3. g_renderer = make_unique<Renderer>()   -- 设置 GL 状态默认值
4. g_input    = make_unique<Input>()      -- 挂接窗口事件
5. g_scene    = make_unique<Scene>()      -- 空 ECS 容器
6. config.onInit()                        -- 用户创建网格和实体
```

## 析构顺序（Run() 末尾和错误路径）

反序销毁：`g_scene` -> `g_renderer` -> `g_input` -> `g_window`。

```cpp
g_scene.reset();     // (1) 清空所有 MeshRenderer
                     //     -> 每个 MeshRenderer 析构
                     //     -> shared_ptr<Mesh> 引用计数 -1
                     //     -> 若归零: Mesh -> VertexArray/Shader/Texture 析构
                     //     -> glDelete* 调用（此时 GL 上下文仍存活）

g_renderer.reset();  // (2) 清理（无 GL 资源）

g_input.reset();     // (3) 关闭 Raw Input

g_window.reset();    // (4) 销毁 GL 上下文（wglDeleteContext 等）
                     //     此后所有 glDelete* 调用将无效
```

**核心保证**：所有 RAII 析构中发起的 `glDelete*` 都发生在 GL 上下文销毁之前。

## 各层所有权细节

### 第二层：GL 原子资源

全部仅可移动（copy 已删除，move noexcept）。

| 类 | GL 资源 | 析构调用 |
|----|---------|----------|
| VertexBuffer | GLuint buffer | `glDeleteBuffers(1, &m_handle)` |
| IndexBuffer | GLuint buffer | `glDeleteBuffers(1, &m_handle)` |
| VertexArray | GLuint array | `glDeleteVertexArrays(1, &m_handle)` |
| Shader | GLuint program | `glDeleteProgram(m_programID)` |
| Texture | GLuint texture | `glDeleteTextures(1, &m_handle)` |
| Framebuffer | GLuint FBO + 深度纹理 | `glDeleteFramebuffers` + `glDeleteTextures` |

使用 `shared_ptr` 管理，允许多个 Material 共享同一个 Shader 或 Texture。

### 第三层：组合对象

**Material**：持有 `shared_ptr<Shader>` + `shared_ptr<Texture>`。无自定义析构，成员自动释放。

**Mesh**：持有 `shared_ptr<VertexArray>` + `shared_ptr<Material>`。无自定义析构。

### 第四层：Scene

**MeshRenderer**：持有 `shared_ptr<Mesh>`。由 `ComponentPool<MeshRenderer>` 值存储。池销毁时每个 MeshRenderer 析构，其 Mesh 指针释放。

**Scene**：持有四种 `ComponentPool<T>`。析构时清空所有池，触发上述链条。无需手写析构代码。

### 第五层：引擎全局

全部是 Application.cpp 匿名命名空间中的 `std::unique_ptr<T>`。这提供了显式的销毁顺序控制。

## shared_ptr 的风险

由于 Mesh/Material 使用 `shared_ptr` 持有子资源，如果用户在全局或静态变量中额外保存了一份 `shared_ptr<Mesh>`（或 `shared_ptr<Shader>` 等），该资源就不会在 `g_scene.reset()` 时释放。它将在 C++ 静态析构阶段释放，而此时 `g_window` 已销毁、GL 上下文已不存在。

**正确做法**：在 `config.onShutdown` 回调中释放所有用户持有的 GL 资源 shared_ptr。

```cpp
cfg.onShutdown = []() {
    g_myExtraMesh.reset();   // 在引擎清理前释放
    g_myExtraShader.reset();
};
```

## 错误路径清理

如果初始化失败（窗口创建、GL 加载、onInit 返回 false），引擎会反序销毁已创建的模块再返回：

```cpp
// 例：窗口创建成功，但 gladLoadGL 失败
g_window.reset();  // 清理已创建的窗口

// 例：onInit 返回 false
g_scene.reset();
g_renderer.reset();
g_input.reset();
g_window.reset();
```

错误路径无资源泄漏。

## 总结

| 资源类别 | 持有方式 | 释放途径 |
|----------|---------|---------|
| 平台句柄 | `unique_ptr<Window>` | `g_window.reset()` |
| GL 原子（VBO, VAO, Shader 等） | `shared_ptr` + RAII 析构 | MeshRenderer 池清空 |
| Scene 组件 | ECS 池值存储 | `g_scene.reset()` |
| 引擎全局 | `unique_ptr`（显式顺序） | 反序 reset |
| 用户自持 shared_ptr | 用户负责 | `onShutdown` 回调 |
