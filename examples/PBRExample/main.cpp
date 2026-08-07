#include <sgkit/sgkit.h>

using namespace sgkit;

// ============================================================================
// Global state
// ============================================================================

static scene::Entity                       s_camera;
static std::shared_ptr<graphics::Shader>   s_pbrShader;
static std::shared_ptr<graphics::Shader>   s_blinnPhongShader;
static std::shared_ptr<graphics::Shader>   s_simpleShader;

// Shared 1*1 procedural textures (reused across PBR demo objects)
static std::shared_ptr<graphics::Texture>  s_whiteTex;       // RGBA (1,1,1,1)
static std::shared_ptr<graphics::Texture>  s_flatNormalTex;  // RGBA (128,128,255,255)

// PBR instanced grid - 25 entities sharing one VAO + one material
static std::vector<scene::Entity>          s_gridEntities;

// External model
static scene::Entity                       s_modelRoot;
static scene::Entity                       s_modelMarker;  // origin marker, NOT parented (survives T toggle)
static std::vector<scene::Entity>          s_modelEntities;
static bool                                s_modelVisible = true;

// Camera orbit
static float s_orbitRadius = 8.0f;
static float s_orbitYaw    = 0.0f;
static float s_orbitPitch  = 0.3f;

// ============================================================================
// Procedural mesh helpers
// ============================================================================

// Cube vertices: pos(3) normal(3) tex(2) = 8 floats/vertex
static constexpr float kCubeVerts[] = {
    // Front
    -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,   0.5f,-0.5f, 0.5f, 0,0,1, 1,0,
     0.5f, 0.5f, 0.5f, 0,0,1, 1,1,  -0.5f, 0.5f, 0.5f, 0,0,1, 0,1,
    // Back
     0.5f,-0.5f,-0.5f, 0,0,-1,0,0,  -0.5f,-0.5f,-0.5f, 0,0,-1,1,0,
    -0.5f, 0.5f,-0.5f, 0,0,-1,1,1,   0.5f, 0.5f,-0.5f, 0,0,-1,0,1,
    // Top
    -0.5f, 0.5f, 0.5f, 0,1,0, 0,0,   0.5f, 0.5f, 0.5f, 0,1,0, 1,0,
     0.5f, 0.5f,-0.5f, 0,1,0, 1,1,  -0.5f, 0.5f,-0.5f, 0,1,0, 0,1,
    // Bottom
    -0.5f,-0.5f,-0.5f, 0,-1,0,0,0,   0.5f,-0.5f,-0.5f, 0,-1,0,1,0,
     0.5f,-0.5f, 0.5f, 0,-1,0,1,1,  -0.5f,-0.5f, 0.5f, 0,-1,0,0,1,
    // Right
     0.5f,-0.5f, 0.5f, 1,0,0, 0,0,   0.5f,-0.5f,-0.5f, 1,0,0, 1,0,
     0.5f, 0.5f,-0.5f, 1,0,0, 1,1,   0.5f, 0.5f, 0.5f, 1,0,0, 0,1,
    // Left
    -0.5f,-0.5f,-0.5f,-1,0,0, 0,0,  -0.5f,-0.5f, 0.5f,-1,0,0, 1,0,
    -0.5f, 0.5f, 0.5f,-1,0,0, 1,1,  -0.5f, 0.5f,-0.5f,-1,0,0, 0,1,
};

static constexpr uint32_t kCubeIndices[] = {
     0, 1, 2, 2, 3, 0,    4, 5, 6, 6, 7, 4,
     8, 9,10,10,11, 8,   12,13,14,14,15,12,
    16,17,18,18,19,16,   20,21,22,22,23,20,
};

static std::shared_ptr<scene::Mesh> MakeCubeMesh(
    std::shared_ptr<scene::Material> material)
{
    graphics::VertexLayout layout;
    layout.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);

    auto vb = std::make_shared<graphics::VertexBuffer>();
    vb->Create(kCubeVerts, sizeof(kCubeVerts));

    auto ib = std::make_shared<graphics::IndexBuffer>();
    ib->Create(kCubeIndices, sizeof(kCubeIndices) / sizeof(uint32_t));

    auto va = std::make_shared<graphics::VertexArray>();
    va->Create();
    va->SetVertexBuffer(vb, layout);
    va->SetIndexBuffer(ib);

    auto mesh = std::make_shared<scene::Mesh>();
    mesh->vertexArray = va;
    mesh->material    = material;
    return mesh;
}

// ============================================================================
// PBR material helper  (no textures -> uses 1*1 fallbacks + factor uniforms)
// ============================================================================

static std::shared_ptr<scene::Material> MakePBRMaterial(
    float r, float g, float b,     // albedo colour
    float metallic,
    float roughness)
{
    // Coloured 1*1 albedo
    uint8_t albedoPx[4] = {
        static_cast<uint8_t>(r * 255),
        static_cast<uint8_t>(g * 255),
        static_cast<uint8_t>(b * 255),
        255
    };
    auto albedoTex = std::make_shared<graphics::Texture>(0);
    albedoTex->Create(1, 1, albedoPx);

    auto mat = std::make_shared<scene::Material>();
    mat->shader         = s_pbrShader;
    mat->lightingModel  = scene::LightingModel::PBR;
    mat->albedo         = albedoTex;
    mat->metallic       = s_whiteTex;       // 1*1 white * metallicFactor
    mat->roughness      = s_whiteTex;       // 1*1 white * roughnessFactor
    mat->normalMap      = s_flatNormalTex;  // (0,0,1) flat
    mat->ao             = s_whiteTex;       // full AO
    mat->metallicFactor  = metallic;
    mat->roughnessFactor = roughness;
    return mat;
}

// ============================================================================
// CreateApplication
// ============================================================================

ApplicationConfig sgkit::CreateApplication()
{
    ApplicationConfig cfg{};
    cfg.title  = "SGKit PBR Example";
    cfg.width  = 1280;
    cfg.height = 720;
    cfg.fullscreen = true;
    cfg.fullscreenBolderless = true;

    cfg.onInit = [&]() -> bool
    {
        auto& sm = scene::Scene::instance();

        // -- 1. Load shaders --------------------------------------------------
        s_pbrShader = std::make_shared<graphics::Shader>();
        if (!s_pbrShader->LoadFromFile("assets/shaders/pbr.vert",
                                        "assets/shaders/pbr.frag"))
        {
            SGK_LOG_ERROR("PBR", "Failed to load PBR shader");
            return false;
        }

        s_blinnPhongShader = std::make_shared<graphics::Shader>();
        if (!s_blinnPhongShader->LoadFromFile("assets/shaders/blinn_phong.vert",
                                               "assets/shaders/blinn_phong.frag"))
        {
            SGK_LOG_ERROR("PBR", "Failed to load Blinn-Phong shader");
            return false;
        }

        s_simpleShader = std::make_shared<graphics::Shader>();
        s_simpleShader->LoadFromFile("assets/shaders/simple.vert",
                                      "assets/shaders/simple.frag");

        // -- 2. Shared 1*1 textures -------------------------------------------
        {
            uint8_t white[]      = {255, 255, 255, 255};
            uint8_t flatNormal[] = {128, 128, 255, 255};  // (0,0,1) in tangent space

            s_whiteTex = std::make_shared<graphics::Texture>(1);
            s_whiteTex->Create(1, 1, white);

            s_flatNormalTex = std::make_shared<graphics::Texture>(3);
            s_flatNormalTex->Create(1, 1, flatNormal);
        }

        // -- 3. PBR material showcase: 5*5 instanced grid ---------------------
        {
            // One shared VAO
            graphics::VertexLayout lo;
            lo.PushFloat(0,3).PushFloat(1,3).PushFloat(2,2);
            auto vb = std::make_shared<graphics::VertexBuffer>();
            vb->Create(kCubeVerts, sizeof(kCubeVerts));
            auto ib = std::make_shared<graphics::IndexBuffer>();
            ib->Create(kCubeIndices, sizeof(kCubeIndices)/sizeof(uint32_t));
            auto va = std::make_shared<graphics::VertexArray>();
            va->Create(); va->SetVertexBuffer(vb, lo); va->SetIndexBuffer(ib);

            // One shared material
            auto mat = MakePBRMaterial(1.0f, 0.6f, 0.2f, 0.0f, 0.5f);
            auto mesh = std::make_shared<scene::Mesh>();
            mesh->vertexArray = va;
            mesh->material    = mat;

            constexpr int N = 5;
            constexpr float spacing = 1.8f;
            const float sx = -(N-1)*spacing*0.5f, sz = -(N-1)*spacing*0.5f;
            for (int r = 0; r < N; ++r)
                for (int c = 0; c < N; ++c)
                {
                    scene::Entity e = sm.CreateEntity();
                    auto* tf = sm.AddComponent<scene::component::Transform>(e);
                    tf->position = {sx + c*spacing, 0.5f, sz + r*spacing};
                    tf->scale    = {0.8f,0.8f,0.8f};
                    sm.AddComponent<scene::component::MeshRenderer>(e)->mesh = mesh;
                    s_gridEntities.push_back(e);
                }
        }

        // -- 4. Ground plane (Blinn-Phong, large flat quad) -------------------
        {
            // Quad: 2 triangles, facing up (Y-up)
            constexpr float quadVerts[] = {
                -10, 0, -10,  0,1,0,  0,10,
                 10, 0, -10,  0,1,0,  10,10,
                 10, 0,  10,  0,1,0,  10,0,
                -10, 0,  10,  0,1,0,  0,0,
            };
            constexpr uint32_t quadIdx[] = { 0,2,1, 0,3,2 };

            graphics::VertexLayout lo;
            lo.PushFloat(0,3).PushFloat(1,3).PushFloat(2,2);

            auto vb = std::make_shared<graphics::VertexBuffer>();
            vb->Create(quadVerts, sizeof(quadVerts));
            auto ib = std::make_shared<graphics::IndexBuffer>();
            ib->Create(quadIdx, 6);
            auto va = std::make_shared<graphics::VertexArray>();
            va->Create(); va->SetVertexBuffer(vb, lo); va->SetIndexBuffer(ib);

            // Gray diffuse ground
            uint8_t grayPx[] = { 80, 80, 80, 255 };
            auto diffTex = std::make_shared<graphics::Texture>(0);
            diffTex->Create(1, 1, grayPx);

            auto mat = std::make_shared<scene::Material>();
            mat->shader    = s_blinnPhongShader;
            mat->diffuse   = diffTex;
            mat->shininess = 8.0f;

            auto mesh = std::make_shared<scene::Mesh>();
            mesh->vertexArray = va;
            mesh->material    = mat;

            scene::Entity ground = sm.CreateEntity();
            auto* tf = sm.AddComponent<scene::component::Transform>(ground);
            tf->position = { 0, -0.6f, 0 };
            sm.AddComponent<scene::component::MeshRenderer>(ground)->mesh = mesh;
        }

        // -- 5. External model (optional) -------------------------------------
        {
            // Try common paths - first match wins
            const char* modelPaths[] = {
                //"assets/models/star_wars_model.glb",
                "assets/models/space_ship_torb.glb",
                //"assets/models/cute cartoon girl.glb"
            };
            for (auto* path : modelPaths)
            {
                auto result = scene::Model::Load(path, s_blinnPhongShader, s_pbrShader);
                if (result.root != scene::Entity::Invalid)
                {
                    s_modelRoot    = result.root;
                    s_modelEntities = result.entities;

                    auto* rootTf = sm.GetComponent<scene::component::Transform>(s_modelRoot);
                    rootTf->position = { 0.0f, 4.0f, 0.0f };
                    rootTf->scale    = { 0.3f, 0.3f, 0.3f };
                    // rootTf->position = { 0.0f, 1.0f, 0.0f };
                    // rootTf->scale    = { 2.0f, 2.0f, 2.0f };

                    // Origin marker: small glowing cube at model position (NOT parented, so T only hides model)
                    {
                        auto markerMat = std::make_shared<scene::Material>();
                        markerMat->shader     = s_simpleShader;
                        markerMat->blendMode  = scene::BlendMode::Additive;
                        markerMat->depthMode  = scene::DepthMode::ReadOnly;
                        markerMat->cullMode   = scene::CullMode::None;
                        auto markerMesh = MakeCubeMesh(markerMat);

                        s_modelMarker = sm.CreateEntity();
                        auto* mt = sm.AddComponent<scene::component::Transform>(s_modelMarker);
                        mt->scale    = { 0.15f, 0.15f, 0.15f };
                        mt->position = { 0, 0, 0 };  // same world pos as model root
                        sm.AddComponent<scene::component::MeshRenderer>(s_modelMarker)->mesh = markerMesh;
                    }
                    break;
                }
            }
        }

        // -- 6. Camera --------------------------------------------------------
        s_camera = sm.CreateEntity();
        auto* ct = sm.AddComponent<scene::component::Transform>(s_camera);
        ct->position = { 0.0f, 3.0f, 8.0f };
        sm.AddComponent<scene::component::Camera>(s_camera);

        // -- 7. Lights --------------------------------------------------------
        {
            // Directional - sun
            {
                scene::Entity e = sm.CreateEntity();
                sm.AddComponent<scene::component::Transform>(e);
                auto* l = sm.AddComponent<scene::component::Light>(e);
                l->type      = scene::component::Light::Type::Directional;
                l->direction = { 0.5f, -1.0f, 0.3f };
                l->ambient   = { 0.18f, 0.18f, 0.20f };
                l->diffuse   = { 2.0f, 1.9f, 1.6f };
                l->specular  = { 1.0f, 1.0f, 1.0f };
            }

            // Four point lights at corners (warm/cool alternating)
            auto makeLightMarker = [&](const math::Vector3& pos,
                                        const math::Vector3& color) -> std::shared_ptr<scene::Mesh>
            {
                auto mat = std::make_shared<scene::Material>();
                mat->shader     = s_simpleShader;
                mat->blendMode  = scene::BlendMode::Additive;
                mat->depthMode  = scene::DepthMode::ReadOnly;
                mat->cullMode   = scene::CullMode::None;
                return MakeCubeMesh(mat);
            };

            struct PointCfg { math::Vector3 pos; math::Vector3 diff; float scale; };
            PointCfg pts[] = {
                {{  4, 2,  4}, {4.0f, 3.0f, 2.0f}, 0.15f},
                {{ -4, 2,  4}, {2.0f, 3.0f, 4.0f}, 0.15f},
                {{  4, 2, -4}, {2.0f, 4.0f, 3.0f}, 0.15f},
                {{ -4, 2, -4}, {4.0f, 3.6f, 2.0f}, 0.15f},
            };
            for (auto& pt : pts)
            {
                scene::Entity e = sm.CreateEntity();
                auto* t = sm.AddComponent<scene::component::Transform>(e);
                t->position = pt.pos; t->scale = { pt.scale, pt.scale, pt.scale };
                sm.AddComponent<scene::component::MeshRenderer>(e)->mesh
                    = makeLightMarker(pt.pos, pt.diff);
                auto* l = sm.AddComponent<scene::component::Light>(e);
                l->type      = scene::component::Light::Type::Point;
                l->diffuse   = pt.diff;
                l->ambient   = pt.diff * 0.25f;
                l->linear    = 0.09f;
                l->quadratic = 0.032f;
            }
        }

        return true;
    };

    // -- onUpdate -------------------------------------------------------------

    cfg.onUpdate = [&]()
    {
        auto& sm = scene::Scene::instance();
        auto* ct = sm.GetComponent<scene::component::Transform>(s_camera);

        float dt  = framework::Clock::GetFrameDeltaSeconds();
        float spd = 5.0f * dt;
        auto& in  = core::Input::instance();

        // Free-fly: WASD + Q/E for vertical
        math::Vector3 fwd = ct->rotation * math::Vector3::k_Forward;
        math::Vector3 rht = ct->rotation * math::Vector3::k_Right;

        if (in.IsKeyDown(core::KeyCode::W)) ct->position += fwd * spd;
        if (in.IsKeyDown(core::KeyCode::S)) ct->position -= fwd * spd;
        if (in.IsKeyDown(core::KeyCode::A)) ct->position -= rht * spd;
        if (in.IsKeyDown(core::KeyCode::D)) ct->position += rht * spd;
        if (in.IsKeyDown(core::KeyCode::Q)) ct->position.y -= spd;
        if (in.IsKeyDown(core::KeyCode::E)) ct->position.y += spd;

        // Mouse look (left button)
        if (in.IsMouseButtonDown(core::MouseButton::Left))
        {
            float s = 0.002f;
            ct->rotation
                = math::Quaternion::FromEulerAngles(0, -in.GetMouseDeltaX() * s, 0)
                * ct->rotation
                * math::Quaternion::FromEulerAngles(-in.GetMouseDeltaY() * s, 0, 0);
            ct->rotation.Normalize();
        }

        // Toggle model visibility
        if (in.IsKeyPressed(core::KeyCode::T))
            s_modelVisible = !s_modelVisible;

        // Toggle fullscreen
        core::Window& w = core::Window::instance();
        if (in.IsKeyPressed(core::KeyCode::Z))
            w.SetFullscreen(false);

        // Cursor toggle (hold V)
        if (in.IsKeyDown(core::KeyCode::V))
            w.SetCursorVisible(false);
        if (in.IsKeyReleased(core::KeyCode::V))
            w.SetCursorVisible(true);

        // Rotate grid cubes
        {
            static float accum = 0;
            accum += dt * 0.3f;
            for (size_t i = 0; i < s_gridEntities.size(); ++i)
            {
                auto* tf = sm.GetComponent<scene::component::Transform>(s_gridEntities[i]);
                if (tf)
                    tf->rotation = math::Quaternion::FromEulerAngles(0, accum + i * 0.15f, 0);
            }
        }
    };

    // -- onRender -------------------------------------------------------------

    cfg.onRender = [&]()
    {
        auto& sm = scene::Scene::instance();
        if (s_modelRoot != scene::Entity::Invalid)
            sm.SetVisible(s_modelRoot, s_modelVisible);
        sm.Render(s_camera);
    };

    // -- onShutdown -----------------------------------------------------------

    cfg.onShutdown = [&]()
    {
        s_pbrShader.reset();
        s_blinnPhongShader.reset();
        s_simpleShader.reset();
        s_whiteTex.reset();
        s_flatNormalTex.reset();
    };

    return cfg;
}
