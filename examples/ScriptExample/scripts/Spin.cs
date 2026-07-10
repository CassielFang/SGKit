using SGKit;

// Demo gameplay script. Attached from C++ with:
//     scene.AddComponent<scene::component::Script>(cube)->typeName = "Spin";
//
// Spins its entity around the Y axis, and lets you speed it up / slow it down
// with the arrow keys to show input flowing from the engine into C#.
public class Spin : Script
{
    private float _angle;
    private float _speed = 1.0f; // radians per second

    public override void OnCreate()
    {
        Log("attached to entity " + Entity);
    }

    public override void OnUpdate(float dt)
    {
        if (Input.IsKeyDown(Key.Up))   _speed += dt * 2.0f;
        if (Input.IsKeyDown(Key.Down)) _speed -= dt * 2.0f;
        if (_speed < 0.0f) _speed = 0.0f;

        _angle += dt * _speed;
        EulerAngles = new Vec3(0.0f, _angle, 0.0f);
    }

    public override void OnDestroy()
    {
        Log("destroyed");
    }
}
