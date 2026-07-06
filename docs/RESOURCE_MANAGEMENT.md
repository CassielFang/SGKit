# SGKit 资源管理

梳理库内所有资源的类型、持有方式和释放时机。

## 资源层级

```
第一层：平台资源（Window）
  HWND, HDC, HGLRC, HIMC

第二层：GL 原子资源（析构函数调用 glDelete*）
  VertexBuffer(VBO), IndexBuffer(EBO), VertexArray(VAO),
  Shader, Texture, FrameBuffer(FBO+深度纹理)

第三层：组合对象（shared_ptr 成员，自身无 GL 句柄）
  Material    -- 持有 Shader + Texture + 渲染状态枚举（scene:: 命名空间）
  Mesh        -- 纯数据容器：持有 VertexArray + Material（scene:: 命名空间）
  RenderQueue -- 帧级绘制批次容器，无 GL 资源（scene:: 命名空间）

第四层：Scene ECS
  MeshRenderer -- 持有 shared_ptr<Mesh>
  ComponentPool<MeshRenderer> -- 值存储 MeshRenderer

第五层：引擎全局（模块类内部静态指针，通过 Create/Destroy/instance() 管理）
  Window     -- core::Window（平台句柄 + GL 上下文）
  Renderer   -- scene::Renderer（无 GL 资源，只调 GL 状态）
  Input      -- core::Input（无 GL 资源）
  Scene      -- scene::Scene（容器，无自有 GL 句柄）
  ThreadPool -- core::ThreadPool（工作线程）
```

## 构造顺序（Run() 内）

```
1. core::Window::Create(hInst, wd)          -- 创建窗口 + GL 上下文 + gladLoadGL
2. core::ThreadPool::Create(numThreads)     -- 固定大小线程池
3. core::Input::Create(window handle)       -- 挂接窗口输入
4. scene::Renderer::Create()                -- 设置 GL 状态默认值
5. scene::Scene::Create()                   -- 空 ECS 容器
6. config.onInit()                          -- 用户创建网格和实体
```

## 析构顺序（Run() 末尾和错误路径）

反序销毁：Scene -> Renderer -> Input -> ThreadPool -> Window。

```cpp
scene::Scene::Destroy();     // (1) 清空所有 MeshRenderer
                             //     -> 每个 MeshRenderer 析构
                             //     -> shared_ptr<Mesh> 引用计数 -1
                             //     -> 若归零: Mesh -> VertexArray/Shader/Texture 析构
                             //     -> glDelete* 调用（此时 GL 上下文仍存活）

scene::Renderer::Destroy();  // (2) 清理（无 GL 资源）

core::Input::Destroy();      // (3) 关闭 Raw Input

core::ThreadPool::Destroy(); // (4) 等待任务完成，回收线程

core::Window::Destroy();     // (5) 销毁 GL 上下文（wglDeleteContext 等）
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
| FrameBuffer | GLuint FBO + 深度纹理 | `glDeleteFramebuffers` + `glDeleteTextures` |

使用 `shared_ptr` 管理，允许多个 Material 共享同一个 Shader 或 Texture。

### 第三层：组合对象

**Material**：持有 `shared_ptr<Shader>` + `shared_ptr<Texture>` + 渲染状态枚举（blendMode, cullMode, depthMode）。无自定义析构，成员自动释放。

**Mesh**：纯数据容器，持有 `shared_ptr<VertexArray>` + `shared_ptr<Material>`。无 Render() 方法，渲染逻辑已移至 Renderer::ExecuteBatch()。无自定义析构。

**RenderQueue**：帧级数据结构（RenderBatch 向量），由 Scene::BuildRenderQueue() 构建，由 Renderer::Execute() 消费。不持有任何 GL 句柄。

### 第四层：Scene

**MeshRenderer**：持有 `shared_ptr<Mesh>`。由 `ComponentPool<MeshRenderer>` 值存储。池销毁时每个 MeshRenderer 析构，其 Mesh 指针释放。

**Scene**：持有四种 `ComponentPool<T>`。析构时清空所有池，触发上述链条。无需手写析构代码。

### 第五层：引擎全局

每个模块类管理自己的静态指针实例，通过 `Create()`/`Destroy()`/`instance()` 暴露。引擎的 `Application.cpp` 在 `Run()` 开头按依赖顺序调用 `Create`，在末尾或错误路径上反序调用 `Destroy`。这种模式保证了显式的销毁顺序控制。

## shared_ptr 的风险

由于 Mesh/Material 使用 `shared_ptr` 持有子资源，如果用户在全局或静态变量中额外保存了一份 `shared_ptr<Mesh>`（或 `shared_ptr<Shader>` 等），该资源就不会在 `scene::Scene::Destroy()` 时释放。它将在 C++ 静态析构阶段释放，而此时 Window 已销毁、GL 上下文已不存在。

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
// 例：窗口创建成功，但 Input 创建失败
core::Input::Destroy();
core::ThreadPool::Destroy();
core::Window::Destroy();

// 例：onInit 返回 false
scene::Scene::Destroy();
scene::Renderer::Destroy();
core::Input::Destroy();
core::ThreadPool::Destroy();
core::Window::Destroy();
```

错误路径无资源泄漏。

## 总结

| 资源类别 | 持有方式 | 释放途径 |
|----------|---------|---------|
| 平台句柄 | 模块内部静态指针 | `core::Window::Destroy()` |
| GL 原子（VBO, VAO, Shader 等） | `shared_ptr` + RAII 析构 | MeshRenderer 池清空 |
| 组合对象（Material, Mesh, RenderQueue） | `shared_ptr` 或值 | 跟随 GL 原子析构 |
| Scene 组件 | ECS 池值存储 | `scene::Scene::Destroy()` |
| 引擎全局 | 模块单例（显式顺序） | 反序 `Destroy()` |
| 用户自持 shared_ptr | 用户负责 | `onShutdown` 回调 |
