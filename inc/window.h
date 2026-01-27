#pragma once

#include <string>

#include "common.h"

struct GLFWwindow;

namespace HopEngine
{

class Window : public Destructible
{
private:
	GLFWwindow* window;
	int width;
	int height;
	
public:
	DELETE_CONSTRUCTORS(Window);
	Window(uint32_t _width, uint32_t _height, const std::string& title);
	~Window() override;

	static void terminateEnvironment();
	static void pollEvents();

	GLFWwindow* getWindow() const { return window; }
	std::pair<uint32_t, uint32_t> getSize();
	bool getShouldClose() const;
	bool isMinified() const;
	bool isResized();
	void setTitle(const std::string& title) const;
	void setVisible(bool visible) const;
	void setIcon(const std::string& path) const;
};

}
