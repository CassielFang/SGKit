#include <sgkit/sgkit.h>
#include "objects.h"
using namespace sgkit;

static scene::Entity s_cubes[10];
static scene::Entity s_camera;
static scene::Entity s_light;
static std::shared_ptr<graphics::Shader> s_lightShader;
static std::shared_ptr<graphics::Texture> s_diffTex;
static std::shared_ptr<graphics::Texture> s_specTex;

 const math::Vector3 s_cubePositions[10] =
{
    {0.0f,  0.0f,  0.0f},
    {2.0f,  5.0f, -15.0f},
    {-1.5f, -2.2f, -2.5f},
    {-3.8f, -2.0f, -12.3f},
    {2.4f, -0.4f, -3.5f},
    {-1.7f,  3.0f, -7.5f},
    {1.3f, -2.0f, -2.5f},
    {1.5f,  2.0f, -2.5f},
    {1.5f,  0.2f, -1.5f},
    {-1.3f,  1.0f, -1.5f}
};

ApplicationConfig sgkit::CreateApplication()
{
    ApplicationConfig cfg{};
    cfg.title = "SGKit Light";
    cfg.width = 1280;
    cfg.height = 720;
    cfg.fullscreenBolderless = true;

    cfg.onInit = [&]() -> bool
        {
            s_lightShader = std::make_shared<graphics::Shader>();
            s_lightShader->LoadFromFile("assets/simple.vert", "assets/simple.frag");
            s_diffTex = std::make_shared<graphics::Texture>(0);
            s_diffTex->LoadFromFile("assets/container2.png");
            s_specTex = std::make_shared<graphics::Texture>(1);
            s_specTex->LoadFromFile("assets/container2_specular.png");

            for (int i = 0; i < 10; ++i)
            {
                scene::component::Transform transform;
                transform.position = s_cubePositions[i];
                transform.rotation = math::Quaternion::FromAxisAngle({ 1.0f, 0.3f, 0.5f }, math::ToRadians(20.f * i));
                s_cubes[i] = createCube(transform, s_lightShader, s_diffTex, s_specTex);
            }
            s_camera = createCamera();
            s_light = createLight();
            return true;
        };
    cfg.onUpdate = [&]()
        {
            auto cameraTransform = scene::Scene::instance().GetComponent<scene::component::Transform>(s_camera);
            math::Vector3 forward = cameraTransform->rotation * math::Vector3::k_Forward;
            math::Vector3 right = cameraTransform->rotation * math::Vector3::k_Right;
            float speed = 5.0f * framework::Clock::GetFrameDeltaSeconds();
            auto& in = core::Input::instance();
            if (in.IsKeyDown(core::KeyCode::W)) cameraTransform->position += forward * speed;
            if (in.IsKeyDown(core::KeyCode::S)) cameraTransform->position -= forward * speed;
            if (in.IsKeyDown(core::KeyCode::A)) cameraTransform->position -= right * speed;
            if (in.IsKeyDown(core::KeyCode::D)) cameraTransform->position += right * speed;
            if (in.IsKeyDown(core::KeyCode::Q)) cameraTransform->position.y -= speed;
            if (in.IsKeyDown(core::KeyCode::E)) cameraTransform->position.y += speed;
            if (in.IsMouseButtonDown(core::MouseButton::Left))
            {
                float s = 0.002f;
                float yaw = -in.GetMouseDeltaX() * s;
                float pitch = -in.GetMouseDeltaY() * s;
                cameraTransform->rotation
                    = math::Quaternion::FromEulerAngles(0, yaw, 0)
                    * cameraTransform->rotation
                    * math::Quaternion::FromEulerAngles(pitch, 0, 0);
                cameraTransform->rotation.Normalize();
            }
            
            core::Window& window = core::Window::instance();
            if (in.IsKeyPressed(core::KeyCode::Space)) window.SetFullscreen(true);
            if (in.IsKeyPressed(core::KeyCode::Z)) window.SetFullscreen(false);
            if (in.IsKeyDown(core::KeyCode::V)) window.SetCursorVisible(false);
            if (in.IsKeyReleased(core::KeyCode::V)) window.SetCursorVisible(true);
            
            if (!window.isActive())
            {
                core::DebugOut("inActive!");
                if (window.IsCloseRequest())
                {
                    window.RequestClose(false);
                    core::DebugOut("Denied close.");
                }
            }
        };
    cfg.onRender = [&]()
        {
            scene::Scene::instance().Render(s_camera);
        };
    cfg.onShutdown = [&]()
        {
            s_lightShader.reset();
        };
    return cfg;
}
