# SGKit 用户手册

SGKit（Straightforward Graphics Kit）是一个 C++ 3D 渲染引擎。核心依赖仅有 glad（OpenGL 加载器），其余全部基于 C++ 标准库 + Win32 平台 API。列主序矩阵、RAII 管理 GL 对象、稀疏集 ECS。

---

## 目录

1. [Hello World](#hello-world)
2. [Framework — 应用框架](#framework)
3. [Math — 数学库](#math)
4. [Core — 核心模块](#core)
5. [Graphics — 图形模块](#graphics)
6. [Scene — 场景模块](#scene)
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
        // 初始化：创建场景、加载资源
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
    cfg.onShutdown = []() { /* 清理 */ };
    return cfg;
}
```

不需要继承类、不需要写 `WinMain`、不需要 `#include <windows.h>`。引擎内部已包含 `WinMain`，它负责创建窗口、加载 OpenGL、初始化输入、然后调用你的回调。四个回调中可以通过 `GetWindow()`、`GetInput()`、`GetRenderer()`、`GetScene()` 访问引擎模块。

---

## Framework

Framework 是引擎的入口层，把平台细节（`WinMain`、消息循环、控制台重定向）封装在库内部，向用户暴露简单明了的四个回调。

### ApplicationConfig

配置结构体，所有字段都有默认值。

| 字段 | 类型 | 默认 | 作用 |
|------|------|------|------|
| `title` | `std::string` | `"SGKit"` | 窗口标题（UTF-8，内部自动转 UTF-16 传给 `CreateWindowExW`） |
| `width` | `int` | `1280` | 窗口客户区宽度（像素） |
| `height` | `int` | `720` | 窗口客户区高度（像素） |
| `vsync` | `bool` | `true` | 是否垂直同步——开启后帧率锁定到显示器刷新率，防止画面撕裂 |
| `fullscreen` | `bool` | `false` | 若为 `true`，窗口创建后立即进入无边框全屏模式；此后最大化按钮也自动变为全屏 |
| `glMajor` | `int` | `4` | 请求的 OpenGL 主版本号 |
| `glMinor` | `int` | `6` | 请求的 OpenGL 次版本号——引擎依次尝试 4.6 → 3.3 → Legacy，只要任一成功即启动 |
| `onInit` | `function<bool()>` | — | 引擎完成窗口+GL+输入初始化后调用。**返回 `false` 表示初始化失败，程序退出** |
| `onUpdate` | `function<void(float)>` | — | 每帧调用一次，参数 `dt` 为上一帧到本帧的间隔（秒）。`dt` 值与 `GetDeltaTime()` 完全相同，只是省去手动获取 |
| `onRender` | `function<void()>` | — | 每帧调用一次，紧接在 `onUpdate` 之后。通常在此调用 `GetScene().OnRender(...)` |
| `onShutdown` | `function<void()>` | — | 窗口关闭后、引擎销毁前调用一次，用于清理用户资源 |

### 引擎全局函数

这些函数在回调中任意位置均可调用，它们访问的是引擎内部维护的全局单例。

| 函数 | 返回类型 | 作用 |
|------|----------|------|
| `GetWindow()` | `core::Window&` | 获取窗口对象，可查询尺寸、设置全屏、关闭窗口等 |
| `GetInput()` | `core::Input&` | 获取输入对象，可轮询键盘/鼠标/滚轮状态 |
| `GetRenderer()` | `graphics::Renderer&` | 获取渲染器，可清屏、切换状态（线框、深度测试等） |
| `GetScene()` | `scene::Scene&` | 获取场景对象，所有实体和组件的容器 |
| `GetDeltaTime()` | `float` | 返回当前帧间隔（秒）。和 `onUpdate` 的 `dt` 参数值相同，方便在 `onRender` 或其他函数中使用 |
| `GetFPS()` | `float` | 返回每秒帧数，每秒更新一次 |
| `RequestQuit()` | `void` | 请求退出主循环。调用后当前帧会完整执行完毕，然后程序正常退出 |

---

## Math

`sgkit::math` 命名空间。所有类型都是 Plain Struct 值类型，可直接拷贝，没有虚函数。**Matrix4 使用列主序存储**，`Data()` 返回的指针可直接传给 `glUniformMatrix4fv(loc, 1, GL_FALSE, ptr)`。

### MathUtils

```cpp
k_Pi          // π = 3.1415927f
k_TwoPi       // 2π
k_HalfPi      // π/2
k_Epsilon     // 1e-5f，浮点判等容差
k_Deg2Rad     // 度转弧度因子 (π/180)
k_Rad2Deg     // 弧度转度因子 (180/π)

ToRadians(90)   // → 1.5708  角度转弧度
ToDegrees(π)    // → 180     弧度转角度

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
Vector3::Cross(a, b)        // 叉乘。a×b —— 结果同时垂直于 a 和 b
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
m.Inverse()         // 返回逆矩阵（退化时返回单位阵而非 NaN）
m.Determinant()     // 行列式

// --- 矩阵乘法 ---
Matrix4 c = a * b;              // 矩阵乘法，用于组合变换

// --- 变换向量 ---
Vector4 v4 = m * Vector4{x,y,z,w};    // 矩阵 × 列向量
Vector3 p2 = m.TransformPoint(p);     // 变换点：等价于 (m * vec4(p,1)).xyz / w
Vector3 d2 = m.TransformDirection(d); // 变换方向：等价于 (m * vec4(d,0)).xyz（平移被忽略）

// --- 静态工厂（返回新矩阵，不依赖已有实例）---
Matrix4::Identity()
Matrix4::Translate(v)       // → 平移矩阵
Matrix4::RotateX/Y/Z(rad)   // → 旋转矩阵
Matrix4::Scale(v)           // → 缩放矩阵
Matrix4::Perspective(fovY_rad, aspect, near, far)
Matrix4::LookAt(eye, target, up)
```

**常见用法**：`Matrix4 mvp = proj * view * model;`，结果传给 `shader->SetMatrix4("u_MVP", mvp);`

### Quaternion

四元数表示三维旋转——避免欧拉角的万向节锁问题。单位四元数（`Length()≈1`）才能正确表示旋转。

```cpp
Quaternion q;                          // 默认 = Identity: (0,0,0,1)
Quaternion q{x, y, z, w};              // 从四分量构造

q * q2                                 // 四元数乘法 = 旋转组合（先 q 后 q2）
q * Vector3                            // 用四元数旋转向量

q.Length(), q.LengthSquared()          // 长度
q.Normalize(), q.Normalized()          // 归一化（旋转前应确保单位长度）
q.Conjugate(), q.Conjugated()          // 共轭 = (-x, -y, -z, w)
q.Invert(), q.Inverse()               // 逆四元数 = 共轭/长度²
q.Dot(q2)                              // 点乘

q.ToEulerAngles()                      // 转换为欧拉角 (pitch, yaw, roll)，单位弧度

// 静态工厂
Quaternion::Identity()
Quaternion::FromEulerAngles(pitch, yaw, roll)  // 从欧拉角创建（弧度）
Quaternion::FromAxisAngle(axis, radians)       // 绕任意轴旋转
Quaternion::Slerp(a, b, t)                     // 球面线性插值——两个朝向之间平滑过渡，t∈[0,1]
Quaternion::LookAt(direction, up)              // 朝向向量→四元数
```

### math::Transform

纯 CPU 的 TRS 变换结构体，与 Scene 无关。

```cpp
Transform tf;
tf.position = {0, 0, 0};        // 位移
tf.rotation = Quaternion{};      // 旋转（默认朝向）
tf.scale    = {1, 1, 1};        // 缩放
Matrix4 local = tf.GetLocalMatrix();  // 计算 Scale→Rotate→Translate 矩阵
// 等价于 Matrix4::Translate(pos) * rotation矩阵 * Matrix4::Scale(s)
```

---

## Core

### Window

窗口与 OpenGL 上下文的封装。由引擎创建和销毁，用户通过 `GetWindow()` 引用。

**背景**：PIMPL 模式把 Win32 类型（`HWND`、`HDC`、`HGLRC`）隐藏在不透明 `Impl` 中，用户代码不需要包含 `<windows.h>`。

```cpp
auto& w = GetWindow();

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
w.SetFullscreen(false)  // 退出全屏：恢复之前保存的样式、位置、尺寸
// 引擎自动处理：全屏状态下按 Esc 自动还原。若 fullscreen=true，标题栏最大化按钮也映射为全屏

// 输入法
w.SetIMEEnabled(false)  // 禁用输入法（默认）。游戏输入场景下避免 IME 干扰
w.SetIMEEnabled(true)   // 重新启用输入法。聊天框、文本框等需要中文输入时调用
```

### Input

轮询式输入——每帧查询按键/鼠标状态，不需要注册事件监听器。底层用 Win32 Raw Input（鼠标高精度位移）+ 标准消息（键盘状态表）。

**关键概念**：`IsKeyDown` 查询"当前是否按住"，`IsKeyPressed`/`IsKeyReleased` 查询"本帧变化"（上升沿/下降沿）。这三者的区别对实现"按一下跳一下"而非"按住连跳"至关重要。

```cpp
auto& in = GetInput();

// 键盘：三态机制
in.IsKeyDown(KeyCode::k_W)          // 是否处于按下状态（持续触发）
in.IsKeyPressed(KeyCode::k_Space)    // 本帧刚按下——从"未按下"变为"按下"的那一帧才返回 true
in.IsKeyReleased(KeyCode::k_E)      // 本帧刚松开——从"按下"变为"未按下"的那一帧才返回 true

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
字母:    k_A ~ k_Z
数字:    k_0 ~ k_9
控制:    k_Escape, k_Enter, k_Tab, k_Backspace, k_Insert, k_Delete
方向:    k_Left, k_Right, k_Up, k_Down
翻页:    k_PageUp, k_PageDown, k_Home, k_End
功能:    k_F1 ~ k_F25
修饰:    k_LeftShift/RightShift, k_LeftCtrl/RightCtrl, k_LeftAlt/RightAlt, k_LeftSuper/RightSuper
小键盘:  k_KeyPad0 ~ k_KeyPad9, k_KeyPadDecimal, k_KeyPadDivide, k_KeyPadMultiply, k_KeyPadSubtract, k_KeyPadAdd, k_KeyPadEnter
鼠标:    k_MouseLeft, k_MouseRight, k_MouseMiddle, k_MouseButton4, k_MouseButton5
符号:    k_Space, k_Comma, k_Minus, k_Period, k_Slash, k_Semicolon, k_Equal, k_LeftBracket, k_Backslash, k_RightBracket, k_GraveAccent
```

### FileSystem

静态文件工具类，所有方法无需实例化。底层基于 `std::ifstream`/`std::ofstream` 和 `std::filesystem`。返回值用 `std::optional`——读取失败返回 `std::nullopt`，不会抛异常。

```cpp
// 读取整个文件
auto text = FileSystem::ReadText("shader.vert");     // → optional<string>
auto bin  = FileSystem::ReadBinary("data.bin");      // → optional<vector<uint8_t>>
if (text) { UseString(*text); }
if (!bin)  { HandleError(); }

// 写入
FileSystem::WriteText("log.txt", "hello");             // → bool
FileSystem::WriteBinary("out.bin", byteArray);

// 路径工具
FileSystem::Exists("path")                             // 文件/目录是否存在
FileSystem::IsDirectory("path")                        // 是否为目录
FileSystem::GetDirectory("a/b/c.txt")                  // → "a/b"
FileSystem::GetExtension("shader.vert")                // → "vert"（小写、无点号）
FileSystem::GetFilename("a/b/c.txt")                   // → "c.txt"
FileSystem::GetFilenameWithoutExtension("a/b/c.txt")  // → "c"

// 资产路径解析
FileSystem::SetAssetDirectory("data/");                // 设置资产根目录
FileSystem::GetAssetPath("textures/wood.bmp")          // → "data/textures/wood.bmp"
```

### ThreadPool

固定大小线程池。构造时启动 N 个工作线程；析构时自动等待所有任务完成并回收线程。适用于并行 asset 加载、数据预处理等。

```cpp
ThreadPool pool;                             // 线程数 = hardware_concurrency()
ThreadPool pool(4);                          // 明确指定 4 线程

// 提交任务——返回 future，调用方可以稍后获取结果
auto f = pool.Enqueue([](int n) { return n * n; }, 42);
int result = f.get();  // 阻塞直到任务完成 → 1764

// 批量任务
std::vector<std::future<void>> jobs;
for (auto& file : files)
    jobs.push_back(pool.Enqueue([&file]() { LoadFile(file); }));

pool.WaitAll();          // 阻塞直到队列全部完成
size_t pending = pool.PendingTasks();  // 当前排队中的任务数量
```

---

## Graphics

所有 GL 资源对象遵循 RAII——构造时分配，析构时自动调用 `glDelete*`。不可拷贝（`= delete`），支持移动。用 `std::shared_ptr` 管理生命周期。

### Shader

封装 GLSL 程序的编译、链接和 uniform 操作。失败时打印错误日志到 stderr。

```cpp
auto s = std::make_shared<Shader>();

// 从文件加载——内部走 FileSystem::ReadText
bool ok = s->LoadFromFile("assets/shaders/default.vert",
                          "assets/shaders/default.frag");

// 从字符串加载——适用于内嵌 shader、运行时生成等场景
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

描述顶点缓冲的布局——每个属性在哪个 location、占几个元素、什么类型、偏移多少。

```cpp
VertexLayout layout;
layout.PushFloat(0, 3)   // location=0  →  vec3 position
      .PushFloat(1, 3)   // location=1  →  vec3 normal
      .PushFloat(2, 2);  // location=2  →  vec2 texCoord
// 调用链式后 stride 自动计算：3*4 + 3*4 + 2*4 = 32 字节

layout.GetStride();        // 单个顶点总字节数
layout.GetAttributes();    // const vector<VertexAttribute>&
```

### VertexArray

VAO——把 VBO、布局和 IBO 绑定在一起，一个 Draw 调用即可渲染。

```cpp
auto va = std::make_shared<VertexArray>();
va->Create();                           // glGenVertexArrays

va->AddVertexBuffer(vb, layout);        // 绑定 VBO 并按布局设置顶点属性指针
va->SetIndexBuffer(ib);                 // 绑定索引缓冲

va->Bind();
va->Draw();                             // glDrawElements(GL_TRIANGLES, ib->GetCount(), ...)
va->Draw(GL_LINES);                     // 可指定 primitive 类型
va->Unbind();
```

### Texture

2D 纹理。支持 BMP 加载（24/32-bit 无压缩）和程序化创建。

```cpp
auto tex = std::make_shared<Texture>();

// BMP 加载
tex->LoadFromFile("textures/wood.bmp");

// 程序化创建——传入 RGBA 像素数组
std::vector<uint8_t> pixels(w * h * 4);
// ... fill pixels ...
tex->Create(w, h, pixels.data());
tex->Create(w, h, pixels.data(), GL_RGBA8, GL_RGBA);  // 指定内部格式和输入格式

// 过滤模式
tex->SetFilterLinear(true);   // 线性插值 + mipmap（默认，适合自然图像）
tex->SetFilterLinear(false);  // 邻近采样（适合像素艺术/棋盘格，边缘锐利）

// 环绕模式
tex->SetWrapRepeat(true);     // repeat —— 纹理坐标超出 [0,1] 时重复（默认）
tex->SetWrapRepeat(false);    // clamp-to-edge —— 超出部分取边缘色

tex->Bind(0);    // 绑定到 GL_TEXTURE0
tex->Bind(2);    // 绑定到 GL_TEXTURE2
tex->Unbind();
tex->GetWidth(), tex->GetHeight();
```

### Material

材质 = Shader + 纹理 + 颜色/反光度参数。控制物体对光照的响应程度。

```cpp
auto mat = std::make_shared<Material>();
mat->shader         = myShader;          // 必须——着色器程序
mat->diffuseTexture = myTex;            // 可选——漫反射纹理

mat->ambientColor  = {0.2f, 0.2f, 0.2f};  // 环境光反射率（暗面基底颜色）
mat->diffuseColor  = {0.8f, 0.8f, 0.8f};  // 漫反射率（亮面主色）
mat->specularColor = {1.0f, 1.0f, 1.0f};  // 高光颜色
mat->shininess     = 64.0f;                // 高光锐度——越大高光越集中越小
mat->opacity       = 1.0f;                 // 不透明度

mat->Apply();  // 绑定 shader + 上传 material uniform + 绑定纹理到单元 0
// 通常不直接调用 Apply()，而是通过 Mesh::Render() 间接调用
```

### Mesh

渲染单元 = VAO（几何数据）+ Material（外观）。

```cpp
auto mesh = std::make_shared<Mesh>();
mesh->vertexArray = va;
mesh->material    = mat;

mesh->Render();  // = material->Apply() + vertexArray->Draw()
```

### Renderer

全局渲染器，管理清屏、视口和 GL 状态开关。通过 `GetRenderer()` 获取。

```cpp
auto& r = GetRenderer();

r.SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});  // 设置清屏色（RGBA）
r.Clear();                                      // 清除颜色 + 深度缓冲

r.SetViewport(0, 0, w, h);                      // 设置渲染视口（左下角 + 宽高）

// GL 状态控制
r.SetDepthTest(true);   // 开启深度测试（默认）——物体前后遮挡正确
r.SetDepthTest(false);  // 关闭——后画的覆盖先画的
r.SetCullFace(true);    // 开启背面剔除（默认）——逆时针面不渲染
r.SetCullFace(false);   // 关闭——双面渲染
r.SetBlend(true);       // 开启混合——透明物体需要
r.SetBlend(false);      // 关闭混合
r.SetWireframe(true);   // 线框模式——调试用
r.SetWireframe(false);  // 回到实体填充

r.Draw(mesh);           // 画一个完整 Mesh
r.Draw(va);             // 直接画 VAO（绕过 Material，适用特殊 shader）
```

### Framebuffer

离屏渲染目标。当前实现为深度专用 FBO（用于阴影贴图），后续可扩展颜色附件。

```cpp
Framebuffer fbo;
bool ok = fbo.Create(2048, 2048);    // 创建 2048×2048 深度纹理 FBO
// 内部：GL_DEPTH_COMPONENT + GL_FLOAT，NEAREST 过滤，CLAMP_TO_BORDER 环绕（边界值=1.0）

fbo.IsValid();          // FBO 是否完整可用
fbo.Bind();             // 绑定——后续渲染写入此 FBO 的深度缓冲
  // ... 用 depth-only shader 渲染场景 ...
fbo.Unbind();           // 回到默认帧缓冲
fbo.GetDepthTexture();  // 获取深度纹理的 GL ID，可在 shader 中用 sampler2D 采样
```

---

## Scene

ECS（实体-组件系统），稀疏集存储——增删 O(1)，遍历组件线性时间。4 种内置组件：Transform、Camera、Light、MeshRenderer。

### Entity 与 Scene

```cpp
auto& scene = GetScene();

Entity e = scene.CreateEntity();   // 分配实体 ID，加入活跃列表。上限 k_MaxEntities=10000
scene.DestroyEntity(e);            // 移除该实体的所有组件，从活跃列表删除
scene.IsAlive(e);                  // 实体是否存活

k_InvalidEntity  // 0xFFFFFFFF  无效实体标识
```

### 组件操作

```cpp
// 添加组件。已存在则返回已有引用
auto& tf  = scene.AddComponent<scene::Transform>(entity);
auto& cam = scene.AddComponent<scene::Camera>(entity);
auto& lt  = scene.AddComponent<scene::Light>(entity);
auto& mr  = scene.AddComponent<scene::MeshRenderer>(entity);

// 获取（返回指针，不存在返回 nullptr）
Transform* t = scene.GetComponent<scene::Transform>(entity);
if (t) { t->position.x += 1.0f; }

// 检查是否存在
if (scene.HasComponent<scene::Camera>(entity)) { ... }

// 移除
scene.RemoveComponent<scene::Light>(entity);
```

**技术细节**：每个组件类型对应一个 `ComponentPool<T>`（稀疏集）。Add 时把实体映射到 dense 数组尾部，Remove 时 swap-and-pop 尾部元素。因此遍历效率高（dense 数组连续），但 Entity ID 不能无限大（稀疏数组预留上限）。

### scene::Transform

有层级关系的世界变换组件。每个 Transform 可指一个 parent 和若干 children。

```cpp
auto& tf = scene.AddComponent<scene::Transform>(entity);
tf.position = {0, 2, 0};
tf.rotation = Quaternion::FromEulerAngles(0, y, 0);  // Y 轴旋转 y 弧度
tf.scale    = {0.8f, 0.8f, 0.8f};

// 建立父子关系
tf.parent = parentEntity;
parentTf.children.push_back(entity);

// 变换矩阵
tf.GetLocalMatrix();               // 仅自身的 Scale→Rotate→Translate
scene.GetWorldMatrix(entity);      // 考虑父子层级累乘的世界矩阵
```

**OnUpdate 行为**：引擎每帧调用 `scene.OnUpdate(dt)` → 内部调用 `RecomputeWorldTransforms()`。算法：先重置所有实体为 local 矩阵，然后从根到叶迭代累乘 parent * local，直到稳定（最多 100 轮迭代）。

### scene::Camera

场景摄像机。渲染时选取一个 camera entity 传给 `scene.OnRender(renderer, cameraEntity, ...)`。

```cpp
auto& cam = scene.AddComponent<scene::Camera>(entity);
cam.fovY      = 60.0f;    // 垂直视场角（度），透视投影参数
cam.nearPlane = 0.1f;     // 近裁剪平面（<此距离的物体不可见）
cam.farPlane  = 1000.0f;  // 远裁剪平面（>此距离的物体不可见）

// 内部使用（OnRender 自动调用）
cam.GetViewMatrix(worldMatrix);          // world 矩阵的逆 → 视图矩阵
cam.GetProjectionMatrix(aspectRatio);   // 透视投影矩阵
```

### scene::Light

内建光源组件。`OnRender` 遍历 Light 组件收集光源并传给 shader。

```cpp
auto& light = scene.AddComponent<scene::Light>(entity);

// 方向光（平行光）——太阳
light.type = LightType::k_Directional;
// Transform.position 存光照方向——指向光源。例 {0.3, 1.0, 0.4}.Normalized() 表示右上方来的光
light.color     = {1.0f, 0.95f, 0.8f};  // 暖白色
light.intensity = 1.0f;

// 点光源——有位置有范围
light.type      = LightType::k_Point;
// Transform.position 存世界空间位置
light.color     = {1.0f, 0.6f, 0.3f};  // 橙色
light.intensity = 2.0f;
light.range     = 8.0f;                // 光照最大半径
```

### scene::MeshRenderer

给实体附加渲染网格。

```cpp
auto& mr = scene.AddComponent<scene::MeshRenderer>(entity);
mr.mesh    = myMesh;   // shared_ptr<Mesh>
mr.enabled = true;     // 设为 false 则跳过渲染
```

### OnRender 渲染流程

`scene.OnRender(renderer, cameraEntity, w, h)` 内部执行：
1. 用 camera entity 的 Transform + Camera 组件计算 view 和 projection 矩阵
2. 遍历所有实体：跳过无 MeshRenderer、无 Transform 或不可用的
3. 对每个可渲染实体计算 MVP，设置 shader uniform（model/view/projection 矩阵）
4. 调用 `mesh->Render()` → `material->Apply()` + `vertexArray->Draw()`

**用户需要在自己的 `onRender` 中调用**：
```cpp
config.onRender = []() {
    GetScene().OnRender(GetRenderer(), cameraEntity,
                        GetWindow().GetWidth(), GetWindow().GetHeight());
};
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

SGKit 会传递链接 `glad` 和平台库 (`gdi32 user32 opengl32 imm32`)。

### 目录结构

```
SGKit/
├── CMakeLists.txt / CMakePresets.json
├── cmake/                     # Platform.cmake + CompilerSettings.cmake
├── external/glad/             # OpenGL 4.6 Core 加载器（静态库）
├── include/sgkit/             # 公共头（聚合头 sgkit.h 一次性包含全部子模块）
│   ├── math/      Vector2/3/4, Matrix4, Quaternion, Transform, MathUtils
│   ├── core/      Window, Input, KeyCodes, FileSystem, ThreadPool
│   ├── graphics/  Shader, VertexBuffer, IndexBuffer, VertexLayout, VertexArray,
│   │              Texture, Material, Mesh, Renderer, Framebuffer
│   ├── scene/     Entity, ComponentPool, Components(Transform/Camera/Light/MeshRenderer), Scene
│   └── framework/ Application(Config), Timing
├── src/                       # 实现（按模块对应）
├── examples/sandbox/          # 示例：地板+立方体+相机控制
├── tests/                     # 单元测试
├── lib/                       # 预编译产物
│   ├── sgkit_d.lib + .pdb     #   Debug
│   └── sgkit.lib              #   Release
└── icon/                      # 程序图标（app.rc + app.ico）
```

### 全局预定义宏（MSVC）

| 宏 | 值 | 说明 |
|----|-----|------|
| `_WINDOWS` | — | GUI 子系统标识 |
| `UNICODE` / `_UNICODE` | — | Windows API + CRT 宽字符 |
| `NOMINMAX` | — | 阻止 `<windows.h>` 定义 `min`/`max` 宏污染 `std::` |
| `WINVER` / `_WIN32_WINNT` | `0x0A00` | 最低调用 Windows 10 API |
| `_DEBUG` | Debug only (MSVC 自动) | 条件启用调试代码 + 控制台 + GL 回调 |
| `SGK_PLATFORM_WINDOWS` | 库 PRIVATE | 库内部平台区分（用户代码不可见） |

### Shader 编写约定

- GLSL 版本：`#version 330 core`
- 顶点属性 location：`0=Position(vec3), 1=Normal(vec3), 2=TexCoord(vec2)`
- 矩阵 uniform 用 `SetMatrix4(name, mat)`，内部调用 `glUniformMatrix4fv(..., 1, GL_FALSE, mat.Data())`
- 纹理采样器通过 `SetInt(name, slot)` 指向 `GL_TEXTURE0 + slot`，配合 `texture.Bind(slot)`
- 默认约定：`u_DiffuseTexture` 绑定到单元 0
