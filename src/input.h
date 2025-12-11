#pragma once

#include <glm/vec2.hpp>
#include <set>

#include "common.h"
#include "window.h"

struct GLFWwindow;

namespace HopEngine
{

class Input
{
private:
	Ref<Window> window;
	std::set<int> pressed_since_checked;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Input);

	static void init(Ref<Window> window);
	static void destroy();

	static bool isKeyDown(int key);
	static bool wasKeyPressed(int key);
	static float getAxis(int key_negative, int key_positive);
	static glm::vec2 getMouseDelta();
	static glm::vec2 getMousePosition();
	static bool isMouseDown(int button);
	static bool wasMousePressed(int button);

private:
	Input(Ref<Window> window);
	~Input();

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};

}
