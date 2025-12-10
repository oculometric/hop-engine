#pragma once

#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include <set>

#include "common.h"

namespace HopEngine
{

class Input
{
private:
	GLFWwindow* window = nullptr;
	std::set<int> pressed_since_checked;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Input);

	static void init(GLFWwindow* window);
	static bool isKeyDown(int key);
	static bool wasKeyPressed(int key);
	static float getAxis(int key_negative, int key_positive);
	static glm::vec2 getMouseDelta();
	static glm::vec2 getMousePosition();
	static bool isMouseDown(int button);
	static bool wasMousePressed(int button);

private:
	Input() { };

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};

}
