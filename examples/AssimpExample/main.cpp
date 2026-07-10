#include <sgkit/sgkit.h>

using namespace sgkit;

// -- Global state

static scene::Entity                       s_camera;
static scene::Entity                       s_modelRoot;
static std::shared_ptr<graphics::Shader>   s_shader;
static std::shared_ptr<graphics::Shader>   s_simpleShader;
static std::vector<scene::Entity>          s_meshEntities;
static bool                                s_modelVisible = true;

// -- CreateApplication

ApplicationConfig sgkit::CreateApplication()
{
    ApplicationConfig cfg{};
    cfg.title  = "SGKit Assimp Example";
    cfg.width  = 1280;
    cfg.height = 720;
    cfg.fullscreen = true;
    cfg.fullscreenBolderless = true;

    cfg.onInit = [&]() -> bool
    {
        // 1. Load Phong shader
        s_shader = std::make_shared<graphics::Shader>();
        if (!s_shader->LoadFromFile("assets/shaders/light.vert",
                                    "assets/shaders/light.frag"))
            return false;

        // 2. Load 3D model - one call, returns root + mesh list
        auto model = scene::Model::Load("assets/backpack/backpack.obj", s_shader);
        // auto model = scene::Model::Load("assets/cute cartoon girl.glb", s_shader);
        if (model.root == scene::Entity::Invalid)
            return false;

        s_modelRoot    = model.root;
        s_meshEntities = model.entities;

        // Set initial transform on the root
        auto* rootTf = scene::Scene::instance()
            .GetComponent<scene::component::Transform>(s_modelRoot);
        rootTf->position = { 0.0f, 0.0f, 0.0f };
        rootTf->scale    = { 1.5f, 1.5f, 1.5f };

        // 3. Create camera
        s_camera = scene::Scene::instance().CreateEntity();
        auto* ct = scene::Scene::instance()
            .AddComponent<scene::component::Transform>(s_camera);
        ct->position = { 0.0f, 0.5f, 6.0f };
        scene::Scene::instance().AddComponent<scene::component::Camera>(s_camera);

        // 4. Light-marker shader
        s_simpleShader = std::make_shared<graphics::Shader>();
        s_simpleShader->LoadFromFile("assets/shaders/simple.vert",
                                     "assets/shaders/simple.frag");

        // 5. Lights
        {
            constexpr float cubeVerts[] = {
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
            constexpr uint32_t cubeIdx[] = {
                0,1,2, 2,3,0,  4,5,6, 6,7,4,  8,9,10, 10,11,8,
                12,13,14, 14,15,12,  16,17,18, 18,19,16,  20,21,22, 22,23,20,
            };

            auto makeMarker = [&]() -> std::shared_ptr<scene::Mesh>
            {
                graphics::VertexLayout lo;
                lo.PushFloat(0,3).PushFloat(1,3).PushFloat(2,2);
                auto vb = std::make_shared<graphics::VertexBuffer>();
                vb->Create(cubeVerts, sizeof(cubeVerts));
                auto ib = std::make_shared<graphics::IndexBuffer>();
                ib->Create(cubeIdx, sizeof(cubeIdx)/sizeof(uint32_t));
                auto va = std::make_shared<graphics::VertexArray>();
                va->Create(); va->SetVertexBuffer(vb, lo); va->SetIndexBuffer(ib);
                auto m = std::make_shared<scene::Material>();
                m->shader = s_simpleShader;
                m->blendMode = scene::BlendMode::Additive;
                m->depthMode = scene::DepthMode::ReadOnly;
                m->cullMode  = scene::CullMode::None;
                auto mesh = std::make_shared<scene::Mesh>();
                mesh->vertexArray = va; mesh->material = m;
                return mesh;
            };

            auto& sm = scene::Scene::instance();

            // SpotLight - tight red beam
            {
                sgkit::scene::Entity e = sm.CreateEntity();
                auto* t = sm.AddComponent<scene::component::Transform>(e);
                t->position = { 0, 2, 3 }; t->scale = { 0.3f, 0.3f, 0.3f };
                sm.AddComponent<scene::component::MeshRenderer>(e)->mesh = makeMarker();
                auto* l = sm.AddComponent<scene::component::Light>(e);
                l->type = scene::component::Light::Type::SpotLight;
                l->direction = { 0, -2, -3 };
                l->cutOff = 0.985f; l->outerCutOff = 0.95f;
                l->diffuse = { 3, 3, 3 };
                l->linear = 0.027f; l->quadratic = 0.008f;
            }

            // Point - soft white fill
            {
                sgkit::scene::Entity e = sm.CreateEntity();
                auto* t = sm.AddComponent<scene::component::Transform>(e);
                t->position = { -4, 2, 3 }; t->scale = { 0.12f, 0.12f, 0.12f };
                sm.AddComponent<scene::component::MeshRenderer>(e)->mesh = makeMarker();
                auto* l = sm.AddComponent<scene::component::Light>(e);
                l->type = scene::component::Light::Type::Point;
                l->ambient = { 0.3f, 0.3f, 0.3f };
                l->diffuse = { 2, 2, 2 };
                l->linear = 0.07f; l->quadratic = 0.018f;
            }
        }

        return true;
    };

    cfg.onUpdate = [&]()
    {
        auto& sm = scene::Scene::instance();
        auto* ct = sm.GetComponent<scene::component::Transform>(s_camera);

        math::Vector3 fwd = ct->rotation * math::Vector3::k_Forward;
        math::Vector3 rht = ct->rotation * math::Vector3::k_Right;
        float spd = 5.0f * framework::Clock::GetFrameDeltaSeconds();

        auto& in = core::Input::instance();
        if (in.IsKeyDown(core::KeyCode::W)) ct->position += fwd * spd;
        if (in.IsKeyDown(core::KeyCode::S)) ct->position -= fwd * spd;
        if (in.IsKeyDown(core::KeyCode::A)) ct->position -= rht * spd;
        if (in.IsKeyDown(core::KeyCode::D)) ct->position += rht * spd;
        if (in.IsKeyDown(core::KeyCode::Q)) ct->position.y -= spd;
        if (in.IsKeyDown(core::KeyCode::E)) ct->position.y += spd;

        if (in.IsMouseButtonDown(core::MouseButton::Left))
        {
            float s = 0.002f;
            ct->rotation
                = math::Quaternion::FromEulerAngles(0, -in.GetMouseDeltaX()*s, 0)
                * ct->rotation
                * math::Quaternion::FromEulerAngles(-in.GetMouseDeltaY()*s, 0, 0);
            ct->rotation.Normalize();
        }

        core::Window& w = core::Window::instance();
        if (in.IsKeyPressed(core::KeyCode::T))     s_modelVisible = !s_modelVisible;
        if (in.IsKeyPressed(core::KeyCode::Z))     w.SetFullscreen(false);
        if (in.IsKeyDown(core::KeyCode::V))        w.SetCursorVisible(false);
        if (in.IsKeyReleased(core::KeyCode::V))    w.SetCursorVisible(true);
    };

    cfg.onRender = [&]()
    {
        auto& sm = scene::Scene::instance();
        sm.SetVisible(s_modelRoot, s_modelVisible);
        sm.Render(s_camera);
    };

    cfg.onShutdown = [&]()
    {
        s_shader.reset();
        s_simpleShader.reset();
    };

    return cfg;
}
