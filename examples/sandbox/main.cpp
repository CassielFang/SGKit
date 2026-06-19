#include <sgkit/sgkit.h>

#include <cstdio>
#include <memory>
#include <vector>

using namespace sgkit;
using namespace sgkit::graphics;

// File-scope state
static scene::Entity s_floorEntity   = scene::k_InvalidEntity;
static scene::Entity s_lightEntity   = scene::k_InvalidEntity;
static scene::Entity s_cameraEntity  = scene::k_InvalidEntity;
static std::shared_ptr<graphics::Shader> s_defaultShader;
static constexpr int k_NumCubes = 4;
static scene::Entity s_cubeEntities[k_NumCubes] = {};

static std::shared_ptr<graphics::Mesh> CreateColoredCubeMesh(
    float r, float g, float b)
{
    float vertices[] = {
        // pos (3)             normal (3)         texCoord (2)
        // Front
        -0.5f, -0.5f,  0.5f,   0,0,1,   0,0,   0.5f, -0.5f,  0.5f,   0,0,1,   1,0,
         0.5f,  0.5f,  0.5f,   0,0,1,   1,1,  -0.5f,  0.5f,  0.5f,   0,0,1,   0,1,
        // Back
         0.5f, -0.5f, -0.5f,   0,0,-1,  0,0,  -0.5f, -0.5f, -0.5f,   0,0,-1,  1,0,
        -0.5f,  0.5f, -0.5f,   0,0,-1,  1,1,   0.5f,  0.5f, -0.5f,   0,0,-1,  0,1,
        // Top
        -0.5f,  0.5f,  0.5f,   0,1,0,   0,0,   0.5f,  0.5f,  0.5f,   0,1,0,   1,0,
         0.5f,  0.5f, -0.5f,   0,1,0,   1,1,  -0.5f,  0.5f, -0.5f,   0,1,0,   0,1,
        // Bottom
        -0.5f, -0.5f, -0.5f,   0,-1,0,  0,0,   0.5f, -0.5f, -0.5f,   0,-1,0,  1,0,
         0.5f, -0.5f,  0.5f,   0,-1,0,  1,1,  -0.5f, -0.5f,  0.5f,   0,-1,0,  0,1,
        // Right
         0.5f, -0.5f,  0.5f,   1,0,0,   0,0,   0.5f, -0.5f, -0.5f,   1,0,0,   1,0,
         0.5f,  0.5f, -0.5f,   1,0,0,   1,1,   0.5f,  0.5f,  0.5f,   1,0,0,   0,1,
        // Left
        -0.5f, -0.5f, -0.5f,  -1,0,0,   0,0,  -0.5f, -0.5f,  0.5f,  -1,0,0,   1,0,
        -0.5f,  0.5f,  0.5f,  -1,0,0,   1,1,  -0.5f,  0.5f, -0.5f,  -1,0,0,   0,1,
    };

    uint32_t indices[] = {
         0, 1, 2, 2, 3, 0,    4, 5, 6, 6, 7, 4,
         8, 9,10,10,11, 8,   12,13,14,14,15,12,
        16,17,18,18,19,16,   20,21,22,22,23,20,
    };

    graphics::VertexLayout layout;
    layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);

    auto vb = std::make_shared<graphics::VertexBuffer>();
    vb->Create(vertices, sizeof(vertices));

    auto ib = std::make_shared<graphics::IndexBuffer>();
    ib->Create(indices, 36);

    auto va = std::make_shared<graphics::VertexArray>();
    va->Create();
    va->AddVertexBuffer(vb, layout);
    va->SetIndexBuffer(ib);

    // use s_defaultShader directly

    // Solid-color texture (1x1 pixel)
    std::vector<uint8_t> px = {
        static_cast<uint8_t>(r * 255),
        static_cast<uint8_t>(g * 255),
        static_cast<uint8_t>(b * 255), 255
    };
    auto tex = std::make_shared<graphics::Texture>();
    tex->Create(1, 1, px.data(), TexInternalDataFormat::RGBA8, TexDataFormat::RGBA);

    auto mat = std::make_shared<graphics::Material>();
    mat->shader         = s_defaultShader;
    mat->diffuseTexture = tex;
    mat->shininess      = 64.0f;

    auto mesh = std::make_shared<graphics::Mesh>();
    mesh->vertexArray = va;
    mesh->material    = mat;
    return mesh;
}

static std::shared_ptr<graphics::Texture> CreateStripedTexture()
{
    const int ts = 64;
    std::vector<uint8_t> px(ts * ts * 4);
    for (int y = 0; y < ts; ++y)
        for (int x = 0; x < ts; ++x)
        {
            bool stripe = (x / 8) % 2 == 0;
            uint8_t c = stripe ? 255 : 80;
            size_t idx = (static_cast<size_t>(y) * ts + x) * 4;
            px[idx+0] = 220; px[idx+1] = c; px[idx+2] = 30; px[idx+3] = 255;
        }
    auto tex = std::make_shared<graphics::Texture>();
    tex->Create(ts, ts, px.data());
    return tex;
}

static std::shared_ptr<graphics::Mesh> CreateFloorMesh()
{
    float s = 10.0f;  // half-size
    float vertices[] = {
        -s, 0, -s,  0,1,0,  0,0,
         s, 0, -s,  0,1,0,  1,0,
         s, 0,  s,  0,1,0,  1,1,
        -s, 0,  s,  0,1,0,  0,1,
    };
    uint32_t indices[] = {2, 1, 0, 0, 3, 2};

    graphics::VertexLayout layout;
    layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);

    auto vb = std::make_shared<graphics::VertexBuffer>();
    vb->Create(vertices, sizeof(vertices));
    auto ib = std::make_shared<graphics::IndexBuffer>();
    ib->Create(indices, 6);
    auto va = std::make_shared<graphics::VertexArray>();
    va->Create();
    va->AddVertexBuffer(vb, layout);
    va->SetIndexBuffer(ib);

    // use s_defaultShader directly

    // Checkerboard texture
    const int ts = 64;
    std::vector<uint8_t> px(ts * ts * 4);
    for (int y = 0; y < ts; ++y)
        for (int x = 0; x < ts; ++x)
        {
            bool w = ((x / 8) + (y / 8)) % 2 == 0;
            uint8_t c = w ? 200 : 60;
            size_t idx = (static_cast<size_t>(y) * ts + x) * 4;
            px[idx+0]=c; px[idx+1]=c; px[idx+2]=c; px[idx+3]=255;
        }
    auto tex = std::make_shared<graphics::Texture>();
    tex->Create(ts, ts, px.data());
    tex->SetFilterLinear(false);  // sharp checkerboard edges

    auto mat = std::make_shared<graphics::Material>();
    mat->shader         = s_defaultShader;
    mat->diffuseTexture = tex;
    mat->ambientColor   = {0.8f, 0.8f, 0.8f};
    mat->diffuseColor   = {1.0f, 1.0f, 1.0f};
    mat->shininess      = 16.0f;

    auto mesh = std::make_shared<graphics::Mesh>();
    mesh->vertexArray = va;
    mesh->material    = mat;
    return mesh;
}

// ===================================================================

ApplicationConfig sgkit::CreateApplication()
{
    ApplicationConfig config;
    config.title  = "SGKit Sandbox";
    config.width  = 1280;
    config.height = 720;
    config.fullscreen = true;

    config.onInit = []() -> bool
    {
        s_defaultShader = std::make_shared<graphics::Shader>();
        s_defaultShader->LoadFromFile("assets/shaders/default.vert", "assets/shaders/default.frag");

        // Floor
        auto floorMesh = CreateFloorMesh();
        s_floorEntity = GetScene().CreateEntity();
        auto& ft = GetScene().AddComponent<scene::Transform>(s_floorEntity);
        ft.position = {0.0f, -0.5f, 0.0f};
        ft.scale    = {1.5f, 1.0f, 1.5f};
        auto& fmr = GetScene().AddComponent<scene::MeshRenderer>(s_floorEntity);
        fmr.mesh = floorMesh;

        // Colored cubes
        math::Vector3 colors[] = {
            {1.0f, 0.3f, 0.3f},
            {0.3f, 1.0f, 0.3f},
            {0.3f, 0.3f, 1.0f},
            {1.0f, 0.8f, 0.2f},
        };
        math::Vector3 positions[] = {
            {-2.0f, 0.0f, -1.0f},
            { 2.0f, 0.0f, -1.0f},
            {-2.0f, 0.0f,  2.0f},
            { 2.0f, 0.0f,  2.0f},
        };
        for (int i = 0; i < k_NumCubes; ++i)
        {
            auto mesh = CreateColoredCubeMesh(colors[i].x, colors[i].y, colors[i].z);
            // Give the yellow cube a striped texture
            if (i == 3)
            {
                auto stripedTex = CreateStripedTexture();
                mesh->material->diffuseTexture = stripedTex;
            }
            s_cubeEntities[i] = GetScene().CreateEntity();
            auto& t = GetScene().AddComponent<scene::Transform>(s_cubeEntities[i]);
            t.position = positions[i];
            t.scale = {0.8f, 0.8f, 0.8f};
            auto& mr = GetScene().AddComponent<scene::MeshRenderer>(s_cubeEntities[i]);
            mr.mesh = mesh;
        }

        // Camera
        s_cameraEntity = GetScene().CreateEntity();
        auto& ct = GetScene().AddComponent<scene::Transform>(s_cameraEntity);
        ct.position = {0.0f, 2.0f, 7.0f};
        auto& cam = GetScene().AddComponent<scene::Camera>(s_cameraEntity);
        cam.fovY = 60.0f;

        std::printf("Sandbox loaded: 4 cubes, floor, dir-light, point-light.\n");
        return true;
    };

    config.onUpdate = [](float dt)
    {
        // Rotate cubes
        for (int i = 0; i < k_NumCubes; ++i)
        {
            auto* t = GetScene().GetComponent<scene::Transform>(s_cubeEntities[i]);
            if (t)
            {
                float speed = 0.5f + i * 0.2f;
                t->rotation = math::Quaternion::FromEulerAngles(0.0f, speed * dt, 0.0f) * t->rotation;
            }
        }

        // Camera movement
        auto* ct = GetScene().GetComponent<scene::Transform>(s_cameraEntity);
        if (ct)
        {
            float s = 4.0f * dt;
            if (GetInput().IsKeyDown(core::KeyCode::k_W)) ct->position.z -= s;
            if (GetInput().IsKeyDown(core::KeyCode::k_S)) ct->position.z += s;
            if (GetInput().IsKeyDown(core::KeyCode::k_A)) ct->position.x -= s;
            if (GetInput().IsKeyDown(core::KeyCode::k_D)) ct->position.x += s;
            if (GetInput().IsKeyDown(core::KeyCode::k_Q)) ct->position.y -= s;
            if (GetInput().IsKeyDown(core::KeyCode::k_E)) ct->position.y += s;

            // Scroll wheel zoom
            float zoom = GetInput().GetScrollDelta();
            if (zoom != 0.0f)
            {
                math::Vector3 fwd = ct->rotation * math::Vector3{0.0f, 0.0f, -1.0f};
                ct->position = ct->position + fwd * zoom * 0.5f;
            }

            if (GetInput().IsMouseButtonDown(core::MouseButton::Right))
            {
                float look = 0.002f;
                float yaw   = -GetInput().GetMouseDeltaX() * look;
                float pitch = -GetInput().GetMouseDeltaY() * look;
                ct->rotation = math::Quaternion::FromEulerAngles(0.0f, yaw, 0.0f)
                             * ct->rotation
                             * math::Quaternion::FromEulerAngles(pitch, 0.0f, 0.0f);
            }
        }

        if (GetInput().IsKeyPressed(core::KeyCode::k_Space))
            GetWindow().SetFullscreen(!GetWindow().IsFullscreen());
    };

    config.onRender = []()
    {
        GetScene().OnRender(GetRenderer(), s_cameraEntity,
                            GetWindow().GetWidth(), GetWindow().GetHeight());
    };

    return config;
}
