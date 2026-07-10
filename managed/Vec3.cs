using System.Runtime.InteropServices;

namespace SGKit
{
    // Blittable 3-vector. Layout MUST match native sgkit::scripting::Vec3
    // (3 sequential floats).
    [StructLayout(LayoutKind.Sequential)]
    public struct Vec3
    {
        public float X;
        public float Y;
        public float Z;

        public Vec3(float x, float y, float z) { X = x; Y = y; Z = z; }

        public override string ToString() => "(" + X + ", " + Y + ", " + Z + ")";
    }
}
