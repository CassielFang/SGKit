namespace SGKit
{
    // Base class for all C# gameplay scripts. Subclass it, override the
    // lifecycle methods, and attach by type name via a native Script component:
    //
    //     scene.AddComponent<scene::component::Script>(entity)->typeName = "MyScript";
    //
    // The engine instantiates the type on first sight, sets Entity, and drives
    // OnCreate once then OnUpdate every frame.
    public abstract class Script
    {
        // Native entity id this script instance is attached to.
        public uint Entity { get; internal set; }

        public virtual void OnCreate() { }
        public virtual void OnUpdate(float dt) { }
        public virtual void OnDestroy() { }

        // -- Convenience access to the attached entity's Transform ------------
        protected Vec3 Position
        {
            get => Native.Transform_GetPosition(Entity);
            set => Native.Transform_SetPosition(Entity, value);
        }

        // Euler angles in radians (pitch, yaw, roll).
        protected Vec3 EulerAngles
        {
            get => Native.Transform_GetEuler(Entity);
            set => Native.Transform_SetEuler(Entity, value);
        }

        protected Vec3 Scale
        {
            get => Native.Transform_GetScale(Entity);
            set => Native.Transform_SetScale(Entity, value);
        }

        protected void Log(string message) =>
            Native.Log(0, "[" + GetType().Name + "] " + message);
    }

    // Frame timing.
    public static class Time
    {
        public static float DeltaTime => Native.Clock_DeltaTime();
    }

    // Polled keyboard input, mirroring sgkit::core::Input.
    public static class Input
    {
        public static bool IsKeyDown(Key key) => Native.Input_IsKeyDown((int)key) != 0;
        public static bool IsKeyPressed(Key key) => Native.Input_IsKeyPressed((int)key) != 0;
    }
}
