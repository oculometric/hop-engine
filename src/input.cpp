#include "input.h"

#include <imgui.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace HopEngine;
using namespace std;

static Input* application_instance = nullptr;

void Input::init(Ref<Window> window)
{
	DBG_INFO("initialising input for window " + PTR(window.get()));
	if (application_instance == nullptr)
		application_instance = new Input(window);
}

void Input::destroy()
{
	DBG_INFO("destroying input");
	if (application_instance != nullptr)
	{
		delete application_instance;
		application_instance = nullptr;
	}
}

bool Input::isKeyDown(int key)
{
	if (ImGui::GetIO().WantTextInput)
		return false;
	return glfwGetKey(application_instance->window->getWindow(), key) == GLFW_PRESS;
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

static double last_x = 0, last_y = 0;
glm::vec2 Input::getMouseDelta()
{
	double new_x, new_y;
	glfwGetCursorPos(application_instance->window->getWindow(), &new_x, &new_y);
	glm::vec2 difference = { new_x - last_x, new_y - last_y };
	return difference;
}

void Input::resetMouseDelta()
{
	glfwGetCursorPos(application_instance->window->getWindow(), &last_x, &last_y);
}

glm::vec2 Input::getMousePosition()
{
	double new_x, new_y;
	glfwGetCursorPos(application_instance->window->getWindow(), &new_x, &new_y);
	return glm::vec2{ (float)new_x, (float)new_y };
}

bool Input::isMouseDown(MouseButton button)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return false;
	return glfwGetMouseButton(application_instance->window->getWindow(), button) == GLFW_PRESS;
}

bool Input::wasMousePressed(MouseButton button)
{
	auto it = application_instance->pressed_since_checked_mouse.find(button);
	if (it == application_instance->pressed_since_checked_mouse.end())
		return false;
	application_instance->pressed_since_checked_mouse.erase(it);
	return true;
}

void Input::setCursorVisible(bool visible)
{
	glfwSetInputMode(application_instance->window->getWindow(), GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Input::pollGamepads()
{
	for (int i = 0; i <= GLFW_JOYSTICK_LAST; ++i)
	{
		GLFWgamepadstate state;
		if (glfwGetGamepadState(i, &state))
		{
			GamepadState compacted_state;
			compacted_state.buttons[GAMEPAD_RTRIGGER] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.0f;
			compacted_state.buttons[GAMEPAD_LTRIGGER] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.0f;
			compacted_state.buttons[GAMEPAD_A] = state.buttons[GLFW_GAMEPAD_BUTTON_A];
			compacted_state.buttons[GAMEPAD_B] = state.buttons[GLFW_GAMEPAD_BUTTON_B];
			compacted_state.buttons[GAMEPAD_X] = state.buttons[GLFW_GAMEPAD_BUTTON_X];
			compacted_state.buttons[GAMEPAD_Y] = state.buttons[GLFW_GAMEPAD_BUTTON_Y];
			compacted_state.buttons[GAMEPAD_UP] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
			compacted_state.buttons[GAMEPAD_DOWN] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
			compacted_state.buttons[GAMEPAD_LEFT] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
			compacted_state.buttons[GAMEPAD_RIGHT] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
			compacted_state.buttons[GAMEPAD_RBUTTON] = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
			compacted_state.buttons[GAMEPAD_LBUTTON] = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
			compacted_state.buttons[GAMEPAD_RSTICK] = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
			compacted_state.buttons[GAMEPAD_LSTICK] = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
			compacted_state.axes[GAMEPAD_RTRIGGER] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
			compacted_state.axes[GAMEPAD_LTRIGGER] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
			compacted_state.axes[GAMEPAD_RX] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
			compacted_state.axes[GAMEPAD_RY] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
			compacted_state.axes[GAMEPAD_LX] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
			compacted_state.axes[GAMEPAD_LY] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
			compacted_state.axes[GAMEPAD_BX] = (float)(compacted_state.buttons[GAMEPAD_RIGHT]) - (float)(compacted_state.buttons[GAMEPAD_LEFT]);
			compacted_state.axes[GAMEPAD_BY] = (float)(compacted_state.buttons[GAMEPAD_UP]) - (float)(compacted_state.buttons[GAMEPAD_DOWN]);
			compacted_state.axes[GAMEPAD_BUTTONS] = (float)(compacted_state.buttons[GAMEPAD_RBUTTON]) - (float)(compacted_state.buttons[GAMEPAD_LBUTTON]);
			application_instance->gamepad_states[i] = compacted_state;
		}
		else
			application_instance->gamepad_states.erase(i);
	}
}

bool Input::isGamepadButtonDown(GamepadButton button, int controller)
{
	return application_instance->gamepad_states[controller].buttons[button];
}

float Input::getGamepadAxis(GamepadAxis axis, int controller)
{
	return application_instance->gamepad_states[controller].axes[axis];
}

void Input::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (ImGui::GetIO().WantTextInput)
		return;
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
		application_instance->pressed_since_checked.insert(key);
}

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;
	if (action == GLFW_PRESS)
		application_instance->pressed_since_checked_mouse.insert((MouseButton)button);
}

Input::Input(Ref<Window> _window)
{
	window = _window;
	glfwSetKeyCallback(window->getWindow(), Input::keyCallback);
	glfwSetMouseButtonCallback(window->getWindow(), Input::mouseButtonCallback);
}

Input::~Input()
{
	window = nullptr;
}
