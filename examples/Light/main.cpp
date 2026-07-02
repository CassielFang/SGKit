#include <sgkit/sgkit.h>

using namespace sgkit;

auto shader = std::make_shared<graphics::Shader>();
auto lightShader = std::make_shared<graphics::Shader>();
auto cubeMesh = std::make_shared<graphics::Mesh>();
auto lightMesh = std::make_shared<graphics::Mesh>();

scene::Entity flagEntity;
scene::Entity cubeEntity;
scene::Entity lightEntity;
scene::Entity cameraEntity;

ApplicationConfig sgkit::CreateApplication()
{
	ApplicationConfig cfg{};
	cfg.title = "SGKit Light";
	cfg.width = 1280;
	cfg.height = 720;
	cfg.fullscreen = true;
	cfg.fullscreenBolderless = true;

	cfg.onInit = []() -> bool
		{
			scene::Scene& manager = scene::Scene::instance();

			shader->LoadFromFile("assets/simple.vert", "assets/simple.frag");
			lightShader->LoadFromFile("assets/light.vert", "assets/light.frag");

			float objVertices[] = {
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
			uint32_t objIndices[] = {
				 0, 1, 2, 2, 3, 0,    4, 5, 6, 6, 7, 4,
				 8, 9,10,10,11, 8,   12,13,14,14,15,12,
				16,17,18,18,19,16,   20,21,22,22,23,20,
			};
			graphics::VertexLayout layout2;
			layout2.PushFloat(0, 3).PushFloat(1, 3).PushFloat(2, 2);
			auto vb2 = std::make_shared<graphics::VertexBuffer>();
			vb2->Create(objVertices, sizeof(objVertices));
			auto ib2 = std::make_shared<graphics::IndexBuffer>();
			ib2->Create(objIndices, sizeof(objIndices) / sizeof(uint32_t));
			auto va2 = std::make_shared<graphics::VertexArray>();
			va2->Create();
			va2->AddVertexBuffer(vb2, layout2);
			va2->SetIndexBuffer(ib2);

			auto texture2 = std::make_shared<graphics::Texture>();
			texture2->LoadFromFile("assets/container2.bmp");

			auto material2 = std::make_shared<graphics::Material>();
			material2->shader = shader;
			material2->diffuse = texture2;

			cubeMesh->vertexArray = va2;
			cubeMesh->material = material2;

			cubeEntity = manager.CreateEntity();
			auto transform = manager.AddComponent<scene::Transform>(cubeEntity);
			transform->position = { 0.0f, 0.0f, 0.0f };
			auto meshRenderer = manager.AddComponent<scene::MeshRenderer>(cubeEntity);
			meshRenderer->mesh = cubeMesh;

			auto material3 = std::make_shared<graphics::Material>();
			material3->shader = lightShader;

			lightMesh->vertexArray = va2;
			lightMesh->material = material3;

			lightEntity = manager.CreateEntity();
			transform = manager.AddComponent<scene::Transform>(lightEntity);
			transform->position = { 0.8f, 1.f, 1.2f };
			transform->scale = { 0.3f, 0.3f, 0.3f };
			manager.AddComponent<scene::Light>(lightEntity);
			meshRenderer = manager.AddComponent<scene::MeshRenderer>(lightEntity);
			meshRenderer->mesh = lightMesh;

			cameraEntity = manager.CreateEntity();
			manager.AddComponent<scene::Camera>(cameraEntity);
			auto cameraTrans = manager.AddComponent<scene::Transform>(cameraEntity);
			cameraTrans->position = { 0.0f, 0.0f, 5.0f };

			std::printf("Light: already init\n");

			return true;
		};
	cfg.onUpdate = [&]()
		{
			//math::Vector3 lightColor;
			//lightColor.x = static_cast<float>(sin(clk.GetElapsedSeconds() * 2.0f));
			//lightColor.y = static_cast<float>(sin(clk.GetElapsedSeconds() * 0.7f));
			//lightColor.z = static_cast<float>(sin(clk.GetElapsedSeconds() * 1.3f));
			//scene::Light* light = manager.GetComponent<scene::Light>(lightEntity);
			//light->ambient = lightColor * 0.2f;
			//light->diffuse = lightColor * 0.5f;

			auto ct = scene::Scene::instance().GetComponent<scene::Transform>(cameraEntity);

			math::Vector3 forward = ct->rotation * math::Vector3{ 0.0f, 0.0f, -1.0f };
			math::Vector3 right = ct->rotation * math::Vector3{ 1.0f, 0.0f,  0.0f };
			float speed = 5.0f * framework::Clock::GetFrameDeltaSeconds();
			auto& in = core::Input::instance();
			if (in.IsKeyDown(core::KeyCode::W)) ct->position += forward * speed;
			if (in.IsKeyDown(core::KeyCode::S)) ct->position -= forward * speed;
			if (in.IsKeyDown(core::KeyCode::A)) ct->position -= right * speed;
			if (in.IsKeyDown(core::KeyCode::D)) ct->position += right * speed;
			if (in.IsKeyDown(core::KeyCode::Q)) ct->position.y -= speed;
			if (in.IsKeyDown(core::KeyCode::E)) ct->position.y += speed;
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

			core::Window& window = core::Window::instance();
			if (in.IsKeyPressed(core::KeyCode::Space)) window.SetFullscreen(true);
			if (in.IsKeyPressed(core::KeyCode::Z)) window.SetFullscreen(false);
			if (in.IsKeyPressed(core::KeyCode::Q)) window.RequestClose();
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
	cfg.onRender = []()
		{
			scene::Scene::instance().OnRender(cameraEntity);
		};
	cfg.onShutdown = []()
		{
			shader.reset();
			lightShader.reset();
			cubeMesh.reset();
			lightMesh.reset();
		};
	return cfg;
}
