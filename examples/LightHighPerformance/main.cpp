#include <sgkit/sgkit.h>

#include <vector>
#include <random>
#include <cstdio>

using namespace sgkit;

// -- Grid configuration
// 20 x 20 x 3 = 1200 entities, randomly split across 2 shaders:
//   simple (textured Blinn-Phong) + light (solid emissive glow).
// 8 material variants, 2 shaders.  Old path: up to 1200 shader binds.
// New path (RenderQueue): 2 shader binds, ~9 batches.
static constexpr int    GRID_X  = 20;
static constexpr int    GRID_Y  = 20;
static constexpr int    GRID_Z  = 3;
static constexpr float  SPACING = 1.8f;

// -- Resources
static std::shared_ptr<graphics::Shader>      g_shaderSimple;
static std::shared_ptr<graphics::Shader>      g_shaderLight;
static std::shared_ptr<graphics::VertexArray> g_vaoCube;
static std::shared_ptr<graphics::Texture>     g_texture;

// 6 material variants: 5 textured (simple) + 1 emissive (light)
static std::shared_ptr<graphics::Material> g_mats[6];

// Grid cells
struct Cell
{
    scene::Entity entity;
    float rotSpeedX, rotSpeedY, rotSpeedZ;
};
static std::vector<Cell> g_cells;

static scene::Entity              g_cameraEntity;
static std::vector<scene::Entity> g_lightEntities;

// FPS
static float g_fpsTimer   = 0.0f;
static int   g_frameCount = 0;

// -- Helpers

static std::shared_ptr<graphics::VertexArray> MakeCubeVAO()
{
    float vertices[] = {
        -0.5f,-0.5f, 0.5f,   0,0,1,   0,0,   0.5f,-0.5f, 0.5f,   0,0,1,   1,0,
         0.5f, 0.5f, 0.5f,   0,0,1,   1,1,  -0.5f, 0.5f, 0.5f,   0,0,1,   0,1,

         0.5f,-0.5f,-0.5f,   0,0,-1,  0,0,  -0.5f,-0.5f,-0.5f,   0,0,-1,  1,0,
        -0.5f, 0.5f,-0.5f,   0,0,-1,  1,1,   0.5f, 0.5f,-0.5f,   0,0,-1,  0,1,

        -0.5f, 0.5f, 0.5f,   0,1,0,   0,0,   0.5f, 0.5f, 0.5f,   0,1,0,   1,0,
         0.5f, 0.5f,-0.5f,   0,1,0,   1,1,  -0.5f, 0.5f,-0.5f,   0,1,0,   0,1,

        -0.5f,-0.5f,-0.5f,   0,-1,0,  0,0,   0.5f,-0.5f,-0.5f,   0,-1,0,  1,0,
         0.5f,-0.5f, 0.5f,   0,-1,0,  1,1,  -0.5f,-0.5f, 0.5f,   0,-1,0,  0,1,

         0.5f,-0.5f, 0.5f,   1,0,0,   0,0,   0.5f,-0.5f,-0.5f,   1,0,0,   1,0,
         0.5f, 0.5f,-0.5f,   1,0,0,   1,1,   0.5f, 0.5f, 0.5f,   1,0,0,   0,1,

        -0.5f,-0.5f,-0.5f,  -1,0,0,   0,0,  -0.5f,-0.5f, 0.5f,  -1,0,0,   1,0,
        -0.5f, 0.5f, 0.5f,  -1,0,0,   1,1,  -0.5f, 0.5f,-0.5f,  -1,0,0,   0,1,
    };
    uint32_t indices[] = {
         0, 1, 2,  2, 3, 0,    4, 5, 6,  6, 7, 4,    8, 9,10, 10,11, 8,
        12,13,14, 14,15,12,   16,17,18, 18,19,16,   20,21,22, 22,23,20,
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
    return va;
}

// -- Application

ApplicationConfig sgkit::CreateApplication()
{
    ApplicationConfig cfg{};
    cfg.title   = "SGKit Stress Test - 1200 cubes";
    cfg.width   = 1280;
    cfg.height  = 720;
    cfg.vsync   = false;
    cfg.fullscreen          = true;
    cfg.fullscreenBolderless = true;

    cfg.onInit = []() -> bool
    {
        auto& scene = scene::Scene::instance();

        // -- Shaders
        g_shaderSimple = std::make_shared<graphics::Shader>();
        g_shaderLight  = std::make_shared<graphics::Shader>();
        if (!g_shaderSimple->LoadFromFile("assets/simple.vert", "assets/simple.frag"))
        { std::fprintf(stderr, "simple shader failed\n"); return false; }
        if (!g_shaderLight->LoadFromFile("assets/light.vert", "assets/light.frag"))
        { std::fprintf(stderr, "light shader failed\n"); return false; }

        // -- Shared geometry
        g_texture = std::make_shared<graphics::Texture>();
        g_texture->LoadFromFile("assets/container2.bmp");

        g_vaoCube = MakeCubeVAO();

        // -- Material variants
        // 5 textured (simple) + 1 emissive (light).
        // Emissive cubes are only ~10 % of the grid - the rest are textured.
        // Each Material* is a separate RenderQueue batch.

        // --- simple shader variants (textured Blinn-Phong)
        g_mats[0] = std::make_shared<graphics::Material>();
        g_mats[0]->shader    = g_shaderSimple;
        g_mats[0]->diffuse   = g_texture;
        g_mats[0]->specular  = math::Vector3{1.0f, 0.7f, 0.3f};
        g_mats[0]->shininess = 128.0f;

        g_mats[1] = std::make_shared<graphics::Material>();
        g_mats[1]->shader    = g_shaderSimple;
        g_mats[1]->diffuse   = g_texture;
        g_mats[1]->specular  = math::Vector3{0.3f, 0.5f, 1.0f};
        g_mats[1]->shininess = 32.0f;

        g_mats[2] = std::make_shared<graphics::Material>();
        g_mats[2]->shader    = g_shaderSimple;
        g_mats[2]->diffuse   = g_texture;
        g_mats[2]->specular  = math::Vector3{0.3f, 0.9f, 0.4f};
        g_mats[2]->shininess = 64.0f;

        g_mats[3] = std::make_shared<graphics::Material>();
        g_mats[3]->shader    = g_shaderSimple;
        g_mats[3]->diffuse   = g_texture;
        g_mats[3]->specular  = math::Vector3{1.0f, 0.3f, 0.3f};
        g_mats[3]->shininess = 256.0f;

        g_mats[4] = std::make_shared<graphics::Material>();
        g_mats[4]->shader    = g_shaderSimple;
        g_mats[4]->diffuse   = g_texture;
        g_mats[4]->specular  = math::Vector3{0.4f, 0.4f, 0.4f};
        g_mats[4]->shininess = 8.0f;

        // --- light shader variant (solid emissive, ~10 % of grid)
        g_mats[5] = std::make_shared<graphics::Material>();
        g_mats[5]->shader   = g_shaderLight;
        g_mats[5]->specular = math::Vector3{1.0f, 0.5f, 0.1f};  // orange glow

        constexpr int k_NumMats = 6;

        // -- Spawn grid
        g_cells.reserve(GRID_X * GRID_Y * GRID_Z);
        std::mt19937 rng(42);

        float offX = (GRID_X - 1) * SPACING * 0.5f;
        float offY = (GRID_Y - 1) * SPACING * 0.5f;
        float offZ = (GRID_Z - 1) * SPACING * 0.5f;

        for (int z = 0; z < GRID_Z; ++z)
        for (int y = 0; y < GRID_Y; ++y)
        for (int x = 0; x < GRID_X; ++x)
        {
            Cell cell;
            cell.entity = scene.CreateEntity();

            auto* tf = scene.AddComponent<scene::Transform>(cell.entity);
            tf->position = {x * SPACING - offX, y * SPACING - offY, z * SPACING - offZ};
            tf->scale    = math::Vector3{0.6f, 0.6f, 0.6f};

            auto mesh = std::make_shared<graphics::Mesh>();
            mesh->vertexArray = g_vaoCube;

            // ~10 % emissive, ~90 % textured
            int matIdx = (rng() % 10 == 0) ? 5 : (rng() % 5);
            mesh->material = g_mats[matIdx];

            auto* mr = scene.AddComponent<scene::MeshRenderer>(cell.entity);
            mr->mesh = mesh;

            cell.rotSpeedX = std::uniform_real_distribution<float>(-0.8f, 0.8f)(rng);
            cell.rotSpeedY = std::uniform_real_distribution<float>(-0.8f, 0.8f)(rng);
            cell.rotSpeedZ = std::uniform_real_distribution<float>(-0.8f, 0.8f)(rng);

            g_cells.push_back(cell);
        }

        // -- Light sources (with visible markers)
        // Position #0 above centre - the backward-compat single-light shader
        // uses this one for diffuse / specular calculations.
        math::Vector3 lightPositions[] = {
            {  0.0f,  0.0f, offZ * 2.0f },  // above centre (key light)
            {-offX * 0.7f,  offY * 0.7f, offZ * 1.2f },
            { offX * 0.7f, -offY * 0.7f, offZ * 1.2f },
            {  0.0f,  0.0f, -offZ * 1.5f }, // below centre (fill)
        };

        // Shared material for all light markers -> 1 batch
        auto matLightMarker = std::make_shared<graphics::Material>();
        matLightMarker->shader = g_shaderLight;

        for (int i = 0; i < 4; ++i)
        {
            scene::Entity le = scene.CreateEntity();

            auto* lc = scene.AddComponent<scene::Light>(le);
            lc->ambient  = math::Vector3{0.35f, 0.35f, 0.40f};
            lc->diffuse  = math::Vector3{1.0f, 0.95f, 0.85f};
            lc->specular = math::Vector3{1.0f, 1.0f, 1.0f};

            auto* lt = scene.AddComponent<scene::Transform>(le);
            lt->position = lightPositions[i];
            lt->scale    = math::Vector3{2.0f, 2.0f, 2.0f};

            auto lightMarker = std::make_shared<graphics::Mesh>();
            lightMarker->vertexArray = g_vaoCube;
            lightMarker->material    = matLightMarker;

            auto* mr = scene.AddComponent<scene::MeshRenderer>(le);
            mr->mesh = lightMarker;

            g_lightEntities.push_back(le);
        }

        // -- Camera
        g_cameraEntity = scene.CreateEntity();
        scene.AddComponent<scene::Camera>(g_cameraEntity);
        auto* camTf = scene.AddComponent<scene::Transform>(g_cameraEntity);
        camTf->position = {0.0f, 0.0f, GRID_Y * SPACING * 0.9f};

        int total = static_cast<int>(g_cells.size());
        core::DebugOut("==================================================");
        core::DebugOut(" SGKit RenderQueue Stress Test");
        std::printf(" Entities : %d  (%dx%dx%d grid + %zu lights)\n",
                    total + 4, GRID_X, GRID_Y, GRID_Z, g_lightEntities.size());
        std::printf(" Shaders  : 2  (simple + light)\n");
        std::printf(" Materials: %d  (5 grid variants + light marker)\n", k_NumMats + 1);
        std::printf(" Batches  : %d  (5 textured + 1 emissive + 1 marker)\n", k_NumMats + 1);
        std::printf(" Shader binds/frame: 2  (old path: up to %d)\n", total + 4);
        core::DebugOut("==================================================");
        return true;
    };

    // -- Update

    cfg.onUpdate = [&]()
    {
        float dt = framework::Clock::GetFrameDeltaSeconds();

        // Spin each cube at its own random speed
        for (auto& cell : g_cells)
        {
            auto* tf = scene::Scene::instance().GetComponent<scene::Transform>(cell.entity);
            if (tf)
            {
                tf->rotation = math::Quaternion::FromEulerAngles(
                    cell.rotSpeedX * dt, cell.rotSpeedY * dt, cell.rotSpeedZ * dt)
                    * tf->rotation;
                tf->rotation.Normalize();
            }
        }

        // FPS counter (print every second)
        g_fpsTimer += dt;
        g_frameCount++;
        if (g_fpsTimer >= 1.0f)
        {
            float fps = static_cast<float>(g_frameCount) / g_fpsTimer;
            std::printf("[FPS] %7.1f  (frame time: %.2f ms)\n", fps, dt * 1000.0f);
            g_fpsTimer   = 0.0f;
            g_frameCount = 0;
        }

        // Camera: WASD + mouse look
        auto ct = scene::Scene::instance().GetComponent<scene::Transform>(g_cameraEntity);
        if (ct)
        {
            math::Vector3 fwd   = ct->rotation * math::Vector3{0, 0, -1};
            math::Vector3 right = ct->rotation * math::Vector3{1, 0,  0};
            float speed = 8.0f * dt;
            auto& in = core::Input::instance();
            if (in.IsKeyDown(core::KeyCode::W)) ct->position += fwd   * speed;
            if (in.IsKeyDown(core::KeyCode::S)) ct->position -= fwd   * speed;
            if (in.IsKeyDown(core::KeyCode::A)) ct->position -= right * speed;
            if (in.IsKeyDown(core::KeyCode::D)) ct->position += right * speed;
            if (in.IsKeyDown(core::KeyCode::Q)) ct->position.y -= speed;
            if (in.IsKeyDown(core::KeyCode::E)) ct->position.y += speed;

            if (in.IsMouseButtonDown(core::MouseButton::Left))
            {
                float s     = 0.002f;
                float yaw   = -in.GetMouseDeltaX() * s;
                float pitch = -in.GetMouseDeltaY() * s;
                ct->rotation = math::Quaternion::FromEulerAngles(0, yaw, 0)
                             * ct->rotation
                             * math::Quaternion::FromEulerAngles(pitch, 0, 0);
                ct->rotation.Normalize();
            }
        }

        auto& window = core::Window::instance();
        auto& in     = core::Input::instance();
        if (in.IsKeyPressed(core::KeyCode::Space)) window.SetFullscreen(!window.IsFullscreen());
        if (in.IsKeyPressed(core::KeyCode::Escape)) window.RequestClose();
    };

    // -- Render

    cfg.onRender = []()
    {
        scene::Scene::instance().Render(g_cameraEntity);
    };

    // -- Shutdown

    cfg.onShutdown = []()
    {
        g_cells.clear();
        g_lightEntities.clear();
        for (auto& m : g_mats) m.reset();
        g_vaoCube.reset();
        g_texture.reset();
        g_shaderSimple.reset();
        g_shaderLight.reset();
    };

    return cfg;
}
