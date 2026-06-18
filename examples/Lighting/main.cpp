#include <sgkit/sgkit.h>

namespace sgkit {

	ApplicationConfig CreateApplication()
	{
		sgkit::ApplicationConfig cfg{};
		cfg.title = "SGKit Lighting";
		cfg.width = 1280;
		cfg.height = 720;

		cfg.onInit = []() -> bool
			{

				return true;
			};

		cfg.onUpdate = [](float dt)
			{

			};

		cfg.onRender = []()
			{

			};

		cfg.onShutdown = []()
			{

			};
		return cfg;
	}

}
