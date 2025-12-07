#include "input.h"

using namespace HopEngine;
using namespace std;

static Input* application_instance = nullptr;

void Input::init(GLFWwindow* window)
{
	if (application_instance == nullptr)
	{
		application_instance = new Input();
		application_instance->window = window;
	}
}

bool Input::isKeyDown(int key)
{
	return glfwGetKey(application_instance->window, key) == GLFW_PRESS;
}

float Input::getAxis(int key_negative, int key_positive)
{
	float value = 0;
	if (Input::isKeyDown(key_negative))
		value -= 1.0f;
	if (Input::isKeyDown(key_positive))
		value += 1.0f;

	return value;
}

glm::vec2 Input::getMouseDelta()
{
	static double last_x = 0, last_y = 0;
	double new_x, new_y;
	glfwGetCursorPos(application_instance->window, &new_x, &new_y);
	glm::vec2 difference = { new_x - last_x, new_y - last_y };
	last_x = new_x;
	last_y = new_y;
	return difference;
}

bool Input::isMouseDown(int button)
{
	return glfwGetMouseButton(application_instance->window, button) == GLFW_PRESS;
}



