#include "input.h"

#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "render_server.h"

using namespace HopEngine;
using namespace std;

static Input* instance = nullptr;

void Input::init()
{
	if (instance == nullptr)
		instance = new Input();
}

void Input::destroy()
{
	DBG_INFO("destroying input");
	if (instance != nullptr)
	{
		delete instance;
		instance = nullptr;
	}
}

bool Input::isKeyDown(const int key)
{
	if (ImGui::GetIO().WantTextInput)
		return false;
	return glfwGetKey(instance->window, key) == GLFW_PRESS;
}

bool Input::wasKeyPressed(const int key)
{
	const auto it = instance->pressed_since_checked.find(key);
	if (it == instance->pressed_since_checked.end())
		return false;
	instance->pressed_since_checked.erase(it);
	return true;
}

float Input::getAxis(const int key_negative, const int key_positive)
{
	float value = 0;
	if (Input::isKeyDown(key_negative))
		value -= 1.0f;
	if (Input::isKeyDown(key_positive))
		value += 1.0f;

	return value;
}

static double last_x = 0, last_y = 0;
bool Input::isMouseDown(const MouseButton button)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return false;
	return glfwGetMouseButton(instance->window, button) == GLFW_PRESS;
}

bool Input::wasMousePressed(const MouseButton button)
{
	const auto it = instance->pressed_since_checked_mouse.find(button);
	if (it == instance->pressed_since_checked_mouse.end())
		return false;
	instance->pressed_since_checked_mouse.erase(it);
	return true;
}

glm::vec2 Input::getMouseDelta()
{
	return instance->mouse_delta;
}

glm::vec2 Input::getMousePosition()
{
	return instance->mouse_position;
}

void Input::lockMouseToRectangle(glm::vec2 min, glm::vec2 max)
{
    instance->lock_mouse = true;
    instance->mouse_lock_min = min;
    instance->mouse_lock_max = max;
}

void Input::unlockMouse()
{
    instance->lock_mouse = false;
}

void Input::pollInput()
{
    glfwPollEvents();

	double new_mouse_x;
	double new_mouse_y;
	glfwGetCursorPos(instance->window, &new_mouse_x, &new_mouse_y);

	glm::vec2 new_mouse = { static_cast<float>(new_mouse_x), static_cast<float>(new_mouse_y) };
	instance->mouse_delta = new_mouse - instance->mouse_position;
	instance->mouse_position = new_mouse;

    if (instance->lock_mouse)
    {
        instance->mouse_position = glm::clamp(instance->mouse_position, instance->mouse_lock_min, instance->mouse_lock_max);
        glfwSetCursorPos(instance->window, static_cast<double>(instance->mouse_position.x), static_cast<double>(instance->mouse_position.y));
    }

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
			compacted_state.axes[GAMEPAD_BX] = static_cast<float>(compacted_state.buttons[GAMEPAD_RIGHT]) - static_cast<float>(compacted_state.buttons[GAMEPAD_LEFT]);
			compacted_state.axes[GAMEPAD_BY] = static_cast<float>(compacted_state.buttons[GAMEPAD_UP]) - static_cast<float>(compacted_state.buttons[GAMEPAD_DOWN]);
			compacted_state.axes[GAMEPAD_BUTTONS] = static_cast<float>(compacted_state.buttons[GAMEPAD_RBUTTON]) - static_cast<float>(compacted_state.buttons[GAMEPAD_LBUTTON]);
			instance->gamepad_states[i] = compacted_state;
		}
		else
			instance->gamepad_states.erase(i);
	}
}

bool Input::isGamepadButtonDown(const GamepadButton button, const int controller)
{
	return instance->gamepad_states[controller].buttons[button];
}

float Input::getGamepadAxis(const GamepadAxis axis, const int controller)
{
	return instance->gamepad_states[controller].axes[axis];
}

void Input::setCursorVisible(const bool visible)
{
	glfwSetInputMode(instance->window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Input::applyCallbackBindings()
{
	instance->window = RenderServer::getWindow();
	glfwSetKeyCallback(instance->window, Input::keyCallback);
	glfwSetMouseButtonCallback(instance->window, Input::mouseButtonCallback);
	ImGui_ImplGlfw_InstallCallbacks(instance->window);
}

void Input::keyCallback(GLFWwindow* window, const int key, const int scancode, const int action, const int mods)
{
	if (ImGui::GetIO().WantTextInput)
		return;
	if (action == GLFW_PRESS || action == GLFW_REPEAT)
		instance->pressed_since_checked.insert(key);
}

void Input::mouseButtonCallback(GLFWwindow* window, const int button, const int action, const int mods)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;
	if (action == GLFW_PRESS)
		instance->pressed_since_checked_mouse.insert(static_cast<MouseButton>(button));
}

Input::Input()
{
	instance = this;
	applyCallbackBindings();
}
