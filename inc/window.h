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
	void setIcon(std::string path);

	Window(uint32_t width, uint32_t height, std::string title);
	~Window() override;
};

}
