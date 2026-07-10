#include <sgkit/sgkit.h>

using namespace sgkit;

// -- Checkpoint B: a cube spun entirely from C#.
//
// The C++ side only builds the cube, camera and attaches a Script component
// naming the managed type "Spin". Every frame the framework calls
// ScriptEngine::Update, which runs Spin.OnUpdate in C# - that writes the
// entity's rotation back through the interop table. No rotation code here.
//
// Run x64-Debug. Expect a blue cube rotating around Y; Up/Down arrows change
// its speed (input flowing C++ -> C#). Console shows "[Spin] attached ...".

namespace {

std::shared_ptr<graphics::Shader> g_shader;

const char* kVert = R"(#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTex;
uniform mat4 u_Model;
uniform mat4 u_ViewProjection;
out vec3 vNormal;
void main()
{
    gl_Position = u_ViewProjection * u_Model * vec4(aPos, 1.0);
    vNormal = mat3(u_Model) * aNormal;
}
)";

const char* kFrag = R"(#version 330 core
in vec3 vNormal;
out vec4 FragColor;
void main()
{
    vec3 n = normalize(vNormal);
    float d = max(dot(n, normalize(vec3(0.4, 0.8, 0.6))), 0.0);
    vec3 col = vec3(0.2, 0.5, 0.9) * (0.3 + 0.7 * d);
    FragColor = vec4(col, 1.0);
}
)";

std::shared_ptr<scene::Mesh> MakeCube()
{
    constexpr float verts[] = {
        -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,  0.5f,-0.5f, 0.5f, 0,0,1, 1,0,
         0.5f, 0.5f, 0.5f, 0,0,1, 1,1, -0.5f, 0.5f, 0.5f, 0,0,1, 0,1,
         0.5f,-0.5f,-0.5f, 0,0,-1,0,0, -0.5f,-0.5f,-0.5f, 0,0,-1,1,0,
        -0.5f, 0.5f,-0.5f, 0,0,-1,1,1,  0.5f, 0.5f,-0.5f, 0,0,-1,0,1,
        -0.5f, 0.5f, 0.5f, 0,1,0, 0,0,  0.5f, 0.5f, 0.5f, 0,1,0, 1,0,
         0.5f, 0.5f,-0.5f, 0,1,0, 1,1, -0.5f, 0.5f,-0.5f, 0,1,0, 0,1,
        -0.5f,-0.5f,-0.5f, 0,-1,0,0,0,  0.5f,-0.5f,-0.5f, 0,-1,0,1,0,
         0.5f,-0.5f, 0.5f, 0,-1,0,1,1, -0.5f,-0.5f, 0.5f, 0,-1,0,0,1,
         0.5f,-0.5f, 0.5f, 1,0,0, 0,0,  0.5f,-0.5f,-0.5f, 1,0,0, 1,0,
         0.5f, 0.5f,-0.5f, 1,0,0, 1,1,  0.5f, 0.5f, 0.5f, 1,0,0, 0,1,
        -0.5f,-0.5f,-0.5f,-1,0,0, 0,0, -0.5f,-0.5f, 0.5f,-1,0,0, 1,0,
        -0.5f, 0.5f, 0.5f,-1,0,0, 1,1, -0.5f, 0.5f,-0.5f,-1,0,0, 0,1,
    };
    constexpr uint32_t idx[] = {
        0,1,2, 2,3,0,  4,5,6, 6,7,4,  8,9,10, 10,11,8,
        12,13,14, 14,15,12,  16,17,18, 18,19,16,  20,21,22, 22,23,20,
    };

    graphics::VertexLayout layout;
    layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);

    auto vb = std::make_shared<graphics::VertexBuffer>();
    vb->Create(verts, sizeof(verts));
    auto ib = std::make_shared<graphics::IndexBuffer>();
    ib->Create(idx, sizeof(idx) / sizeof(uint32_t));
    auto va = std::make_shared<graphics::VertexArray>();
    va->Create();
    va->SetVertexBuffer(vb, layout);
    va->SetIndexBuffer(ib);

    auto mat = std::make_shared<scene::Material>();
    mat->shader = g_shader;

    auto mesh = std::make_shared<scene::Mesh>();
    mesh->vertexArray = va;
    mesh->material    = mat;
    return mesh;
}

scene::Entity g_camera;

} // namespace

ApplicationConfig sgkit::CreateApplication()
{
    ApplicationConfig cfg{};
    cfg.title           = "SGKit Script Example";
    cfg.width           = 1024;
    cfg.height          = 640;
    cfg.enableScripting = true;

    cfg.onInit = []() -> bool
    {
        g_shader = std::make_shared<graphics::Shader>();
        if (!g_shader->LoadFromSource(kVert, kFrag))
            return false;

        auto& scene = scene::Scene::instance();

        // Camera
        g_camera = scene.CreateEntity();
        auto* ct = scene.AddComponent<scene::component::Transform>(g_camera);
        ct->position = { 0.0f, 0.8f, 4.0f };
        scene.AddComponent<scene::component::Camera>(g_camera);

        // Cube driven by the C# "Spin" script
        scene::Entity cube = scene.CreateEntity();
        scene.AddComponent<scene::component::Transform>(cube);
        scene.AddComponent<scene::component::MeshRenderer>(cube)->mesh = MakeCube();
        scene.AddComponent<scene::component::Script>(cube)->typeName = "Spin";

        // Make Spin's type resolvable, then let ScriptEngine drive it.
        auto& engine = scripting::ScriptEngine::instance();
        if (engine.IsReady())
            engine.LoadScriptAssembly("GameScripts.dll");

        return true;
    };

    cfg.onUpdate = []()
    {
        if (core::Input::instance().IsKeyPressed(core::KeyCode::Escape))
            core::Window::instance().RequestClose();
    };

    cfg.onRender = []()
    {
        scene::Scene::instance().Render(g_camera);
    };

    cfg.onShutdown = []()
    {
        g_shader.reset();
    };

    return cfg;
}
