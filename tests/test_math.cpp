#include <sgkit/sgkit_math.h>

#include <cstdio>
#include <vector>
#include <string>

using namespace sgkit::math;

struct TestRunner
{
    int passed = 0;
    int failed = 0;
    std::vector<std::string> failures;

    void Check(bool cond, const char* msg)
    {
        if (cond)
            passed++;
        else
        {
            failed++;
            failures.push_back(msg);
        }
    }

    int Finish()
    {
        for (auto& f : failures)
            std::printf("FAIL: %s\n", f.c_str());
        std::printf("\n%d passed, %d failed\n", passed, failed);
        return failed > 0 ? 1 : 0;
    }
};

int main()
{
    TestRunner r;

    // Vector2
    {
        Vector2 a{2.0f, 3.0f};
        Vector2 b{1.0f, 4.0f};
        r.Check((a + b) == Vector2{3.0f, 7.0f},    "Vector2 add");
        r.Check((a - b) == Vector2{1.0f, -1.0f},   "Vector2 sub");
        r.Check((a * 2.0f) == Vector2{4.0f, 6.0f}, "Vector2 mul");
        r.Check((a / 2.0f) == Vector2{1.0f, 1.5f}, "Vector2 div");
        r.Check((-a) == Vector2{-2.0f, -3.0f},      "Vector2 neg");
    }

    {
        Vector2 v{3.0f, 4.0f};
        r.Check(Approximately(v.Length(), 5.0f),       "Vector2 length");
        r.Check(Approximately(v.LengthSquared(), 25.0f),"Vector2 lengthSq");
        r.Check(Approximately(Vector2::Dot({2.0f, 3.0f}, {4.0f, -1.0f}), 5.0f), "Vector2 dot");
    }

    {
        Vector2 v{3.0f, 4.0f};
        v.Normalize();
        r.Check(Approximately(v.Length(), 1.0f), "Vector2 normalized length");
        r.Check(Approximately(v.x, 0.6f) && Approximately(v.y, 0.8f), "Vector2 normalized values");
    }

    // Vector3
    {
        Vector3 a{2.0f, 3.0f, 1.0f};
        Vector3 b{1.0f, 4.0f, 2.0f};
        r.Check((a + b) == Vector3{3.0f, 7.0f, 3.0f},    "Vector3 add");
        r.Check((a - b) == Vector3{1.0f, -1.0f, -1.0f},  "Vector3 sub");
        r.Check((a * 3.0f) == Vector3{6.0f, 9.0f, 3.0f}, "Vector3 mul");
    }

    {
        Vector3 a{1.0f, 0.0f, 0.0f};
        Vector3 b{0.0f, 1.0f, 0.0f};
        Vector3 c = Vector3::Cross(a, b);
        r.Check(c == Vector3{0.0f, 0.0f, 1.0f}, "Vector3 cross");
    }

    {
        Vector3 v{1.0f, 2.0f, 2.0f};
        v.Normalize();
        r.Check(Approximately(v.Length(), 1.0f), "Vector3 normalize");
    }

    // Vector4
    {
        Vector4 a{1.0f, 2.0f, 3.0f, 4.0f};
        r.Check(Approximately(Vector4::Dot(a, a), 30.0f), "Vector4 dot");
    }

    // Matrix4
    {
        Matrix4 m;
        m.SetIdentity();
        r.Check(Approximately(m(0, 0), 1.0f), "Matrix4 identity[0][0]");
        r.Check(Approximately(m(1, 1), 1.0f), "Matrix4 identity[1][1]");
        r.Check(Approximately(m(2, 2), 1.0f), "Matrix4 identity[2][2]");
        r.Check(Approximately(m(3, 3), 1.0f), "Matrix4 identity[3][3]");
    }

    {
        Matrix4 a = Matrix4::Translate({2.0f, 0.0f, 0.0f});
        Matrix4 b = Matrix4::Scale({3.0f, 1.0f, 1.0f});
        Vector3 p = (a * b).TransformPoint({1.0f, 0.0f, 0.0f});
        r.Check(p == Vector3{5.0f, 0.0f, 0.0f}, "Matrix4 multiply TRS");

        Matrix4 t2 = Matrix4::Translate({10.0f, 0.0f, 0.0f});
        r.Check(t2.TransformPoint({1.0f, 2.0f, 3.0f}) == Vector3{11.0f, 2.0f, 3.0f}, "Matrix4 transform point");

        r.Check(t2.TransformDirection({1.0f, 2.0f, 3.0f}) == Vector3{1.0f, 2.0f, 3.0f}, "Matrix4 transform dir");
    }

    {
        Matrix4 t = Matrix4::Translate({5.0f, -3.0f, 2.0f});
        Matrix4 inv = t.Inverted();
        Vector3 p{1.0f, 2.0f, 3.0f};
        r.Check(inv.TransformPoint(t.TransformPoint(p)) == p, "Matrix4 inverse");
    }

    {
        Matrix4 m;
        m.SetIdentity();
        m(0, 1) = 5.0f;
        m.Transpose();
        r.Check(Approximately(m(1, 0), 5.0f), "Matrix4 transpose");
        r.Check(Approximately(m(1, 1), 1.0f), "Matrix4 transpose keep diag");
    }

    // Quaternion
    {
        Quaternion q;
        r.Check(Approximately(q.w, 1.0f) && Approximately(q.x, 0.0f) && Approximately(q.y, 0.0f) && Approximately(q.z, 0.0f), "Quaternion identity");
    }

    {
        Quaternion q = Quaternion::FromAxisAngle({0.0f, 0.0f, 1.0f}, k_HalfPi);
        Vector3 v = q * Vector3{1.0f, 0.0f, 0.0f};
        r.Check(v == Vector3{0.0f, 1.0f, 0.0f}, "Quaternion rotate 90 Z");
    }

    {
        Quaternion a;
        Quaternion b = Quaternion::FromAxisAngle({0.0f, 1.0f, 0.0f}, k_Pi);
        Quaternion mid = Quaternion::Slerp(a, b, 0.5f);
        Vector3 v = mid * Vector3{0.0f, 0.0f, -1.0f};
        r.Check(Approximately(v.z, 0.0f, 0.01f), "Quaternion slerp");
    }

    // Transform
    {
        Transform tf;
        tf.position = {3.0f, 0.0f, 0.0f};
        Matrix4 m = tf.GetLocalMatrix();
        r.Check(m.TransformPoint({1.0f, 2.0f, 3.0f}) == Vector3{4.0f, 2.0f, 3.0f}, "Transform local matrix");
    }

    return r.Finish();
}
