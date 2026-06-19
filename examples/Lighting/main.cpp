#include <sgkit/sgkit.h>

#include <cstdio>
#include <memory>
#include <vector>

using namespace sgkit;

static scene::Entity s_floorEntity = scene::k_InvalidEntity;
static scene::Entity s_flagEntity = scene::k_InvalidEntity;
static scene::Entity s_cameraEntity = scene::k_InvalidEntity;
static constexpr int k_NumCubes = 4;
static scene::Entity s_cubeEntities[k_NumCubes] = {};
static std::shared_ptr<graphics::Shader> s_defaultShader;

namespace {

    std::shared_ptr<graphics::Mesh> CreateColoredCube(float r, float g, float b)
    {
        float vertices[] = {
            -0.5f, -0.5f,  0.5f,  0,0,1,  0,0,   0.5f, -0.5f,  0.5f,  0,0,1,  1,0,
             0.5f,  0.5f,  0.5f,  0,0,1,  1,1,  -0.5f,  0.5f,  0.5f,  0,0,1,  0,1,
             0.5f, -0.5f, -0.5f,  0,0,-1, 0,0,  -0.5f, -0.5f, -0.5f,  0,0,-1, 1,0,
            -0.5f,  0.5f, -0.5f,  0,0,-1, 1,1,   0.5f,  0.5f, -0.5f,  0,0,-1, 0,1,
            -0.5f,  0.5f,  0.5f,  0,1,0,  0,0,   0.5f,  0.5f,  0.5f,  0,1,0,  1,0,
             0.5f,  0.5f, -0.5f,  0,1,0,  1,1,  -0.5f,  0.5f, -0.5f,  0,1,0,  0,1,
            -0.5f, -0.5f, -0.5f,  0,-1,0, 0,0,   0.5f, -0.5f, -0.5f,  0,-1,0, 1,0,
             0.5f, -0.5f,  0.5f,  0,-1,0, 1,1,  -0.5f, -0.5f,  0.5f,  0,-1,0, 0,1,
             0.5f, -0.5f,  0.5f,  1,0,0,  0,0,   0.5f, -0.5f, -0.5f,  1,0,0,  1,0,
             0.5f,  0.5f, -0.5f,  1,0,0,  1,1,   0.5f,  0.5f,  0.5f,  1,0,0,  0,1,
            -0.5f, -0.5f, -0.5f, -1,0,0,  0,0,  -0.5f, -0.5f,  0.5f, -1,0,0,  1,0,
            -0.5f,  0.5f,  0.5f, -1,0,0,  1,1,  -0.5f,  0.5f, -0.5f, -1,0,0,  0,1,
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

        std::vector<uint8_t> px = {
            static_cast<uint8_t>(r * 255), static_cast<uint8_t>(g * 255),
            static_cast<uint8_t>(b * 255), 255
        };
        auto tex = std::make_shared<graphics::Texture>();
        tex->Create(1, 1, px.data());

        auto mat = std::make_shared<graphics::Material>();
        mat->shader = s_defaultShader;
        mat->diffuseTexture = tex;
        mat->shininess = 64.0f;

        auto mesh = std::make_shared<graphics::Mesh>();
        mesh->vertexArray = va;
        mesh->material = mat;
        return mesh;
    }

    std::shared_ptr<graphics::Mesh> CreateFloor()
    {
        float s = 10.0f;
        float vertices[] = {
            -s, 0, -s,  0,1,0,  0,0,   s, 0, -s,  0,1,0,  1,0,
             s, 0,  s,  0,1,0,  1,1,  -s, 0,  s,  0,1,0,  0,1,
        };
        uint32_t indices[] = { 2, 1, 0, 0, 3, 2 };

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

        const int ts = 64;
        std::vector<uint8_t> px(ts * ts * 4);
        for (int y = 0; y < ts; ++y)
            for (int x = 0; x < ts; ++x)
            {
                bool w = ((x / 8) + (y / 8)) % 2 == 0;
                uint8_t c = w ? 200 : 60;
                size_t idx = (static_cast<size_t>(y) * ts + x) * 4;
                px[idx + 0] = c; px[idx + 1] = c; px[idx + 2] = c; px[idx + 3] = 255;
            }
        auto tex = std::make_shared<graphics::Texture>();
        tex->Create(ts, ts, px.data());
        tex->SetFilterLinear(false);

        auto mat = std::make_shared<graphics::Material>();
        mat->shader = s_defaultShader;
        mat->diffuseTexture = tex;
        mat->ambientColor = { 0.3f, 0.3f, 0.3f };
        mat->diffuseColor = { 1.0f, 1.0f, 1.0f };
        mat->shininess = 16.0f;

        auto mesh = std::make_shared<graphics::Mesh>();
        mesh->vertexArray = va;
        mesh->material = mat;
        return mesh;
    }

    std::shared_ptr<graphics::Mesh> CreateFlag()
    {
        float w = 1.4f;
        float y0 = 0.2f;
        float y1 = 3.0f;
        float vertices[] = {
            -w, y0, 0.0f,   0,0,1,   0,0,
             w, y0, 0.0f,   0,0,1,   1,0,
             w, y1, 0.0f,   0,0,1,   1,1,
            -w, y1, 0.0f,   0,0,1,   0,1,
        };
        uint32_t indices[] = { 0, 1, 2, 2, 3, 0 };

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

        auto tex = std::make_shared<graphics::Texture>();
        tex->LoadFromFile("assets/logo.bmp");

        auto mat = std::make_shared<graphics::Material>();
        mat->shader = s_defaultShader;
        mat->diffuseTexture = tex;

        auto mesh = std::make_shared<graphics::Mesh>();
        mesh->vertexArray = va;
        mesh->material = mat;
        return mesh;
    }

} // anonymous namespace

ApplicationConfig sgkit::CreateApplication()
{
    ApplicationConfig cfg;
    cfg.title = "SGKit Lighting";
    cfg.width = 1280;
    cfg.height = 720;

    cfg.onInit = []() -> bool
        {
            s_defaultShader = std::make_shared<graphics::Shader>();
            s_defaultShader->LoadFromFile("assets/shaders/default.vert", "assets/shaders/default.frag");

            auto floorMesh = CreateFloor();
            s_floorEntity = GetScene().CreateEntity();
            auto& ft = GetScene().AddComponent<scene::Transform>(s_floorEntity);
            ft.position = { 0.0f, -0.5f, 0.0f };
            ft.scale = { 1.5f, 1.0f, 1.5f };
            GetScene().AddComponent<scene::MeshRenderer>(s_floorEntity).mesh = floorMesh;

            math::Vector3 colors[] = {
                {0.8f, 0.2f, 0.2f}, {0.2f, 0.8f, 0.2f},
                {0.2f, 0.2f, 0.8f}, {1.0f, 1.0f, 1.0f},
            };
            math::Vector3 pos[] = {
                {-2, 0, -2}, { 2, 0, -2}, {-2, 0,  2}, { 2, 0,  2},
            };
            for (int i = 0; i < k_NumCubes; ++i)
            {
                auto m = CreateColoredCube(colors[i].x, colors[i].y, colors[i].z);
                if (i == 3)  // textured cube
                {
                    auto tex = std::make_shared<graphics::Texture>();
                    tex->LoadFromFile("assets/tex.bmp");
                    m->material->diffuseTexture = tex;
                }
                s_cubeEntities[i] = GetScene().CreateEntity();
                auto& t = GetScene().AddComponent<scene::Transform>(s_cubeEntities[i]);
                t.position = pos[i];
                t.scale = { 0.8f, 0.8f, 0.8f };
                GetScene().AddComponent<scene::MeshRenderer>(s_cubeEntities[i]).mesh = m;
            }

            // Flag — facing +Z, same as camera initial look direction
            auto flagMesh = CreateFlag();
            s_flagEntity = GetScene().CreateEntity();
            GetScene().AddComponent<scene::Transform>(s_flagEntity);
            GetScene().AddComponent<scene::MeshRenderer>(s_flagEntity).mesh = flagMesh;

            s_cameraEntity = GetScene().CreateEntity();
            auto& ct = GetScene().AddComponent<scene::Transform>(s_cameraEntity);
            ct.position = { 0.0f, 2.0f, 10.0f };
            auto& cam = GetScene().AddComponent<scene::Camera>(s_cameraEntity);
            cam.fovY = 60.0f;

            std::printf("Lighting: flag, 4 cubes, floor.\n");
            return true;
        };

    cfg.onUpdate = [](float dt)
        {
            auto* ct = GetScene().GetComponent<scene::Transform>(s_cameraEntity);
            if (!ct) return;

            for (int i = 0; i < k_NumCubes; ++i)
            {
                auto* t = GetScene().GetComponent<scene::Transform>(s_cubeEntities[i]);
                if (t)
                    t->rotation = math::Quaternion::FromEulerAngles(0, (0.3f + i * 0.2f) * dt, 0) * t->rotation;
            }

            math::Vector3 forward = ct->rotation * math::Vector3{ 0.0f, 0.0f, -1.0f };
            math::Vector3 right = ct->rotation * math::Vector3{ 1.0f, 0.0f,  0.0f };

            float speed = 5.0f * dt;
            auto& in = GetInput();
            if (in.IsKeyDown(core::KeyCode::k_W)) ct->position += forward * speed;
            if (in.IsKeyDown(core::KeyCode::k_S)) ct->position -= forward * speed;
            if (in.IsKeyDown(core::KeyCode::k_A)) ct->position -= right * speed;
            if (in.IsKeyDown(core::KeyCode::k_D)) ct->position += right * speed;
            if (in.IsKeyDown(core::KeyCode::k_Q)) ct->position.y -= speed;
            if (in.IsKeyDown(core::KeyCode::k_E)) ct->position.y += speed;

            if (in.IsMouseButtonDown(core::MouseButton::Left))
            {
                float s = 0.002f;
                float yaw = -in.GetMouseDeltaX() * s;
                float pitch = -in.GetMouseDeltaY() * s;
                ct->rotation = math::Quaternion::FromEulerAngles(0, yaw, 0)
                    * ct->rotation
                    * math::Quaternion::FromEulerAngles(pitch, 0, 0);
                ct->rotation.Normalize();
            }
        };

    cfg.onRender = []()
        {
            GetScene().OnRender(GetRenderer(), s_cameraEntity,
                GetWindow().GetWidth(), GetWindow().GetHeight());
        };

    return cfg;
}
