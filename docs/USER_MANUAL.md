# SGKit 用户手册

SGKit（Straightforward Graphics Kit）是一个 C++ 3D 渲染引擎。核心第三方依赖仅有 glad（OpenGL 加载器）+ stb_image（纹理加载）+ assimp（3D 模型导入），其余全部基于 C++20 标准库 + Win32 平台 API。列主序矩阵,RAII 管理 GL 对象,稀疏集 ECS。

---

## 目录

1. [Hello World](#hello-world)
2. [Framework - 应用框架](#framework)
3. [Math - 数学库](#math)
4. [Core - 核心模块](#core)
5. [Graphics - 图形模块](#graphics)
6. [Scene - 场景模块](#scene)
7. [项目集成与约定](#项目集成)

---

## Hello World

只需要一个 `.cpp` 文件即可启动带窗口的 OpenGL 程序：

```cpp
#include <sgkit/sgkit.h>

sgkit::ApplicationConfig sgkit::CreateApplication()
{
    sgkit::ApplicationConfig cfg;
    cfg.title = "My App";
    cfg.width = 1280;
    cfg.height = 720;

    cfg.onInit = []() -> bool {
        // 初始化：创建场景,加载资源
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
    cfg.onShutdown = []() { /* 清理 */ };
    return cfg;
}
```

不需要继承类,不需要写 `WinMain`,不需要 `#include <windows.h>`。引擎内部已包含 `WinMain`，它负责创建窗口,加载 OpenGL,初始化各模块，然后调用你的回调。各引擎模块均为单例，通过 `::instance()` 访问：`core::Window::instance()`、`core::Input::instance()`、`scene::Renderer::instance()`、`scene::Scene::instance()`。

---

## Framework

Framework 是引擎的入口层，把平台细节（`WinMain`,消息循环,控制台重定向）封装在库内部，向用户暴露简单明了的四个回调。

### ApplicationConfig

配置结构体，所有字段都有默认值。

| 字段 | 类型 | 默认 | 作用 |
|------|------|------|------|
| `title` | `std::string` | `"SGKit"` | 窗口标题（UTF-8，内部自动转 UTF-16 传给 `CreateWindowExW`） |
| `width` | `int` | `1280` | 窗口客户区宽度（像素） |
| `height` | `int` | `720` | 窗口客户区高度（像素） |
| `resizable` | `bool` | `true` | 是否允许用户拖拽边框调整窗口大小 |
| `vsync` | `bool` | `true` | 是否垂直同步--开启后帧率锁定到显示器刷新率，防止画面撕裂 |
| `fullscreenBolderless` | `bool` | `false` | 无边框全屏窗口（覆盖整个显示器但不改变分辨率） |
| `fullscreen` | `bool` | `false` | 若为 `true`，窗口创建后立即进入独占全屏模式 |
| `cursorVisible` | `bool` | `true` | 是否显示鼠标光标（设为 false 可用于 FPS 相机） |
| `glMajor` | `int` | `4` | 请求的 OpenGL 主版本号 |
| `glMinor` | `int` | `6` | 请求的 OpenGL 次版本号--引擎依次尝试 4.6 -> 3.3 -> Legacy，只要任一成功即启动 |
| `numThreads` | `size_t` | `4` | 线程池工作线程数 |
| `onInit` | `function<bool()>` | - | 引擎完成各模块初始化后调用。**返回 `false` 表示初始化失败，程序退出** |
| `onUpdate` | `function<void()>` | - | 每帧调用一次。如需帧间隔时间，使用 `framework::Clock::GetFrameDeltaSeconds()` |
| `onRender` | `function<void()>` | - | 每帧调用一次，紧接在 `onUpdate` 和 `RecomputeWorldTransforms()` 之后。通常在此调用 `scene::Scene::instance().Render(cameraEntity)` |
| `onShutdown` | `function<void()>` | - | 窗口关闭后、引擎销毁前调用一次，用于清理用户资源 |

### 引擎模块访问

各模块均为全局单例，通过静态方法 `::instance()` 访问。生命周期由引擎自动管理（`Create` 在 `onInit` 之前，`Destroy` 在 `onShutdown` 之后）。

| 访问方式 | 返回类型 | 作用 |
|------|----------|------|
| `core::Window::instance()` | `core::Window&` | 获取窗口对象，可查询尺寸、设置全屏、关闭窗口等 |
| `core::Input::instance()` | `core::Input&` | 获取输入对象，可轮询键盘/鼠标/滚轮状态 |
| `scene::Renderer::instance()` | `scene::Renderer&` | 获取渲染器，可清屏、切换状态（线框、深度测试等） |
| `scene::Scene::instance()` | `scene::Scene&` | 获取场景对象，所有实体和组件的容器 |
| `core::ThreadPool::instance()` | `core::ThreadPool&` | 获取线程池，用于异步任务提交 |
| `framework::Clock::GetFrameDeltaSeconds()` | `float` | 返回当前帧间隔（秒） |
| `framework::Clock::GetFPS()` | `float` | 返回每秒帧数，每秒更新一次 |
| `core::Window::instance().RequestClose()` | `void` | 请求退出主循环。调用后当前帧完整执行完毕，程序正常退出 |

---

## Math

`sgkit::math` 命名空间。所有类型都是 值类型 class，可直接拷贝，没有虚函数。**Matrix4 使用列主序存储**，`Data()` 返回的指针可直接传给 `glUniformMatrix4fv(loc, 1, GL_FALSE, ptr)`。

### MathUtils

```cpp
k_Pi          // π = 3.1415927f
k_TwoPi       // 2π
k_HalfPi      // π/2
k_Epsilon     // 1e-5f，浮点判等容差
k_Deg2Rad     // 度转弧度因子 (π/180)
k_Rad2Deg     // 弧度转度因子 (180/π)

ToRadians(90)   // -> 1.5708  角度转弧度
ToDegrees(π)    // -> 180     弧度转角度

Approximately(a, b, eps)    // fabs(a-b) < eps，默认容差 k_Epsilon
Clamp(v, lo, hi)            // 将 v 钳制在 [lo, hi]
Lerp(a, b, t)               // a + (b-a)*t  线性插值
```

### Vector2 / Vector3 / Vector4

向量仅含浮点分量和基本算术运算，构造和赋值都是廉价字节拷贝。

```cpp
Vector2 v{1.0f, 2.0f};
v.x, v.y                    // 分量访问
v[0], v[1]                  // 下标访问（等价于 x, y）
v + w, v - w, v * 2.0f, v / 0.5f, -v  // 算术运算符
v.Length()                  // 向量长度（模）
v.LengthSquared()           // 长度的平方（更快，避免 sqrt）
v.Normalize()               // 原地归一化：各分量除以 Length()。长度 ≈ 0 时不做任何操作
v.Normalized()              // 返回归一化后的新向量，原向量不变
Vector2::Dot(a, b)          // 点乘：a.x*b.x + a.y*b.y
Vector2::Lerp(a, b, 0.3f)   // 线性插值，t 在 [0,1] 之间

// 静态常量
Vector2::k_Zero({0,0}), k_One({1,1}), k_Up({0,1}), k_Right({1,0})
Vector3::k_Zero, k_One, k_Up({0,1,0}), k_Down({0,-1,0})
Vector3::k_Right({1,0,0}), k_Left({-1,0,0})
Vector3::k_Forward({0,0,-1}), k_Back({0,0,1})  // OpenGL 惯例：-Z 向前
Vector4::k_Zero, k_One

// Vector3 独有
Vector3::Cross(a, b)        // 叉乘。a×b -- 结果同时垂直于 a 和 b
Vector3::Lerp(a, b, t)      // 线性插值
v3.XY(), v3.XZ(), v3.YZ()   // 提取对应二维分量

// Vector4 独有
v4.XYZ()                    // 提取前三个分量，丢弃 w
v4.XY()                     // 提取前两个分量
```

### Matrix4

4×4 列主序矩阵，`float m[4][4]`。访问约定 `m[col][row]`：

```
col0:  m[0][0] m[0][1] m[0][2] m[0][3]
col1:  m[1][0] m[1][1] m[1][2] m[1][3]
col2:  m[2][0] m[2][1] m[2][2] m[2][3]
col3:  m[3][0] m[3][1] m[3][2] m[3][3]
```

内存布局与 OpenGL 一致，`Data()` 返回 `&m[0][0]` 可直传 `glUniformMatrix4fv`。

```cpp
Matrix4 m;                      // 默认构造 = 单位阵
m(0, 0) = 2.0f;                 // 读写元素：operator()(col, row)
const float* p = m.Data();      // 获取 float* 指针

// --- 设置方法（原地修改自身）---
m.SetIdentity()                 // 变为单位阵（对角线 1，其余 0）
m.SetZero()                     // 变为全零阵
m.SetTranslate({x, y, z})       // 平移矩阵：col3 = (x, y, z, 1)
m.SetRotateX(rad)               // 绕 X 轴旋转 rad 弧度
m.SetRotateY(rad)               // 绕 Y 轴旋转
m.SetRotateZ(rad)               // 绕 Z 轴旋转
m.SetScale({x, y, z})           // 缩放矩阵：对角线 = (x, y, z, 1)
m.SetPerspective(fovY_rad, aspect, near, far)
    // 透视投影。fovY_rad=垂直视场角弧度, aspect=宽/高
m.SetOrthographic(l, r, b, t, near, far)
    // 正交投影。l/r/b/t=左右下上裁剪面
m.SetLookAt(eye, target, up)
    // 视图矩阵：摄像机在 eye，看向 target，up 指定上方

// --- 变换操作 ---
m.Transpose()       // 原地转置
m.Transposed()      // 返回转置后的新矩阵
m.Invert()          // 原地求逆（退化时不动，保留原值）
m.Inverted()        // 返回逆矩阵（退化时返回单位阵而非 NaN）
m.Determinant()     // 行列式

// --- 矩阵乘法 ---
Matrix4 c = a * b;              // 矩阵乘法，用于组合变换

// --- 变换向量 ---
Vector4 v4 = m * Vector4{x,y,z,w};    // 矩阵 × 列向量
Vector3 p2 = m.TransformPoint(p);     // 变换点：等价于 (m * vec4(p,1)).xyz / w
Vector3 d2 = m.TransformDirection(d); // 变换方向：等价于 (m * vec4(d,0)).xyz（平移被忽略）

// --- 静态工厂（返回新矩阵，不依赖已有实例）---
Matrix4::Identity()
Matrix4::Translate(v)       // -> 平移矩阵
Matrix4::RotateX/Y/Z(rad)   // -> 旋转矩阵
Matrix4::Scale(v)           // -> 缩放矩阵
Matrix4::Perspective(fovY_rad, aspect, near, far)
Matrix4::LookAt(eye, target, up)
```

**常见用法**：`Matrix4 mvp = proj * view * model;`，结果传给 `shader->SetMatrix4("u_MVP", mvp);`

### Quaternion

四元数表示三维旋转--避免欧拉角的万向节锁问题。单位四元数（`Length()≈1`）才能正确表示旋转。

```cpp
Quaternion q;                          // 默认 = Identity: (0,0,0,1)
Quaternion q{x, y, z, w};              // 从四分量构造

q * q2                                 // 四元数乘法 = 旋转组合（先 q 后 q2）
q * Vector3                            // 用四元数旋转向量

q.Length(), q.LengthSquared()          // 长度
q.Normalize(), q.Normalized()          // 归一化（旋转前应确保单位长度）
q.Conjugate(), q.Conjugated()          // 共轭 = (-x, -y, -z, w)
q.Invert(), q.Inverted()               // 逆四元数 = 共轭/长度²
q.Dot(q2)                              // 点乘

q.ToEulerAngles()                      // 转换为欧拉角 (pitch, yaw, roll)，单位弧度

// 静态工厂
Quaternion::Identity()
Quaternion::FromEulerAngles(pitch, yaw, roll)  // 从欧拉角创建（弧度）
Quaternion::FromAxisAngle(axis, radians)       // 绕任意轴旋转
Quaternion::Slerp(a, b, t)                     // 球面线性插值--两个朝向之间平滑过渡，t∈[0,1]
Quaternion::LookAt(direction, up)              // 朝向向量->四元数
```

> 完整数学模块 API 参见 [`docs/MATH_MODULE.md`](MATH_MODULE.md)。

---

## Core

### Window

窗口与 OpenGL 上下文的封装。由引擎创建和销毁，用户通过 `core::Window::instance()` 引用。

**背景**：PIMPL 模式把 Win32 类型（`HWND`,`HDC`,`HGLRC`）隐藏在不透明 `Impl` 中，用户代码不需要包含 `<windows.h>`。

```cpp
auto& w = core::Window::instance();

// 状态查询
w.GetWidth()          // 客户区宽度（像素）
w.GetHeight()         // 客户区高度（像素）
w.GetAspectRatio()    // 宽度/高度。防止除零：高度为 0 时返回 1.0
w.IsRunning()         // 窗口是否尚未关闭（收到 WM_CLOSE 后变 false）
w.IsCreated()         // Create() 成功后为 true
w.IsFullscreen()      // 是否处于无边框全屏状态

// 窗口操作
w.RequestClose()      // 发送 WM_CLOSE 消息。下一帧 IsRunning() 变 false，循环退出
w.SwapBuffers()       // 交换前/后缓冲（引擎主循环自动调用，用户一般不需要手动调）
w.Maximize()          // 最大化。若启动时 fullscreen=true，则跳转至无边框全屏
w.Minimize()          // 最小化到任务栏
w.Restore()           // 还原到正常大小

// 全屏
w.SetFullscreen(true)   // 进入无边框全屏：保存当前窗口样式和位置，切换为 WS_POPUP 并覆盖当前显示器
w.SetFullscreen(false)  // 退出全屏：恢复之前保存的样式,位置,尺寸
// 引擎自动处理：全屏状态下按 Esc 自动还原。若 fullscreen=true，标题栏最大化按钮也映射为全屏

// 输入法
w.SetIMEEnabled(false)  // 禁用输入法（默认）。游戏输入场景下避免 IME 干扰
w.SetIMEEnabled(true)   // 重新启用输入法。聊天框,文本框等需要中文输入时调用
```

### Input

轮询式输入--每帧查询按键/鼠标状态，不需要注册事件监听器。底层用 Win32 Raw Input（鼠标高精度位移）+ 标准消息（键盘状态表）。

**关键概念**：`IsKeyDown` 查询"当前是否按住"，`IsKeyPressed`/`IsKeyReleased` 查询"本帧变化"（上升沿/下降沿）。这三者的区别对实现"按一下跳一下"而非"按住连跳"至关重要。

```cpp
auto& in = core::Input::instance();

// 键盘：三态机制
in.IsKeyDown(KeyCode::W)          // 是否处于按下状态（持续触发）
in.IsKeyPressed(KeyCode::Space)    // 本帧刚按下--从"未按下"变为"按下"的那一帧才返回 true
in.IsKeyReleased(KeyCode::E)      // 本帧刚松开--从"按下"变为"未按下"的那一帧才返回 true

// 鼠标按键
in.IsMouseButtonDown(0)             // 0=左键 1=右键 2=中键 3=侧键(后退) 4=侧键(前进)
in.IsMouseButtonPressed(0)          // 和键盘同理，上升沿检测
in.IsMouseButtonReleased(0)         // 下降沿检测

// 鼠标位置与运动
in.GetMouseX(), in.GetMouseY()      // 光标在窗口客户区中的像素坐标（左上角为原点）
in.GetMouseDeltaX()                 // 本帧水平位移量（像素）。底层用 Raw Input，不受屏幕边界限制
in.GetMouseDeltaY()                 // 本帧垂直位移量
// 典型第一人称视角：yaw -= deltaX * sensitivity; pitch -= deltaY * sensitivity

// 滚轮
in.GetScrollDelta()                 // 本帧滚轮增量。正值=向上滚，负值=向下滚。一"格"约 ±1.0
// 常用：float zoom = GetScrollDelta(); 然后把相机沿视线方向移动 zoom * speed
```

### KeyCode 枚举

`sgkit::core::KeyCode`，底层映射自 Win32 虚拟键码。

```
字母:    A ~ Z
数字:    k_0 ~ k_9
控制:    Escape, Enter, Tab, Backspace, Insert, Delete
方向:    Left, Right, Up, Down
翻页:    PageUp, PageDown, Home, End
功能:    F1 ~ F25
修饰:    LeftShift/RightShift, LeftCtrl/RightCtrl, LeftAlt/RightAlt, LeftSuper/RightSuper
小键盘:  KeyPad0 ~ KeyPad9, KeyPadDecimal, KeyPadDivide, KeyPadMultiply, KeyPadSubtract, KeyPadAdd, KeyPadEnter
鼠标:    MouseButton::Left, MouseButton::Right, MouseButton::Middle, MouseButton::Back, MouseButton::Forward
符号:    Space, Comma, Minus, Period, Slash, Semicolon, Equal, LeftBracket, Backslash, RightBracket, GraveAccent
```

### FileSystem

静态文件工具类，所有方法无需实例化。底层基于 `std::ifstream`/`std::ofstream` 和 `std::filesystem`。返回值用 `std::optional`--读取失败返回 `std::nullopt`，不会抛异常。

```cpp
// 读取整个文件
auto text = FileSystem::ReadText("shader.vert");     // -> optional<string>
auto bin  = FileSystem::ReadBinary("data.bin");      // -> optional<vector<uint8_t>>
if (text) { UseString(*text); }
if (!bin)  { HandleError(); }

// 写入
FileSystem::WriteText("log.txt", "hello");             // -> bool
FileSystem::WriteBinary("out.bin", byteArray);

// 路径工具
FileSystem::Exists("path")                            // 文件/目录是否存在
FileSystem::IsDirectory("path")                       // 是否为目录
FileSystem::GetDirectory("a/b/c.txt")                 // -> "a/b"
FileSystem::GetExtension("shader.vert")               // -> "vert"（小写,无点号）
FileSystem::GetFilename("a/b/c.txt")                  // -> "c.txt"
FileSystem::GetFilenameWithoutExtension("a/b/c.txt")  // -> "c"
```

> 纹理文件格式：stb_image 支持 PNG、JPG、BMP、TGA、GIF 等常见格式。推荐使用 PNG。

### ThreadPool

固定大小线程池（单例）。通过 `Create` 指定线程数，所有工作线程在 `Destroy` 时自动回收。适用于并行 asset 加载、数据预处理等。

```cpp
// ThreadPool 生命周期由引擎管理，用户通过 instance() 使用
auto& pool = core::ThreadPool::instance();

// 提交任务--返回 TaskHandle<T>，调用方可以稍后获取结果
auto handle = pool.Enqueue([](int n) { return n * n; }, 42);
int result = handle.Get();  // 阻塞直到任务完成 -> 1764

// 检查是否完成（非阻塞）
if (handle.IsReady()) { int r = handle.Get(); }

// 批量任务
std::vector<core::TaskHandle<void>> handles;
for (auto& file : files)
    handles.push_back(pool.Enqueue([&file]() { LoadFile(file); }));

// 等待全部完成
for (auto& h : handles) h.Wait();
size_t pending = pool.PendingTasks();  // 当前排队中的任务数量
```

---

## Graphics

所有 GL 资源对象遵循 RAII--构造时分配，析构时自动调用 `glDelete*`。不可拷贝（`= delete`），支持移动。用 `std::shared_ptr` 管理生命周期。

### Shader

封装 GLSL 程序的编译,链接和 uniform 操作。失败时打印错误日志到 stderr。

```cpp
auto s = std::make_shared<Shader>();

// 从文件加载--内部走 FileSystem::ReadText
bool ok = s->LoadFromFile("assets/shaders/default.vert",
                          "assets/shaders/default.frag");

// 从字符串加载--适用于内嵌 shader,运行时生成等场景
s->LoadFromSource(vertexSrcStr, fragmentSrcStr);

s->Bind();     // glUseProgram，激活此 shader
s->Unbind();   // glUseProgram(0)
s->IsValid();  // program ID != 0（编译链接是否成功）

// Uniform 设置。内部缓存 uniform location，减少 glGetUniformLocation 调用
s->SetInt("u_Count", 5);
s->SetFloat("u_Time", 1.5f);
s->SetVector2("u_Size", {1280, 720});
s->SetVector3("u_Color", {1, 0, 0});
s->SetVector4("u_Plane", {0, 1, 0, 5});
s->SetMatrix4("u_MVP", mvp);  // 内部直接调用 glUniformMatrix4fv(..., 1, GL_FALSE, mat.Data())
```

### VertexBuffer

GPU 端的顶点数据缓冲区（VBO）。

```cpp
auto vb = std::make_shared<VertexBuffer>();

// 创建并上传数据。size 为总字节数（=顶点数*单顶点字节数）
vb->Create(vertices, sizeof(vertices));
vb->Create(vertices, sizeof(vertices), GL_DYNAMIC_DRAW);  // 指定 usage hint

vb->Bind();       // 绑定到 GL_ARRAY_BUFFER
vb->Unbind();     // 解绑
vb->SetData(newData, size, offset);  // 更新缓冲区部分数据（glBufferSubData）
vb->GetHandle();  // 返回 GL buffer ID（uint32_t）
vb->GetSize();    // 返回缓冲区字节数
vb->IsValid();    // handle != 0
```

### IndexBuffer

GPU 端的索引缓冲区（EBO），用于指定三角形顶点顺序以复用顶点数据。

```cpp
auto ib = std::make_shared<IndexBuffer>();

ib->Create(indices, 36);   // data + 索引个数。内部类型始终为 uint32_t
ib->Bind();                 // 绑定到 GL_ELEMENT_ARRAY_BUFFER
ib->Unbind();
ib->GetCount();             // 索引数量
ib->IsValid();
```

### VertexLayout

描述顶点缓冲的布局--每个属性在哪个 location,占几个元素,什么类型,偏移多少。

```cpp
VertexLayout layout;
layout.PushFloat(0, 3)   // location=0  ->  vec3 position
      .PushFloat(1, 3)   // location=1  ->  vec3 normal
      .PushFloat(2, 2);  // location=2  ->  vec2 texCoord
// 调用链式后 stride 自动计算：3*4 + 3*4 + 2*4 = 32 字节

layout.GetStride();        // 单个顶点总字节数
layout.GetAttributes();    // const vector<VertexAttribute>&
```

### VertexArray

VAO--把 VBO,布局和 IBO 绑定在一起，一个 Draw 调用即可渲染。

```cpp
auto va = std::make_shared<VertexArray>();
va->Create();                           // glGenVertexArrays

va->SetVertexBuffer(vb, layout);        // 绑定 VBO 并按布局设置顶点属性指针
va->SetIndexBuffer(ib);                 // 绑定索引缓冲

va->Bind();
va->Draw();                             // 默认 DrawMode::Triangles
va->Draw(DrawMode::Lines);             // 也可以线框
va->Draw(GL_LINES);                     // 可指定 primitive 类型
va->Unbind();
```

### Texture

2D 纹理。支持多种图片格式（PNG、JPG、BMP、TGA 等），底层使用 stb_image 解码。也支持程序化创建。

```cpp
auto tex = std::make_shared<Texture>();

// 从文件加载（PNG/JPG/BMP 等）
tex->LoadFromFile("textures/container2.png");

// 程序化创建--传入 RGBA 像素数组
std::vector<uint8_t> pixels(w * h * 4);
// ... fill pixels ...
tex->Create(w, h, pixels.data());
tex->Create(w, h, pixels.data());
tex->Create(w, h, pixels.data(), TexInternalDataFormat::RGBA8, TexDataFormat::RGBA);

// 过滤模式
tex->SetFilterLinear(true);   // 线性插值 + mipmap（默认，适合自然图像）
tex->SetFilterLinear(false);  // 邻近采样（适合像素艺术/棋盘格，边缘锐利）

// 环绕模式
tex->SetWrapRepeat(true);     // repeat -- 纹理坐标超出 [0,1] 时重复（默认）
tex->SetWrapRepeat(false);    // clamp-to-edge -- 超出部分取边缘色

tex->Bind(0);    // 绑定到 GL_TEXTURE0
tex->Bind(2);    // 绑定到 GL_TEXTURE2
tex->Unbind();
tex->GetWidth(), tex->GetHeight();
```

### Material

材质 = Shader + 纹理 + 渲染状态。控制物体的外观和 GPU 状态（混合模式、剔除面、深度写入等）。渲染状态被 Renderer 在执行 RenderQueue 时读取，决定 GL 管线如何绘制该材质。

```cpp
auto mat = std::make_shared<scene::Material>();
mat->shader   = myShader;    // 必须 - 着色器程序
mat->diffuse  = myTex;       // 可选 - 漫反射纹理（绑定到单元 0）
mat->specular = {0.5f, 0.5f, 0.5f};
mat->shininess = 32.0f;

// 渲染状态（决定"怎么画"）
mat->blendMode = BlendMode::Opaque;      // Opaque / AlphaBlend / Additive
mat->cullMode  = CullMode::Back;         // Back / Front / None
mat->depthMode = DepthMode::ReadWrite;   // ReadWrite / ReadOnly / None
mat->renderQueue = 0;                    // 同混合模式内的排序偏移（越大越靠后画）
```

**渲染状态说明**：

| 枚举 | 值 | 含义 |
|------|-----|------|
| `BlendMode::Opaque` | 不透明 | 关闭混合，写入深度 |
| `BlendMode::AlphaBlend` | Alpha 混合 | `glBlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA)`，自动进入透明队列 |
| `BlendMode::Additive` | 叠加混合 | `glBlendFunc(SRC_ALPHA, ONE)`，适合粒子/光晕 |
| `CullMode::Back` | 剔除背面 | 默认，逆时针面不渲染 |
| `CullMode::Front` | 剔除正面 | 用于翻转几何体 |
| `CullMode::None` | 双面渲染 | 树叶、纸片等 |
| `DepthMode::ReadWrite` | 读写深度 | 默认 |
| `DepthMode::ReadOnly` | 只读深度 | 粒子等不写深度的物体 |
| `DepthMode::None` | 无深度测试 | UI 等始终绘制在最上层的元素 |

Material 不再有 `Apply()` 方法 - 渲染时的状态绑定由 `Renderer::ExecuteBatch()` 内部处理。

### Mesh

纯数据容器 - 把 VAO（几何）和 Material（外观）打包在一起。不再有 `Render()` 方法，渲染逻辑已移至 Renderer。

```cpp
auto mesh = std::make_shared<Mesh>();
mesh->vertexArray = va;   // 共享的几何数据
mesh->material    = mat;  // 共享的材质
// 就这样 - 没有 Render() 方法
```

Mesh 只是一个"标签"，表示"用这种材质画这个几何"。多个 Entity 可以共享同一个 Mesh（引用相同的 VAO + Material），引擎内部会按 (Shader, Material, VAO) 自动合并为同一批次，减少 GL 状态切换。

### RenderQueue

由 Scene 构建、Renderer 执行的渲染命令队列。按 (Shader -> Material -> VAO) 分组为 Batch，执行时每条 Batch 只需一次状态切换。

用户一般不直接操作 RenderQueue，但底层 graphics 用户可手动构建：

```cpp
scene::RenderQueue queue;

// 提交网格实例（自动合并同 Shader+Material+VAO 的实例）
queue.Submit(mesh1, worldMatrix1);
queue.Submit(mesh1, worldMatrix2);  // 同 mesh -> 合并到同批次
queue.Submit(mesh2, worldMatrix3);  // 不同 shader -> 新批次

// 排序：opaque 按材质键排序，transparent 按深度排序
queue.Sort(cameraPos);

// 交给 Renderer 执行
renderer.Execute(queue);
```

**两队列设计**：
- `GetOpaqueBatches()` - 不透明队列，按 (Shader*, Material*, VAO*) 排序以最小化状态切换
- `GetTransparentBatches()` - 半透明队列（BlendMode 非 Opaque 的材质），按深度从远到近排序确保混合正确

### Renderer

全局渲染器，管理清屏、视口、GL 状态，以及帧级渲染数据（摄像机、光源）。通过 `scene::Renderer::instance()` 获取。

```cpp
auto& r = scene::Renderer::instance();

// -- 清屏与视口
r.SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
r.Clear();                                // 清除颜色 + 深度缓冲
r.SetViewport(0, 0, w, h);

// -- GL 状态控制（底层 graphics 用户使用）
r.SetDepthTest(true);   // 深度测试开关
r.SetCullFace(true);    // 背面剔除开关
r.SetBlend(true);       // 混合开关
r.SetWireframe(true);   // 线框模式

// -- 帧级数据设置（替代旧的 RenderContext）
r.SetViewProjection(viewProj);             // 视图×投影矩阵
r.SetCameraPosition(cameraPos);            // 摄像机世界坐标
r.SetAmbientLight({0.1f, 0.1f, 0.15f});  // 环境光颜色
r.SetLights(lightDataVec);                // 多光源数据（见 LightData）

// -- 执行渲染队列（Scene 用户通常不直接调用）
r.Execute(queue);     // 两 pass：opaque -> transparent

// -- 底层绘制（绕过 Scene，直接用 graphics）
r.Draw(va);           // 直接绘制 VAO（不设置任何 uniform/状态）
```

**两 Pass 渲染流程** (`Execute` 内部)：

| Pass | 深度缓冲 | 混合 | 顺序 |
|------|----------|------|------|
| Opaque | 读写 | 关闭 | 按 (Shader, Material, VAO) 排序 -> 最小化状态切换 |
| Transparent | 只读 | 开启 | 按深度从远到近排序 -> 确保半透明混合正确 |

每 Batch 内：ApplyBatchState（应用材质的 blend/cull/depth）-> SetFrameUniforms -> 逐个实例 set u_Model + Draw。

**LightInstance** - Renderer 使用的光源数据结构（定义在 `scene::Renderer` 中）：
```cpp
scene::Renderer::LightInstance light;
light.position = {0.8f, 1.0f, 1.2f};
light.ambient  = {0.2f, 0.2f, 0.2f};
light.diffuse  = {0.5f, 0.5f, 0.5f};
light.specular = {1.0f, 1.0f, 1.0f};
// 传给 Renderer: renderer.SetLights({light1, light2, ...});
```

Shader 中通过 `u_Light.position/ambient/diffuse/specular` 访问当前光源。多光源接口已预留，当前默认单光源模式。

### FrameBuffer

离屏渲染目标。当前实现为深度专用 FBO（用于阴影贴图），后续可扩展颜色附件。

```cpp
FrameBuffer fbo;
bool ok = fbo.Create(2048, 2048);    // 创建 2048×2048 深度纹理 FBO
// 内部：GL_DEPTH_COMPONENT + GL_FLOAT，NEAREST 过滤，CLAMP_TO_BORDER 环绕（边界值=1.0）

fbo.IsValid();          // FBO 是否完整可用
fbo.Bind();             // 绑定--后续渲染写入此 FBO 的深度缓冲
  // ... 用 depth-only shader 渲染场景 ...
fbo.Unbind();           // 回到默认帧缓冲
fbo.GetDepthTexture();  // 获取深度纹理的 GL ID，可在 shader 中用 sampler2D 采样
```

---

## Graphics 枚举

OpenGL 常量已封装为 `enum class`，用户不需要包含 `<glad/glad.h>`。

### TexInternalDataFormat

```cpp
Alpha, RGB, R3_G3_B2, RGB4, RGB5, RGB8, RGB10, RGB12, RGB16,
RGBA, RGB5_A1, RGBA8, RGB10_A2, RGBA12, RGBA16
```
用于 `Texture::Create()` 的 GPU 内部存储格式。默认 `RGBA8`。

### TexDataFormat

```cpp
Alpha, RGB, RGBA
```
像素数据的通道排列。默认 `RGBA`。

### AttribType

```cpp
Byte, UnsignedByte, Short, UnsignedShort, Int, UnsignedInt, Float
```
`VertexLayout::Push()` 的顶点属性数据类型。`PushFloat` 隐含 `Float`，`PushUInt` 隐含 `UnsignedInt`。

### DrawMode

```cpp
Points, Lines, LineLoop, LineStrip, Triangles, TriangleStrip, TriangleFan
```
`VertexArray::Draw()` 的渲染模式。默认 `Triangles`。

### Usage

```cpp
// VertexBuffer::Create() 的 usage hint（指定 GPU 如何使用数据）
```
内部映射到 `GL_STATIC_DRAW` / `GL_DYNAMIC_DRAW` 等。

---

## Scene

ECS（实体-组件系统），稀疏集存储--增删 O(1)，遍历组件线性时间。4 种内置组件：Transform,Camera,Light,MeshRenderer。

### Entity 与 Scene

```cpp
auto& scene = scene::Scene::instance();

Entity e = scene.CreateEntity();   // 分配实体 ID，加入活跃列表。上限 k_MaxEntities=10000
scene.DestroyEntity(e);            // 移除该实体的所有组件，从活跃列表删除
scene.IsAlive(e);                  // 实体是否存活

scene.SetVisible(e, false);        // 递归设置该实体及所有子实体的 MeshRenderer.enabled
                                   //   常用于整体显隐一个模型（配合 Model::Load 的 root）

scene::Entity::Invalid  // 无效实体标识（用 e == scene::Entity::Invalid 判断）
```

### 组件操作

```cpp
// 添加组件。已存在则返回已有引用
auto& tf  = scene.AddComponent<scene::component::Transform>(entity);
auto& cam = scene.AddComponent<scene::component::Camera>(entity);
auto& lt  = scene.AddComponent<scene::component::Light>(entity);
auto& mr  = scene.AddComponent<scene::component::MeshRenderer>(entity);

// 获取（返回指针，不存在返回 nullptr）
auto* t = scene.GetComponent<scene::component::Transform>(entity);
if (t) { t->position.x += 1.0f; }

// 检查是否存在
if (scene.HasComponent<scene::component::Camera>(entity)) { ... }

// 移除
scene.RemoveComponent<scene::component::Light>(entity);
```

**技术细节**：每个组件类型对应一个 `ComponentPool<T>`（稀疏集）。Add 时把实体映射到 dense 数组尾部，Remove 时 swap-and-pop 尾部元素。因此遍历效率高（dense 数组连续），但 Entity ID 不能无限大（稀疏数组预留上限）。

### scene\::component\::Transform

有层级关系的世界变换组件。每个 Transform 可指一个 parent 和若干 children。

```cpp
auto& tf = scene.AddComponent<scene::component::Transform>(entity);
tf.position = {0, 2, 0};
tf.rotation = Quaternion::FromEulerAngles(0, y, 0);  // Y 轴旋转 y 弧度
tf.scale    = {0.8f, 0.8f, 0.8f};

// 建立父子关系
tf.parent = parentEntity;
parentTf.children.push_back(entity);

// 变换矩阵
tf.GetLocalMatrix();               // 仅自身的 Scale->Rotate->Translate
scene.GetWorldMatrix(entity);      // 考虑父子层级累乘的世界矩阵
```

**Transform 更新**：引擎主循环每帧调用 `scene.RecomputeWorldTransforms()`，算法：先重置所有实体为 local 矩阵，然后从根到叶迭代累乘 parent * local，直到稳定（最多 100 轮迭代）。

### scene\::component\::Camera

场景摄像机。渲染时选取一个 camera entity 传给 `scene.Render(cameraEntity)`。

```cpp
auto& cam = scene.AddComponent<scene::component::Camera>(entity);
cam.fovY      = 60.0f;    // 垂直视场角（度），透视投影参数
cam.nearPlane = 0.1f;     // 近裁剪平面（<此距离的物体不可见）
cam.farPlane  = 1000.0f;  // 远裁剪平面（>此距离的物体不可见）

// 内部使用（Render 自动调用）
cam.GetViewMatrix(worldMatrix);          // world 矩阵的逆 -> 视图矩阵
cam.GetProjectionMatrix(aspectRatio);   // 透视投影矩阵
```

### scene\::component\::Light

光源组件。`Scene::Render()` 调用 `CollectLights()` 遍历所有 Light 组件，全部传给 Renderer。支持三种光源类型。

```cpp
auto& light = scene.AddComponent<scene::component::Light>(entity);

// 类型（默认 Point）
light.type = scene::component::Light::Type::Point;        // 点光源：从位置向四周衰减
light.type = scene::component::Light::Type::Directional;  // 平行光：只有方向，无衰减（如太阳）
light.type = scene::component::Light::Type::SpotLight;    // 聚光灯：锥形光束

// 颜色分量
light.ambient  = {0.2f, 0.2f, 0.2f};  // 环境光分量
light.diffuse  = {0.5f, 0.5f, 0.5f};  // 漫反射分量
light.specular = {1.0f, 1.0f, 1.0f};  // 镜面反射分量

// 方向（Directional / SpotLight 使用；Point 忽略）
light.direction = {0.0f, -1.0f, 0.0f};

// 聚光灯锥角（余弦值，内锥 > 外锥；SpotLight 使用）
light.cutOff      = 0.91f;   // 内锥余弦，锥内全亮
light.outerCutOff = 0.82f;   // 外锥余弦，内外锥之间平滑过渡

// 距离衰减系数（Point / SpotLight 使用）：atten = 1/(constant + linear*d + quadratic*d²)
light.constant  = 1.0f;
light.linear    = 0.09f;
light.quadratic = 0.032f;
// 位置从 Transform 组件读取：Transform::position
```

有 Light 组件的实体通常也需要 Transform 组件（提供世界坐标位置）。引擎每帧调用 `Scene::CollectLights()` 将所有活跃光源收集为 `vector<LightInstance>`，传给 Renderer 后在 Shader 中访问。

### scene\::component\::MeshRenderer

给实体附加渲染网格。

```cpp
auto& mr = scene.AddComponent<scene::component::MeshRenderer>(entity);
mr.mesh    = myMesh;   // shared_ptr<Mesh>
mr.enabled = true;     // 设为 false 则跳过渲染
```

### scene\::Model - 模型导入

`scene::Model::Load()` 通过 assimp 一次性导入 3D 模型文件，自动创建实体层级：一个 root 实体（仅 Transform）+ 每个子网格一个实体（Transform + MeshRenderer，作为 root 的子节点）。支持 OBJ、FBX、GLB、glTF、DAE、3DS、PLY、STL、Blend 等格式。

```cpp
#include <sgkit/sgkit.h>  // 已含 scene/Model.h

// 加载模型，所有子网格共用传入的 shader
auto shader = std::make_shared<graphics::Shader>();
shader->LoadFromFile("assets/shaders/light.vert", "assets/shaders/light.frag");

scene::Model::Result model = scene::Model::Load("assets/backpack/backpack.obj", shader);
if (model.root == scene::Entity::Invalid)   // 加载失败
    return false;

// Result 结构
model.root       // Entity - 仅 Transform，移动/缩放/显隐/销毁整个模型的把手
model.entities   // vector<Entity> - 每个子网格一个实体（Transform + MeshRenderer）

// 设置整体变换（作用于 root，子网格随层级累乘）
auto* tf = scene.GetComponent<scene::component::Transform>(model.root);
tf->position = {0, 0, 0};
tf->scale    = {1.5f, 1.5f, 1.5f};

// 整体显隐（递归所有子网格）
scene.SetVisible(model.root, false);
```

**导入细节**：
- 顶点布局固定为 `[position(3) normal(3) texCoord(2)]`（location 0/1/2）。缺失法线时自动生成平滑法线（`aiProcess_GenSmoothNormals`），缺失 UV 时填 `(0,0)`。
- 后处理：三角化 + 合并相同顶点。非 `.glb`/`.gltf` 格式自动翻转 UV（`aiProcess_FlipUVs`）。
- 纹理：读取 assimp 材质的 DIFFUSE（回退到 BASE_COLOR）绑定到单元 0、SPECULAR 绑定到单元 1。外部纹理按模型所在目录相对路径加载；内嵌纹理（`*0` 形式）直接解码或临时落盘后加载。同一路径的纹理在一次 Load 内缓存复用。
- 材质 `shininess` 固定为 32.0f。当前光照为 Phong，尚未支持 PBR。

> 需要更精细控制（逐网格换材质、调整包围盒等）时，可遍历 `model.entities` 逐个 `GetComponent<MeshRenderer>()` 修改。

### Render - 渲染流程

`scene.Render(cameraEntity)` 内部执行：

1. **BuildRenderQueue()** - 遍历所有 MeshRenderer 组件，按 (Shader*, Material*, VAO*) 三元组将实体分组为 RenderBatch。相同组的实体合并到同一批次
2. **Sort()** - Opaque 批次按材质键排序以最小化 GL 状态切换；Transparent 批次按到摄像机的深度从远到近排序
3. **CollectLights()** - 遍历所有 Light 组件，收集为 `vector<LightInstance>`
4. 设置 Renderer 帧数据（ViewProjection、CameraPosition、Lights）
5. **renderer.Clear()** -> **renderer.Execute(queue)** - 两 pass 渲染

**用户在自己的 `onRender` 中调用**：
```cpp
cfg.onRender = []() {
    scene::Scene::instance().Render(cameraEntity);
};
```

**高级用法**：用户可以手动控制每一步：
```cpp
// 手动构建队列（可在 Submit 前做 culling）
scene::RenderQueue queue = scene.BuildRenderQueue();
queue.Sort(cameraPos);

auto& renderer = scene::Renderer::instance();
renderer.SetViewProjection(vp);
renderer.SetLights(scene.CollectLights());
renderer.Clear();
renderer.Execute(queue);
```

也可以在完全不使用 Scene 的情况下直接用 scene 层渲染：
```cpp
scene::RenderQueue queue;
queue.Submit(myMesh, worldMatrix);
queue.Sort(cameraPos);

scene::Renderer::instance().SetViewProjection(vp);
scene::Renderer::instance().Execute(queue);
```

---

## 项目集成

SGKit 为静态库，构建产物在 `lib/` 目录：`sgkit_d.lib`（Debug，含 `sgkit_d.pdb`）和 `sgkit.lib`（Release）。

**方式一：链接预编译库**（不需要源码，只需 `include/` + `lib/`）：

```cmake
target_include_directories(YourApp PRIVATE SGKit/include)
target_link_directories(YourApp PRIVATE SGKit/lib)
target_link_libraries(YourApp PRIVATE "$<$<CONFIG:Debug>:sgkit_d>$<$<CONFIG:Release>:sgkit>")
```

**方式二：源码集成**（可修改引擎）：

```cmake
add_subdirectory(external/SGKit)
target_link_libraries(YourApp PRIVATE sgkit)
```

SGKit 会传递链接 `glad` 和平台库 (`gdi32 user32 opengl32 imm32`)。assimp 以预编译库（`assimp-vc145-mt.lib`）在库内部 PRIVATE 链接，若其为动态库形式，最终可执行文件运行时需能找到对应的 `assimp-vc145-mt.dll`。

### 目录结构

```
SGKit/
├-- CMakeLists.txt / CMakePresets.json
├-- external/
│   ├-- glad/                  # OpenGL 4.6 Core 加载器（静态库）
│   ├-- stb/                   # stb_image 纹理加载（静态库）
│   └-- assimp/                # 3D 模型导入（预编译 lib + 头文件）
├-- include/sgkit/             # 公共头（聚合头 sgkit.h 一次性包含全部子模块）
│   ├-- math/      Vector2/3/4, Matrix4, Quaternion, MathUtils
│   ├-- core/      Window, Input, KeyCodes, FileSystem, ThreadPool
│   ├-- graphics/  Shader, VertexBuffer, IndexBuffer, VertexLayout, VertexArray,
│   │              Texture, FrameBuffer（纯 GL 原子 RAII 包装）
│   ├-- scene/     Entity, ComponentPool, Components(Transform/Camera/Light/MeshRenderer),
│   │              Material, Mesh, Model, RenderQueue, Renderer, Scene
│   └-- framework/ Application(Config), Timing
├-- src/                       # 实现（按模块对应）
├-- examples/                  # 演示示例
├-- tests/                     # 单元测试
├-- lib/                       # 预编译产物
│   ├-- sgkit_d.lib + .pdb     #   Debug
│   └-- sgkit.lib              #   Release
└-- icon/                      # 程序图标（app.rc + app.ico）
```

### 全局预定义宏（MSVC）

| 宏 | 值 | 说明 |
|----|-----|------|
| `_WINDOWS` | - | GUI 子系统标识 |
| `UNICODE` / `_UNICODE` | - | Windows API + CRT 宽字符 |
| `NOMINMAX` | - | 阻止 `<windows.h>` 定义 `min`/`max` 宏污染 `std::` |
| `WINVER` / `_WIN32_WINNT` | `0x0A00` | 最低调用 Windows 10 API |
| `_DEBUG` | Debug only (MSVC 自动) | 条件启用调试代码 + 控制台 + GL 回调 |
| `SGK_PLATFORM_WINDOWS` | 库 PRIVATE | 库内部平台区分（用户代码不可见） |

### Shader 编写约定

- GLSL 版本：`#version 330 core`
- 顶点属性 location：`0=Position(vec3), 1=Normal(vec3), 2=TexCoord(vec2)`
- 矩阵 uniform 用 `SetMatrix4(name, mat)`，内部调用 `glUniformMatrix4fv(..., 1, GL_FALSE, mat.Data())`
- 纹理采样器通过 `SetInt(name, slot)` 指向 `GL_TEXTURE0 + slot`，配合 `texture.Bind(slot)`
- 引擎自动设置的 uniform（RenderQueue 执行时绑定）：
  - `u_Model` - 模型矩阵（每实例）
  - `u_ViewProjection` - 视图×投影矩阵（每帧）
  - `u_cameraPos` - 摄像机世界位置（每帧）
  - `u_Light.position/ambient/diffuse/specular` - 光源数据（每帧，单光源模式）
  - `u_Material.diffuse` - 漫反射纹理（纹理单元 0）
  - `u_Material.specular` - 镜面反射纹理（纹理单元 1）
  - `u_Material.shininess` - 光泽度（每批次）
