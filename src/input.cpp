#include "input.h"

using namespace HopEngine;
using namespace std;

static Input* application_instance = nullptr;


void Input::init(GLFWwindow* window)
{
	DBG_INFO("initialising input for window " + PTR(window));
	if (application_instance == nullptr)
		application_instance = new Input();
	application_instance->window = window;
	glfwSetKeyCallback(window, Input::keyCallback);
	glfwSetMouseButtonCallback(window, Input::mouseButtonCallback);
}

bool Input::isKeyDown(int key)
{
	return glfwGetKey(application_instance->window, key) == GLFW_PRESS;
}

bool Input::wasKeyPressed(int key)
{
	auto it = application_instance->pressed_since_checked.find(key);
	if (it == application_instance->pressed_since_checked.end())
		return false;
	application_instance->pressed_since_checked.erase(it);
	return true;
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

glm::vec2 Input::getMousePosition()
{
	double new_x, new_y;
	glfwGetCursorPos(application_instance->window, &new_x, &new_y);
	return glm::vec2{ (float)new_x, (float)new_y };
}

bool Input::isMouseDown(int button)
{
	return glfwGetMouseButton(application_instance->window, button) == GLFW_PRESS;
}

bool Input::wasMousePressed(int button)
{
	auto it = application_instance->pressed_since_checked.find(button);
	if (it == application_instance->pressed_since_checked.end())
		return false;
	application_instance->pressed_since_checked.erase(it);
	return true;
}

void Input::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
		application_instance->pressed_since_checked.insert(key);
}

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
		application_instance->pressed_since_checked.insert(button);
}



