#pragma once

#include <string>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace HopEngine
{

class Window
{
private:
	GLFWwindow* window;
	
public:
	Window() = delete;
	Window(uint32_t width, uint32_t height, std::string title);

	static void initEnvironment();
	static void terminateEnvironment();

	void pollEvents();
	bool getShouldClose();
	inline GLFWwindow* getWindow() { return window; }
	std::pair<uint32_t, uint32_t> getSize();

	~Window();
};

}
