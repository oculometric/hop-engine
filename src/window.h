#pragma once

#include <string>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "common.h"

namespace HopEngine
{

class Window
{
private:
	GLFWwindow* window;
	int width;
	int height;
	
public:
	DELETE_CONSTRUCTORS(Window);

	Window(uint32_t width, uint32_t height, std::string title);

	static void initEnvironment();
	static void terminateEnvironment();

	void pollEvents() const;
	bool getShouldClose() const;
	inline GLFWwindow* getWindow() const { return window; }
	bool isMinified() const;
	bool isResized();
	std::pair<uint32_t, uint32_t> getSize();
	void setTitle(std::string title);
	void setVisible(bool visible);

	~Window();
};

}
