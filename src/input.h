#pragma once

#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

#include "common.h"

namespace HopEngine
{

class Input
{
private:
	GLFWwindow* window = nullptr;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Input);

	static void init(GLFWwindow* window);
	static bool isKeyDown(int key);
	static float getAxis(int key_negative, int key_positive);
	static glm::vec2 getMouseDelta();
	static bool isMouseDown(int button);

private:
	Input() { };
};

}
