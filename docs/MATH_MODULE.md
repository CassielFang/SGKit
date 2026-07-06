# SGKit Math 模块文档

## 概述

`sgkit::math` 命名空间提供了轻量级的 3D 数学库，包含向量（2/3/4 维）、4×4 列主序矩阵、四元数与标量工具函数。所有类型均为 `float` 精度，`constexpr` 友好，无虚函数、无堆分配，适合实时渲染场景。

包含头文件：`#include <sgkit/sgkit_math.h>`

所有内容位于 `sgkit::math` 命名空间下。

---

## 1. 标量常量与工具函数 (`MathUtils.h`)

### 常量

| 常量 | 值 | 说明 |
|---|---|---|
| `k_Pi` | 3.1415927f | π |
| `k_TwoPi` | 6.2831855f | 2π |
| `k_HalfPi` | 1.5707963f | π/2 |
| `k_Epsilon` | 1e-5f | 浮点比较容差 |
| `k_Deg2Rad` | π/180 | 度转弧度乘数 |
| `k_Rad2Deg` | 180/π | 弧度转度乘数 |

### 函数

#### `float ToRadians(float degrees)`
将角度从度转换为弧度。`return degrees * k_Deg2Rad`

#### `float ToDegrees(float radians)`
将角度从弧度转换为度。`return radians * k_Rad2Deg`

#### `bool Approximately(float a, float b, float epsilon = k_Epsilon)`
浮点数近似相等判断。当 `|a - b| < epsilon` 时返回 `true`。所有 `Vector*` 和 `Matrix4` 的 `operator==` 都依赖此函数。

#### `T Clamp(T value, T minVal, T maxVal)`
将值钳制在 `[minVal, maxVal]` 范围内。模板函数，适用于任意可比较类型。

```cpp
float f = Clamp(1.5f, 0.0f, 1.0f);  // f = 1.0f
int  i = Clamp(5, 0, 10);            // i = 5
```

#### `T Lerp(T a, T b, float t)`
线性插值（Linear Interpolation）。`t=0` 返回 `a`，`t=1` 返回 `b`。模板函数，适用于任意支持 `+`、`-`、`* float` 的类型。

```cpp
float mid = Lerp(0.0f, 10.0f, 0.5f);  // 5.0f
```

---

## 2. Vector2 - 二维向量

`#include <sgkit/math/Vector2.h>`

### 成员变量

| 成员 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `x` | `float` | `0.0f` | X 分量 |
| `y` | `float` | `0.0f` | Y 分量 |

### 构造函数

#### `Vector2()` - 默认构造
零向量 `(0, 0)`。

#### `Vector2(float x, float y)` - 分量构造
直接指定 x、y 分量。

### 元素访问

#### `float& operator[](int i)` / `const float& operator[](int i) const`
按索引访问：`[0]` = x，`[1]` = y。不做边界检查，索引越界是未定义行为。

### 算术运算符

| 运算符 | 说明 |
|---|---|
| `v1 + v2` | 逐分量相加 |
| `v1 - v2` | 逐分量相减 |
| `v * s` / `s * v` | 标量乘法（逐分量乘） |
| `v / s` | 标量除法（逐分量除） |
| `-v` | 取反 |
| `v1 += v2` 等 | 复合赋值 |

### 比较运算符

#### `bool operator==(const Vector2& rhs) const`
使用 `Approximately()` 逐分量比较，含浮点容差。

#### `bool operator!=(const Vector2& rhs) const`
不等比较。

### 向量操作

#### `float LengthSquared() const`
返回 `x² + y²`。比 `Length()` 快（省去 `sqrt`），适合做距离比较。

```cpp
// 比较两个向量到某点的距离，无需开根
if ((pos - target).LengthSquared() < 25.0f)  // 等价于长度 < 5
```

#### `float Length() const`
返回向量模长 `√(x² + y²)`。

#### `Vector2& Normalize()`
将向量**就地**归一化为单位向量（长度为1）。若长度过小（< `k_Epsilon`）则保持不变，防止除零。返回 `*this` 支持链式调用。

#### `Vector2 Normalized() const`
返回归一化后的**新向量**，不修改原向量。

### 静态方法

#### `static float Dot(const Vector2& a, const Vector2& b)`
点积：`a.x*b.x + a.y*b.y`。

#### `static Vector2 Lerp(const Vector2& a, const Vector2& b, float t)`
对两向量的每个分量做线性插值。

```cpp
Vector2 mid = Vector2::Lerp({0,0}, {10, 20}, 0.5f);  // {5, 10}
```

### 静态常量

| 常量 | 值 |
|---|---|
| `Vector2::k_Zero` | `(0, 0)` |
| `Vector2::k_One` | `(1, 1)` |
| `Vector2::k_Up` | `(0, 1)` |
| `Vector2::k_Right` | `(1, 0)` |

---

## 3. Vector3 - 三维向量

`#include <sgkit/math/Vector3.h>`

### 成员变量

| 成员 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `x` | `float` | `0.0f` | X 分量 |
| `y` | `float` | `0.0f` | Y 分量 |
| `z` | `float` | `0.0f` | Z 分量 |

### 构造函数

#### `Vector3()` - 默认构造
零向量 `(0, 0, 0)`。

#### `Vector3(float x, float y, float z)` - 分量构造

#### `explicit Vector3(const Vector2& v2, float z = 0.0f)` - 从 Vector2 构造
从 2D 向量提升为 3D，z 分量可选指定（默认 0）。

### 分量混合（Swizzle）

#### `Vector2 XY() const`
提取 `(x, y)` 为 Vector2。

#### `Vector2 XZ() const`
提取 `(x, z)` 为 Vector2。

#### `Vector2 YZ() const`
提取 `(y, z)` 为 Vector2。

```cpp
Vector3 pos(1, 2, 3);
Vector2 flat = pos.XZ();  // {1, 3} - 常用于地形/平面坐标
```

### 元素访问

#### `float& operator[](int i)` / `const float& operator[](int i) const`
按索引访问：`[0]`=x, `[1]`=y, `[2]`=z。

### 算术运算符

与 Vector2 模式相同：`+`、`-`、`*`、`/`、`+=`、`-=`、`*=`、`/=`、一元 `-`。

### 比较运算符

#### `bool operator==(const Vector3& rhs) const`
含容差的逐分量近似比较。

#### `bool operator!=(const Vector3& rhs) const`

### 向量操作

#### `float LengthSquared() const` / `float Length() const`
返回 `x²+y²+z²` 或 `√(x²+y²+z²)`。

#### `Vector3& Normalize()` / `Vector3 Normalized() const`
就地归一化 / 返回归一化后的新向量。

### 静态方法

#### `static float Dot(const Vector3& a, const Vector3& b)`
点积：`a.x*b.x + a.y*b.y + a.z*b.z`。常用于：
- 计算两向量夹角余弦
- 判断方向相似度
- 投影计算

#### `static Vector3 Cross(const Vector3& a, const Vector3& b)`
叉积，返回垂直于 a 和 b 所在平面的向量（遵循右手定则）。模长 = |a|·|b|·sin(θ)。常用于：
- 计算面法线
- 构建局部坐标系（LookAt 中用到）
- 计算扭矩方向

```cpp
Vector3 right   = {1, 0, 0};
Vector3 forward = {0, 0, -1};  // OpenGL 前方向为 -Z
Vector3 up      = Vector3::Cross(forward, right);  // {0, 1, 0}
```

#### `static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)`
逐分量线性插值。

### 静态常量

| 常量 | 值 | 说明 |
|---|---|---|
| `Vector3::k_Zero` | `(0, 0, 0)` | 零向量 |
| `Vector3::k_One` | `(1, 1, 1)` | 全 1 |
| `Vector3::k_Up` | `(0, 1, 0)` | 世界 Y 轴正方向 |
| `Vector3::k_Down` | `(0, -1, 0)` | 世界 Y 轴负方向 |
| `Vector3::k_Right` | `(1, 0, 0)` | 世界 X 轴正方向 |
| `Vector3::k_Left` | `(-1, 0, 0)` | 世界 X 轴负方向 |
| `Vector3::k_Forward` | `(0, 0, -1)` | **OpenGL 前方向（-Z）** |
| `Vector3::k_Back` | `(0, 0, 1)` | **OpenGL 后方向（+Z）** |

> **注意**：Forward 和 Back 的方向符合 OpenGL 相机约定，向前看是 -Z 方向。

---

## 4. Vector4 - 四维向量

`#include <sgkit/math/Vector4.h>`

### 成员变量

| 成员 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `x` | `float` | `0.0f` | X 分量 |
| `y` | `float` | `0.0f` | Y 分量 |
| `z` | `float` | `0.0f` | Z 分量 |
| `w` | `float` | `0.0f` | W 分量 |

### 构造函数

#### `Vector4()` - 默认构造
零向量 `(0, 0, 0, 0)`。

#### `Vector4(float x, float y, float z, float w)` - 分量构造

#### `explicit Vector4(const Vector3& v3, float w = 1.0f)` - 从 Vector3 构造
将 3D 向量扩展为齐次坐标。w=1.0 表示**点**（受平移影响），w=0.0 表示**方向**（不受平移影响）。

#### `explicit Vector4(const Vector2& v2, float z = 0.0f, float w = 1.0f)` - 从 Vector2 构造

### 分量混合（Swizzle）

#### `Vector3 XYZ() const`
提取 `(x, y, z)` 为 Vector3。

#### `Vector2 XY() const`
提取 `(x, y)` 为 Vector2。

### 元素访问 / 算术 / 比较

与 Vector2/Vector3 模式完全一致。`operator[]` 访问 `[0]`=x, `[1]`=y, `[2]`=z, `[3]`=w。

### 向量操作

#### `float LengthSquared() const` / `float Length() const`
返回四维向量的平方长度 / 长度（包含 w 分量）。

#### `Vector4& Normalize()` / `Vector4 Normalized() const`
对四维向量做归一化（包含 w 分量一起参与计算）。

### 静态方法

#### `static float Dot(const Vector4& a, const Vector4& b)`
四维点积：`a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w`。

#### `static Vector4 Lerp(const Vector4& a, const Vector4& b, float t)`
逐分量线性插值。

### 静态常量

| 常量 | 值 |
|---|---|
| `Vector4::k_Zero` | `(0, 0, 0, 0)` |
| `Vector4::k_One` | `(1, 1, 1, 1)` |

### 典型用途

Vector4 在本引擎中主要用于：
- **齐次坐标**：与 `Matrix4` 相乘时，点用 `w=1`，方向用 `w=0`
- **颜色/纹理坐标**：RGBA 四通道值
- **参见 `Matrix4::operator*(const Vector4&)`、`TransformPoint()`、`TransformDirection()`**

---

## 5. Matrix4 - 4×4 列主序矩阵

`#include <sgkit/math/Matrix4.h>`

### 存储约定

Matrix4 采用**列主序（Column-Major）**存储，与 OpenGL 的 `glUniformMatrix4fv` 直接兼容。

内部数据为 `float m[4][4]`，索引方式为 `m[col][row]`（第 col 列，第 row 行）：

```
逻辑布局（数学记法）：
  m[0]  = 列0  = [m00, m10, m20, m30]ᵀ
  m[1]  = 列1  = [m01, m11, m21, m31]ᵀ
  m[2]  = 列2  = [m02, m12, m22, m32]ᵀ
  m[3]  = 列3  = [m03, m13, m23, m33]ᵀ
```

`Data()` 返回的 `float*` 可直接传给 OpenGL：
```cpp
Matrix4 mvp = projection * view * model;
glUniformMatrix4fv(loc, 1, GL_FALSE, mvp.Data());
```

### 构造函数

#### `Matrix4()`
默认构造为单位矩阵（Identity）。

### 原始数据访问

#### `const float* Data() const` / `float* Data()`
返回指向 16 个 float 的指针，列主序排列，可直接传入 OpenGL。

### 元素访问

#### `float& operator()(int col, int row)` / `float operator()(int col, int row) const`
按（列, 行）访问单个元素。注意：第一个参数是**列索引**，第二个是**行索引**。

```cpp
Matrix4 m;
m(3, 0) = 5.0f;   // 第3列第0行 = 平移 X
m(3, 1) = 10.0f;  // 第3列第1行 = 平移 Y
```

### 比较

#### `bool operator==(const Matrix4& rhs) const`
逐元素 `Approximately()` 比较。

#### `bool operator!=(const Matrix4& rhs) const`

### 矩阵乘法

#### `Matrix4 operator*(const Matrix4& rhs) const`
矩阵乘法，结果 = `this × rhs`。注意**矩阵乘法不满足交换律**，顺序很重要。

```cpp
Matrix4 mvp = projection * view * model;  // 先 model，再 view，最后 projection
```

#### `Vector4 operator*(const Vector4& v) const`
矩阵乘以 4D 向量，返回 `M × v`。

### 向量变换

#### `Vector3 TransformPoint(const Vector3& v) const`
将 3D 点通过矩阵变换（内部使用 w=1.0）。会应用平移、旋转、缩放、投影等全部变换。若结果 w ≠ 0 会做透视除法（将 x,y,z 除以 w）。

```cpp
// 将局部坐标变换到世界空间
Vector3 worldPos = modelMatrix.TransformPoint(localPos);
```

#### `Vector3 TransformDirection(const Vector3& v) const`
仅变换方向（内部使用 w=0.0）。**平移分量被忽略**，只应用旋转和缩放。

```cpp
// 将法线方向变换到世界空间（仅旋转/缩放，不平移）
Vector3 worldNormal = modelMatrix.TransformDirection(localNormal);
```

### 初始化为特定变换矩阵（Set 系列）

以下方法将 `*this` 设置为指定变换矩阵并返回 `*this`（支持链式调用）。

#### `Matrix4& SetIdentity()`
设为单位矩阵（对角线上全是 1）。

#### `Matrix4& SetZero()`
设为零矩阵（全部元素为 0）。

#### `Matrix4& SetTranslate(float x, float y, float z)` / `SetTranslate(const Vector3& v)`
设为平移矩阵。

```
     | 1  0  0  x |
     | 0  1  0  y |
     | 0  0  1  z |
     | 0  0  0  1 |
```

#### `Matrix4& SetRotateX(float radians)` / `SetRotateY(float radians)` / `SetRotateZ(float radians)`
设为绕 X / Y / Z 轴旋转指定弧度（右手定则，拇指指向 +轴 时手指弯曲方向为正旋转方向）。

```cpp
m.SetRotateY(ToRadians(90.0f));  // 绕 Y 轴旋转 90°
```

#### `Matrix4& SetScale(float x, float y, float z)` / `SetScale(const Vector3& v)`
设为缩放矩阵。

```
     | x  0  0  0 |
     | 0  y  0  0 |
     | 0  0  z  0 |
     | 0  0  0  1 |
```

#### `Matrix4& SetPerspective(float fovYRadians, float aspect, float nearZ, float farZ)`
设为透视投影矩阵（OpenGL 右手坐标系）。

| 参数 | 说明 |
|---|---|
| `fovYRadians` | 垂直视场角（**弧度**），e.g. `ToRadians(60.0f)` |
| `aspect` | 宽高比（width/height），e.g. `1920.0f/1080.0f` |
| `nearZ` | 近裁剪面距离（正值） |
| `farZ` | 远裁剪面距离（正值） |

#### `Matrix4& SetOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ)`
设为正交投影矩阵。常用于 2D 渲染、UI、编辑器顶/侧视图。

#### `Matrix4& SetLookAt(const Vector3& eye, const Vector3& target, const Vector3& up)`
设为观察矩阵（View Matrix）。构建右手坐标系：X=右(s), Y=上(u), Z=-前(f)。

| 参数 | 说明 |
|---|---|
| `eye` | 相机位置 |
| `target` | 注视目标点 |
| `up` | 世界向上方向，通常用 `Vector3::k_Up` |

### 矩阵运算

#### `Matrix4& Transpose()`
**就地**转置矩阵（行变列、列变行）。返回 `*this`。

#### `Matrix4 Transposed() const`
返回转置后的**新矩阵**，不修改原矩阵。

#### `Matrix4& Invert()`
**就地**求逆矩阵。返回 `*this`。若矩阵退化（行列式接近 0），退化为单位矩阵。

#### `Matrix4 Inverted() const`
返回求逆后的**新矩阵**，不修改原矩阵。若矩阵退化则返回单位矩阵。

> **重要警告**：`Invert()` 和 `Inverted()` 使用通用伴随矩阵法（非针对单一变换优化）。对于纯平移/旋转/缩放矩阵，逆矩阵可能不如预期。若需频繁求逆，建议缓存或使用更专用的求逆方式。

#### `float Determinant() const`
返回矩阵的行列式值。用于判断矩阵是否可逆（接近 0 表示退化）。

### 静态工厂方法

每个静态工厂方法返回一个新 Matrix4 对象，等价于构造 + Set：

| 静态方法 | 等价于 |
|---|---|
| `Matrix4::Identity()` | 构造默认 + `SetIdentity()` |
| `Matrix4::Zero()` | 构造 + `SetZero()` |
| `Matrix4::Translate(v)` | 构造 + `SetTranslate(v)` |
| `Matrix4::RotateX/Y/Z(r)` | 构造 + `SetRotate*(r)` |
| `Matrix4::Scale(v)` | 构造 + `SetScale(v)` |
| `Matrix4::Perspective(...)` | 构造 + `SetPerspective(...)` |
| `Matrix4::Orthographic(...)` | 构造 + `SetOrthographic(...)` |
| `Matrix4::LookAt(...)` | 构造 + `SetLookAt(...)` |

```cpp
// 两种写法等价：
Matrix4 m1 = Matrix4::Translate({5, 0, 0});

Matrix4 m2;
m2.SetTranslate({5, 0, 0});
```

---

## 6. Quaternion - 四元数

`#include <sgkit/math/Quaternion.h>`

四元数是 3D 旋转的高效、无万向节锁表示。`w=1, x=y=z=0` 表示恒等旋转（无旋转）。

### 成员变量

| 成员 | 类型 | 默认值 |
|---|---|---|
| `x` | `float` | `0.0f` |
| `y` | `float` | `0.0f` |
| `z` | `float` | `0.0f` |
| `w` | `float` | `1.0f` |

### 构造函数

#### `Quaternion()` - 默认构造
恒等四元数 `(0, 0, 0, 1)`（表示无旋转）。

#### `Quaternion(float x, float y, float z, float w)` - 分量构造

### 比较

#### `bool operator==(const Quaternion& rhs) const`
逐分量 `Approximately()` 比较。

#### `bool operator!=(const Quaternion& rhs) const`

### 乘法

#### `Quaternion operator*(const Quaternion& rhs) const`
四元数乘法（旋转组合）。`q1 * q2` 表示**先应用 q2 再应用 q1**。

```cpp
// 先绕 Y 轴旋转，再绕 X 轴旋转
Quaternion q = Quaternion::FromEulerAngles(pitch, 0, 0) *
               Quaternion::FromEulerAngles(0, yaw, 0);
```

#### `Vector3 operator*(const Vector3& v) const`
用四元数旋转一个 3D 向量。等价于 `q * v * q⁻¹`。

```cpp
Quaternion rot = Quaternion::FromAxisAngle(Vector3::k_Up, ToRadians(45.0f));
Vector3 dir = rot * Vector3::k_Forward;  // 将前向量旋转 45°
```

### 基本运算

#### `Quaternion& Normalize()` / `Quaternion Normalized() const`
归一化四元数（使其模长为 1）。四元数在多次乘法后可能漂移，需定期归一化以确保表示的是纯旋转。

#### `Quaternion& Conjugate()` / `Quaternion Conjugated() const`
共轭（取反虚部）。对于单位四元数，共轭等于逆。

| 正常 | 共轭后 |
|---|---|
| `(x, y, z, w)` | `(-x, -y, -z, w)` |

#### `Quaternion& Invert()` / `Quaternion Inverted() const`
求逆：先共轭，再除以 `LengthSquared()`。`q * q⁻¹ = Identity`。

#### `float LengthSquared() const` / `float Length() const`
四元数的四维模长的平方 / 模长。

#### `float Dot(const Quaternion& rhs) const`
四元数点积，用于判断两个旋转的相似度。值越接近 1 表示旋转越接近。

### 转换

#### `Vector3 ToEulerAngles() const`
将四元数转换为欧拉角 `(pitch, yaw, roll)`，单位**弧度**。返回值中 x=pitch（绕 X），y=yaw（绕 Y），z=roll（绕 Z）。

### 静态工厂

#### `static Quaternion Identity()`
返回恒等四元数 `(0, 0, 0, 1)`。

#### `static Quaternion FromEulerAngles(float pitch, float yaw, float roll)`
从欧拉角创建四元数。参数单位为**弧度**，旋转顺序为 pitch->yaw->roll。

```cpp
Quaternion q = Quaternion::FromEulerAngles(
    ToRadians(30.0f),  // pitch (X)
    ToRadians(45.0f),  // yaw   (Y)
    ToRadians(0.0f)    // roll  (Z)
);
```

#### `static Quaternion FromAxisAngle(const Vector3& axis, float radians)`
从旋转轴和角度创建四元数。轴会自动归一化。

```cpp
// 绕世界 UP 轴旋转 90°
Quaternion q = Quaternion::FromAxisAngle(Vector3::k_Up, ToRadians(90.0f));
```

#### `static Quaternion Slerp(const Quaternion& a, const Quaternion& b, float t)`
**球面线性插值（Spherical Linear Interpolation）**。沿四维单位球面上的最短弧在两旋转之间做匀速插值。t=0 返回 a，t=1 返回 b。

内部逻辑：
- 若 a·b < 0，取反 b 确保走最短弧
- 若两四元数非常接近（cosθ > 1-ε），退化为线性插值（Lerp）
- 否则做真正的球面插值

```cpp
// 平滑旋转动画
Quaternion mid = Quaternion::Slerp(start, end, elapsedTime / duration);
```

#### `static Quaternion LookAt(const Vector3& direction, const Vector3& up)`
构造一个"朝向某方向"的旋转四元数。与 `Matrix4::LookAt` 旋转部分等效。

```cpp
// 让物体朝向目标
Vector3 dir = (target - position).Normalized();
Quaternion rot = Quaternion::LookAt(dir, Vector3::k_Up);
```

---

## 7. 典型使用模式

### 构建 MVP 矩阵

```cpp
// 透视投影
float fov  = ToRadians(60.0f);
float aspect = (float)width / (float)height;
Matrix4 projection = Matrix4::Perspective(fov, aspect, 0.1f, 100.0f);

// 相机观察
Matrix4 view = Matrix4::LookAt(
    {0, 5, 10},      // 相机位置
    {0, 0, 0},       // 注视目标
    Vector3::k_Up    // 上方向
);

// 模型变换
Matrix4 model = Matrix4::Translate({2, 0, 0}) *
                Matrix4::RotateY(ToRadians(45.0f)) *
                Matrix4::Scale({1, 1, 1});

// 组合（注意顺序：projection × view × model）
Matrix4 mvp = projection * view * model;
```

### 使用四元数做平滑旋转

```cpp
class Rotator {
    Quaternion m_current;
    Quaternion m_target;
    float m_t = 1.0f;  // 已完成

public:
    void Update(float dt) {
        if (m_t < 1.0f) {
            m_t += dt * 2.0f;  // 速度
            m_current = Quaternion::Slerp(m_current, m_target, Clamp(m_t, 0.0f, 1.0f));
        }
    }

    void RotateTo(const Quaternion& target) {
        m_target = target;
        m_t = 0.0f;
    }

    Quaternion GetRotation() const { return m_current; }
};
```

### 判断两个方向是否接近

```cpp
Vector3 forward = GetForward();
Vector3 toTarget = (target - position).Normalized();
float alignment = Vector3::Dot(forward, toTarget);

if (alignment > 0.98f) {
    // 正面面对目标
} else if (alignment < -0.98f) {
    // 完全背对目标
}
```

### 构建局部坐标系（右手）

```cpp
Vector3 forward = (target - eye).Normalized();
Vector3 right   = Vector3::Cross(Vector3::k_Up, forward).Normalized();
Vector3 up      = Vector3::Cross(forward, right);  // 不依赖世界 up

// right/up/forward 形成正交右手基
```

---

## 8. 设计原则与注意事项

1. **列主序**：`Matrix4` 采用列主序存储，`Data()` 可直接传给 `glUniformMatrix4fv`。构造平移/旋转/缩放矩阵时平移分量在 `m[3][0]`、`m[3][1]`、`m[3][2]`（第 3 列的第 0-2 行）。

2. **右手坐标系**：默认采用右手坐标系（OpenGL 风格）。`k_Forward = (0, 0, -1)` 表示相机看向 -Z 方向。

3. **弧度制**：所有角度相关函数使用**弧度**。使用 `ToRadians()` / `ToDegrees()` 转换。

4. **浮点容差**：`operator==` 使用 `Approximately()` 含容差比较（默认 1e-5），如需精确比较请直接访问成员。

5. **Matrix4::Invert() 的局限性**：使用通用伴随矩阵法，对退化矩阵回退到单位矩阵。若矩阵接近奇异（如缩放为 0），求逆可能不准确。`Invert()` 不针对纯变换矩阵做优化，频繁调用的场景建议自行缓存逆矩阵。

6. **四元数默认构造为 Identity**：`Quaternion()` 默认 `w=1`，表示无旋转，与 `Quaternion::Identity()` 等价。

7. **零向量归一化安全**：所有 `Normalize()` 方法在长度 < `k_Epsilon` 时会跳过除法，防止 NaN 传播。
