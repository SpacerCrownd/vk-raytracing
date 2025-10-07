#include <iostream>

#include "Pathtracer/VulkanApp.h"

constexpr uint32_t WINDOW_WIDTH = 1240;
constexpr uint32_t WINDOW_HEIGHT = 720;
constexpr auto APP_NAME = "vk-raytracing";

int main() {
	auto app = PathTracingVk::VulkanApp(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME);

	try {
		app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}