# SGKit 用户手册

## 目录
1. [快速开始](#快速开始)
2. [Framework 框架层](#framework)
3. [Math 数学库](#math)
4. [Core 核心模块](#core)
5. [Graphics 图形模块](#graphics)
6. [Scene 场景模块](#scene)
7. [构建与项目结构](#构建)

---

## 快速开始

**最小程序**——只需要一个 `.cpp` 文件：

```cpp
#include <sgkit/sgkit.h>

sgkit::ApplicationConfig sgkit::CreateApplication()
{
    sgkit::ApplicationConfig cfg;
    cfg.title  = "My App";
    cfg.width  = 1280;
    cfg.height = 720;

    cfg.onInit = []() -> bool {
        // 初始化：创建场景、加载资源
        return true;
    };

    cfg.onUpdate = [](float dt) {
        // 每帧逻辑：处理输入、更新状态
        if (sgkit::GetInput().IsKeyPressed(sgkit::core::KeyCode::k_Escape))
            sgkit::RequestQuit();
    };

    cfg.onRender = []() {
        // 渲染
    };

    cfg.onShutdown = []() {
        // 清理
    };

    return cfg;
}
```

不需要继承类、不需要写 `WinMain`、不需要 `#include <windows.h>`。引擎内部处理一切。

---

## Framework

### ApplicationConfig

| 字段 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| `title` | `std::string` | `"SGKit"` | 窗口标题（UTF-8 编码） |
| `width` | `int` | `1280` | 窗口宽度 |
| `height` | `int` | `720` | 窗口高度 |
| `vsync` | `bool` | `true` | 垂直同步 |
| `fullscreen` | `bool` | `false` | 启动后立即无边框全屏 |
| `glMajor` / `glMinor` | `int` | `4` / `6` | OpenGL 版本（自动回退 3.3 → Legacy） |
| `onInit` | `std::function<bool()>` | — | 初始化回调。返回 `false` 则退出 |
| `onUpdate(float dt)` | `std::function<void(float)>` | — | 每帧更新，`dt` 为帧间隔（秒） |
| `onRender()` | `std::function<void()>` | — | 每帧渲染 |
| `onShutdown()` | `std::function<void()>` | — | 退出前清理 |

### 引擎全局函数

| 函数 | 返回类型 | 说明 |
|------|----------|------|
| `GetWindow()` | `core::Window&` | 窗口控制 |
| `GetInput()` | `core::Input&` | 输入轮询 |
| `GetRenderer()` | `graphics::Renderer&` | 渲染器状态 |
| `GetScene()` | `scene::Scene&` | 场景（实体、组件） |
| `GetDeltaTime()` | `float` | 当前帧间隔（秒） |
| `GetFPS()` | `float` | 每秒帧数 |
| `RequestQuit()` | `void` | 请求退出游戏循环 |

---

## Math

所有类型在 `sgkit::math` 命名空间。向量/矩阵为值类型（Plain Struct），运算符重载。

### 常量 (MathUtils.h)

```cpp
k_Pi, k_TwoPi, k_HalfPi, k_Epsilon, k_Deg2Rad, k_Rad2Deg
ToRadians(deg), ToDegrees(rad), Approximately(a, b, eps), Clamp(v, lo, hi), Lerp(a, b, t)
```

### Vector2

```cpp
Vector2 v{x, y};
v.x, v.y                        // 分量
v[0], v[1]                      // 下标访问
v + w, v - w, v * s, v / s, -v // 算术
v.Length(), v.LengthSquared()   // 长度
v.Normalize(), v.Normalized()   // 归一化
Vector2::Dot(a, b)              // 点乘
Vector2::Lerp(a, b, t)          // 线性插值
Vector2::k_Zero, k_One, k_Up, k_Right  // 静态常量
```

### Vector3

```cpp
Vector3 v{x, y, z};
v.x, v.y, v.z                   // 分量
v.XY(), v.XZ(), v.YZ()          // 提取 Vector2
Vector3::Cross(a, b)            // 叉乘
Vector3::k_Zero, k_One          // 静态常量
Vector3::k_Up, k_Down, k_Right, k_Left, k_Forward, k_Back
```

### Vector4

```cpp
Vector4 v{x, y, z, w};
v.XYZ(), v.XY()                 // 提取低维
```

### Matrix4

**列主序存储**，`m[col][row]`。可直接 `Data()` 传给 `glUniformMatrix4fv`。

```cpp
Matrix4 m;                               // 默认 = Identity
m(0, 0) = 2.0f;                          // 元素访问: (col, row)
m.Data()                                 // float*，直传 OpenGL

// 设置
m.SetIdentity()                          // 单位阵
m.SetZero()                              // 零阵
m.SetTranslate({x, y, z})                // 平移
m.SetRotateX/Y/Z(radians)                // 旋转
m.SetScale({x, y, z})                    // 缩放
m.SetPerspective(fovY_rad, aspect, n, f) // 透视投影
m.SetOrthographic(l, r, b, t, n, f)      // 正交投影
m.SetLookAt(eye, target, up)             // 视图矩阵

// 操作
m.Transpose(), m.Transposed()            // 转置
m.Invert(), m.Inverse()                  // 求逆（退化矩阵返回 Identity）
m.Determinant()                          // 行列式

// 变换向量
Matrix4 * Vector4                        // 矩阵乘向量
m.TransformPoint(v)                      // 变换点（考虑平移，自动透视除法）
m.TransformDirection(v)                  // 变换方向（忽略平移）

// 静态工厂（返回新矩阵）
Matrix4::Translate(v), ::RotateX(r), ::Scale(v)
Matrix4::Perspective(fov, aspect, n, f)
Matrix4::LookAt(eye, target, up)
```

### Quaternion

```cpp
Quaternion q;                                    // 默认 = Identity (w=1)
Quaternion q{x, y, z, w};
q * q2                                           // 四元数乘法
q * Vector3                                      // 旋转向量
q.Normalize(), q.Normalized()                    // 归一化
q.Conjugate(), q.Conjugated()                    // 共轭
q.Invert(), q.Inverse()                          // 求逆
q.Length(), q.LengthSquared()                    // 长度
q.Dot(q2)                                        // 点乘
q.ToEulerAngles()                                // 转欧拉角 (pitch, yaw, roll)

// 静态工厂
Quaternion::Identity()                           // 单位四元数
Quaternion::FromEulerAngles(pitch, yaw, roll)    // 从欧拉角
Quaternion::FromAxisAngle(axis, radians)          // 绕轴旋转
Quaternion::Slerp(a, b, t)                       // 球面线性插值
Quaternion::LookAt(direction, up)                // 朝向
```

### math::Transform

纯 CPU 变换，不涉及场景层级。

```cpp
Transform tf;
tf.position = {0, 0, 0};        // 位置
tf.rotation = Quaternion{};      // 旋转
tf.scale    = {1, 1, 1};        // 缩放
tf.GetLocalMatrix()              // 计算 T * R * S 矩阵（Scale → Rotate → Translate）
```

---

## Core

### Window

窗口 + OpenGL 上下文。引擎自动管理，通过 `GetWindow()` 访问。

```cpp
auto& w = GetWindow();

// 查询
w.GetWidth(), w.GetHeight()      // 当前尺寸
w.GetAspectRatio()               // 宽高比 (w/h)
w.IsRunning()                    // 窗口是否存活
w.IsCreated()                    // 是否已创建
w.IsFullscreen()                 // 是否全屏

// 操作
w.RequestClose()                 // 请求关闭
w.SwapBuffers()                  // 交换前后缓冲（引擎自动调用）
w.Maximize()                     // 最大化（fullscreen=true 时 → 无边框全屏）
w.Minimize()                     // 最小化
w.Restore()                      // 还原
w.SetFullscreen(true/false)      // 切换无边框全屏
w.SetIMEEnabled(true/false)      // 开关输入法（默认关闭）

// 初始化时通过 ApplicationConfig 设置 fullscreen=true 启动即全屏
// 引擎自动检测 Esc 退出全屏
```

### Input

轮询式输入，通过 `GetInput()` 访问。

```cpp
auto& in = GetInput();

// 键盘
in.IsKeyDown(KeyCode::k_W)        // 按住
in.IsKeyPressed(KeyCode::k_Space) // 刚按下（上升沿，只触发一帧）
in.IsKeyReleased(KeyCode::k_E)    // 刚松开（下降沿）

// 鼠标
in.IsMouseButtonDown(0)           // 0=左键 1=右键 2=中键 3=侧键4 4=侧键5
in.IsMouseButtonPressed(1)        // 刚按下
in.GetMouseX(), GetMouseY()       // 屏幕坐标
in.GetMouseDeltaX()               // 本帧 X 位移（Raw Input 高精度）
in.GetMouseDeltaY()               // 本帧 Y 位移
in.GetScrollDelta()               // 滚轮增量（正=向上）
```

### KeyCode 枚举

```
字母:   k_A ~ k_Z
数字:   k_0 ~ k_9
功能:   k_Escape, k_Enter, k_Tab, k_Backspace, k_Delete, k_Insert
方向:   k_Left, k_Right, k_Up, k_Down
翻页:   k_PageUp, k_PageDown, k_Home, k_End
F键:    k_F1 ~ k_F25
修饰:   k_LeftShift, k_LeftCtrl, k_LeftAlt, k_LeftSuper
        k_RightShift, k_RightCtrl, k_RightAlt, k_RightSuper
小键盘: k_KeyPad0 ~ k_KeyPad9, k_KeyPadAdd, k_KeyPadEnter...
鼠标:   k_MouseLeft, k_MouseRight, k_MouseMiddle, k_MouseButton4, k_MouseButton5
其他:   k_Space, k_Comma, k_Minus, k_Period, k_Slash, k_Semicolon, k_Equal
        k_LeftBracket, k_Backslash, k_RightBracket, k_GraveAccent
```

### FileSystem

静态文件操作类。

```cpp
// 读取
auto text = FileSystem::ReadText("path/to/file.txt");
if (text) { std::string content = *text; }

auto bin = FileSystem::ReadBinary("path/to/data.bin");
if (bin) { std::vector<uint8_t> data = *bin; }

// 写入
FileSystem::WriteText("path", "content");
FileSystem::WriteBinary("path", bytes);

// 查询
FileSystem::Exists("path")                    // 是否存在
FileSystem::IsDirectory("path")               // 是否目录
FileSystem::GetDirectory("path/to/file")      // "path/to"
FileSystem::GetExtension("file.txt")          // "txt"
FileSystem::GetFilename("a/b/c.txt")          // "c.txt"
FileSystem::GetFilenameWithoutExtension(...)  // "c"

// 资产路径
FileSystem::SetAssetDirectory("assets/");
FileSystem::GetAssetPath("shaders/default.vert")  // "assets/shaders/default.vert"
```

### ThreadPool

固定大小线程池。

```cpp
ThreadPool pool(4);                            // 4 线程，0 = 自动检测

auto future = pool.Enqueue([](int x) {
    return x * x;
}, 42);
int result = future.get();                      // 1764

pool.WaitAll();                                 // 等待所有任务完成
pool.PendingTasks();                            // 待处理任务数
```

---

## Graphics

所有 GL 对象 RAII（析构自动释放），仅可移动。

### Shader

```cpp
auto s = std::make_shared<Shader>();

// 从文件加载（通过 FileSystem）
s->LoadFromFile("assets/shaders/default.vert",
                "assets/shaders/default.frag");

// 从字符串加载
s->LoadFromSource(vertexSrc, fragmentSrc);

s->Bind();                                       // 激活
s->Unbind();
s->IsValid();                                    // 是否编译链接成功

// 设置 uniform
s->SetInt("u_Count", 5);
s->SetFloat("u_Time", 1.5f);
s->SetVector2("u_Size", {w, h});
s->SetVector3("u_Color", {r, g, b});
s->SetVector4("u_Plane", {a, b, c, d});
s->SetMatrix4("u_MVP", mat);                     // 直传 glUniformMatrix4fv
```

### VertexBuffer

```cpp
auto vb = std::make_shared<VertexBuffer>();

// data=顶点数组, size=字节数, usage=GL_STATIC_DRAW / GL_DYNAMIC_DRAW
vb->Create(data, sizeof(data));
vb->Create(data, sizeof(data), GL_DYNAMIC_DRAW);

vb->Bind();
vb->Unbind();
vb->SetData(newData, size, offset);              // 更新部分数据
vb->GetHandle();                                  // GL buffer ID
vb->GetSize();                                    // 字节数
```

### IndexBuffer

```cpp
auto ib = std::make_shared<IndexBuffer>();

// data=uint32_t 索引数组, count=索引个数
ib->Create(indices, 36);

ib->Bind();
ib->Unbind();
ib->GetCount();                                   // 索引个数
```

### VertexLayout

描述顶点属性布局。

```cpp
VertexLayout layout;
layout.PushFloat(0, 3)      // location=0, vec3 position
      .PushFloat(1, 3)      // location=1, vec3 normal
      .PushFloat(2, 2);     // location=2, vec2 texCoord

// 也支持整数：
// layout.PushUInt(3, 1);  // location=3, 单 uint

layout.GetStride();          // 单顶点字节数
layout.GetAttributes();      // 属性列表
```

### VertexArray

```cpp
auto va = std::make_shared<VertexArray>();
va->Create();

va->AddVertexBuffer(vb, layout);   // 绑定 VBO + 布局
va->SetIndexBuffer(ib);            // 绑定索引缓冲

va->Bind();
va->Draw();                        // 默认 GL_TRIANGLES
va->Draw(GL_LINES);               // 也可以线框
va->Unbind();
```

### Texture

```cpp
auto tex = std::make_shared<Texture>();

// 从文件加载（仅支持 BMP 24/32-bit 无压缩）
tex->LoadFromFile("path/to/image.bmp");

// 程序化创建
std::vector<uint8_t> pixels(w * h * 4);
// ... fill pixels (RGBA) ...
tex->Create(w, h, pixels.data());
tex->Create(w, h, pixels.data(), GL_RGBA8, GL_RGBA);  // 指定格式

// 过滤
tex->SetFilterLinear(true);    // 线性过滤（默认，平滑）
tex->SetFilterLinear(false);   // 邻近过滤（像素风格/棋盘格）

// 环绕
tex->SetWrapRepeat(true);      // 重复（默认）
tex->SetWrapRepeat(false);     // 钳位边缘

tex->Bind(0);                  // 绑定到纹理单元 0
tex->Bind(1);                  // 绑定到纹理单元 1
tex->Unbind();

tex->GetWidth(), tex->GetHeight();
```

### Material

```cpp
auto mat = std::make_shared<Material>();
mat->shader         = myShader;
mat->diffuseTexture = myTexture;

mat->ambientColor   = {0.2f, 0.2f, 0.2f};   // 环境光反射率
mat->diffuseColor   = {0.8f, 0.8f, 0.8f};   // 漫反射率
mat->specularColor  = {1.0f, 1.0f, 1.0f};   // 高光颜色
mat->shininess      = 64.0f;                 // 高光锐度
mat->opacity        = 1.0f;                  // 不透明度

mat->Apply();  // 绑定 shader + 纹理 + 上传 uniform（由 Mesh::Render 自动调用）
```

### Mesh

```cpp
auto mesh = std::make_shared<Mesh>();
mesh->vertexArray = va;     // VAO（含 VBO + EBO + 布局）
mesh->material    = mat;    // Shader + 纹理 + 材质参数

mesh->Render();             // = mat->Apply() + va->Draw()
```

### Renderer

```cpp
auto& r = GetRenderer();

r.SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});  // RGBA
r.Clear();                                      // 清屏（颜色+深度）

r.SetViewport(0, 0, w, h);                      // 视口

// 状态开关
r.SetDepthTest(true);    // 深度测试（默认开）
r.SetBlend(true);        // 混合（默认开，alpha blending）
r.SetCullFace(true);     // 面剔除（默认开，背面剔除）
r.SetWireframe(true);    // 线框模式

// 绘制
r.Draw(mesh);
r.Draw(vertexArray);     // 直接画 VAO（无 Material）
```

### Framebuffer

离屏渲染——深度贴图、后处理等。

```cpp
Framebuffer fbo;
fbo.Create(1024, 1024);            // 创建 1024×1024 深度纹理 FBO
if (!fbo.IsValid()) { /* 失败 */ }

fbo.Bind();                        // 渲染到 FBO
// ... draw ...
fbo.Unbind();                      // 回到默认帧缓冲

fbo.GetDepthTexture();             // 获取深度纹理 ID 用于 shader 采样
fbo.GetWidth(), fbo.GetHeight();

// 典型用途：阴影映射
// 1. fbo.Bind() → 从光视角渲染场景 → fbo.Unbind()
// 2. 绑定 fbo.GetDepthTexture() 到 shader → 采样阴影贴图
```

---

## Scene

ECS（实体-组件系统），稀疏集存储，O(1) 增删查。

### Entity

```cpp
Entity e = GetScene().CreateEntity();           // 创建实体
GetScene().DestroyEntity(e);                    // 销毁实体（移除所有组件）
GetScene().IsAlive(e);                          // 是否存活

k_InvalidEntity                                 // 无效实体常量 (0xFFFFFFFF)
k_MaxEntities                                   // 最大实体数 (10000)
```

### 组件

当前支持的组件：

| 组件 | 关键字段 | 说明 |
|------|----------|------|
| `Transform` | `position`, `rotation`, `scale`, `parent`, `children` | 层级变换 |
| `Camera` | `fovY`, `nearPlane`, `farPlane` | 摄像机 |
| `Light` | `type`, `color`, `intensity`, `range` | 光源 |
| `MeshRenderer` | `mesh`, `enabled` | 渲染网格 |

### 组件操作

```cpp
auto& scene = GetScene();

// 添加
auto& tf = scene.AddComponent<scene::Transform>(entity);
auto& cam = scene.AddComponent<scene::Camera>(entity);

// 获取（返回指针，没有返回 nullptr）
Transform* t = scene.GetComponent<scene::Transform>(entity);

// 检查
bool has = scene.HasComponent<scene::Camera>(entity);

// 移除
scene.RemoveComponent<scene::Light>(entity);
```

### Transform

```cpp
auto& tf = scene.AddComponent<scene::Transform>(entity);
tf.position = {0, 1, 0};
tf.rotation = Quaternion::FromEulerAngles(0, yaw, 0);
tf.scale    = {0.8f, 0.8f, 0.8f};

// 层级
tf.parent = parentEntity;           // 设置父节点
tf.children.push_back(child);       // 添加子节点

tf.GetLocalMatrix();                // 本地变换矩阵
GetScene().GetWorldMatrix(entity);  // 世界变换矩阵（含父子层级）
```

### Camera

```cpp
auto& cam = scene.AddComponent<scene::Camera>(entity);
cam.fovY      = 60.0f;       // 垂直 FOV（度）
cam.nearPlane = 0.1f;        // 近平面
cam.farPlane  = 1000.0f;     // 远平面

// 内部方法（通常不需要手动调用）
cam.GetViewMatrix(worldMatrix);        // 视图矩阵 = world 的逆
cam.GetProjectionMatrix(aspectRatio);  // 透视投影
```

### Light

```cpp
auto& light = scene.AddComponent<scene::Light>(entity);
light.type      = LightType::k_Directional;  // 方向光
light.type      = LightType::k_Point;        // 点光源
light.color     = {1.0f, 0.95f, 0.8f};      // 颜色
light.intensity = 1.0f;                      // 强度
light.range     = 8.0f;                      // 范围（仅点光源）

// 方向光：Transform.position 存光照方向（指向光源）
// 例如 {0.3, 1.0, 0.4}.Normalized() 表示右上方
```

### MeshRenderer

```cpp
auto& mr = scene.AddComponent<scene::MeshRenderer>(entity);
mr.mesh    = myMesh;     // 共享指针
mr.enabled = true;       // 是否渲染
```

### Scene 生命周期

```cpp
scene.OnUpdate(dt);     // 引擎自动调用：更新世界变换矩阵
scene.OnRender(...);    // 引擎自动调用：遍历 MeshRenderer 绘制
```

---

## 构建与项目结构

```
SGKit/
├── CMakeLists.txt
├── CMakePresets.json           # x64-Debug / x64-Release
├── cmake/
│   ├── Platform.cmake          # 平台库
│   └── CompilerSettings.cmake  # C++17 /W4 等
├── external/glad/              # OpenGL 加载器
├── include/sgkit/              # 公共头文件
│   ├── math/     向量/矩阵/四元数/变换
│   ├── core/     窗口/输入/文件/线程池
│   ├── graphics/ Shader/VBO/VAO/纹理/材质/渲染器
│   ├── scene/    ECS/实体/组件
│   └── framework/ 应用入口
├── src/                        # 实现
├── examples/sandbox/           # 示例
└── tests/                      # 单元测试
```

### 在其他项目中使用

```cmake
# 将 SGKit 放到 external/SGKit/
add_subdirectory(external/SGKit)
target_link_libraries(YourApp PRIVATE sgkit)
```

### 编译选项

| 宏 | 说明 |
|----|------|
| `UNICODE` / `_UNICODE` | 全局定义 |
| `NOMINMAX` | 防 windows.h 污染 std::min/max |
| `WINVER=0x0A00` | 最低 Windows 10 |
| `_WINDOWS` | GUI 子系统 |
| `SGK_PLATFORM_WINDOWS` | 库内部使用（非公开） |
| `SGK_DEBUG` | Debug 专属 |

### Shader 约定

- 顶点属性 location：`0=Position, 1=Normal, 2=TexCoord`
- Shader 版本：`#version 330 core`
- 矩阵：列主序，`u_MVP` 等用 `SetMatrix4` 设置
- 纹理采样器：`u_DiffuseTexture`（单元 0）
