#include "input.h"

#include <glm/glm.hpp>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/imgui.h>
#define GLFW_INCLUDE_VULKAN
#include "graphics_server.h"
#include "window.h"

#include <GLFW/glfw3.h>

using namespace HopEngine;

bool Input::isKeyDown(const int key)
{
    if (ImGui::GetIO().WantTextInput) return false;
    return glfwGetKey(static_cast<GLFWwindow*>(getInstance()->window), key) == GLFW_PRESS;
}

bool Input::wasKeyPressed(const int key)
{
    const auto it = getInstance()->pressed_since_checked.find(key);
    if (it == getInstance()->pressed_since_checked.end()) return false;
    getInstance()->pressed_since_checked.erase(it);
    return true;
}

float Input::getAxis(const int key_negative, const int key_positive)
{
    float value = 0;
    if (Input::isKeyDown(key_negative)) value -= 1.0f;
    if (Input::isKeyDown(key_positive)) value += 1.0f;

    return value;
}

static double last_x = 0, last_y = 0;
bool Input::isMouseDown(const MouseButton button)
{
    if (ImGui::GetIO().WantCaptureMouse) return false;
    return glfwGetMouseButton(static_cast<GLFWwindow*>(getInstance()->window), button) == GLFW_PRESS;
}

bool Input::wasMousePressed(const MouseButton button)
{
    const auto it = getInstance()->pressed_since_checked_mouse.find(button);
    if (it == getInstance()->pressed_since_checked_mouse.end()) return false;
    getInstance()->pressed_since_checked_mouse.erase(it);
    return true;
}

glm::vec2 Input::getMouseDelta() { return getInstance()->mouse_delta; }

glm::vec2 Input::getMousePosition() { return getInstance()->mouse_position; }

void Input::lockMouseToRectangle(glm::vec2 min, glm::vec2 max)
{
    getInstance()->lock_mouse     = true;
    getInstance()->mouse_lock_min = min;
    getInstance()->mouse_lock_max = max;
}

void Input::unlockMouse() { getInstance()->lock_mouse = false; }

void Input::pollInput()
{
    glfwPollEvents();

    double new_mouse_x;
    double new_mouse_y;
    glfwGetCursorPos(static_cast<GLFWwindow*>(getInstance()->window), &new_mouse_x, &new_mouse_y);

    glm::vec2 new_mouse           = { static_cast<float>(new_mouse_x), static_cast<float>(new_mouse_y) };
    getInstance()->mouse_delta    = new_mouse - getInstance()->mouse_position;
    getInstance()->mouse_position = new_mouse;

    if (getInstance()->lock_mouse)
    {
        getInstance()->mouse_position = glm::clamp(getInstance()->mouse_position,
            getInstance()->mouse_lock_min, getInstance()->mouse_lock_max);
        glfwSetCursorPos(static_cast<GLFWwindow*>(getInstance()->window),
            static_cast<double>(getInstance()->mouse_position.x),
            static_cast<double>(getInstance()->mouse_position.y));
    }

    for (int i = 0; i <= GLFW_JOYSTICK_LAST; ++i)
    {
        GLFWgamepadstate state;
        if (glfwGetGamepadState(i, &state))
        {
            GamepadState compacted_state;
            compacted_state.buttons[GAMEPAD_RT] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.0f;
            compacted_state.buttons[GAMEPAD_LT] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] > 0.0f;
            compacted_state.buttons[GAMEPAD_A]  = state.buttons[GLFW_GAMEPAD_BUTTON_A];
            compacted_state.buttons[GAMEPAD_B]  = state.buttons[GLFW_GAMEPAD_BUTTON_B];
            compacted_state.buttons[GAMEPAD_X]  = state.buttons[GLFW_GAMEPAD_BUTTON_X];
            compacted_state.buttons[GAMEPAD_Y]  = state.buttons[GLFW_GAMEPAD_BUTTON_Y];
            compacted_state.buttons[GAMEPAD_DU] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP];
            compacted_state.buttons[GAMEPAD_DD] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN];
            compacted_state.buttons[GAMEPAD_DL] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT];
            compacted_state.buttons[GAMEPAD_DR] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT];
            compacted_state.buttons[GAMEPAD_RB] = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER];
            compacted_state.buttons[GAMEPAD_LB] = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER];
            compacted_state.buttons[GAMEPAD_RS] = state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB];
            compacted_state.buttons[GAMEPAD_LS] = state.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB];
            compacted_state.axes[GAMEPAD_RT]    = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
            compacted_state.axes[GAMEPAD_LT]    = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
            compacted_state.axes[GAMEPAD_RX]    = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
            compacted_state.axes[GAMEPAD_RY]    = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
            compacted_state.axes[GAMEPAD_LX]    = state.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
            compacted_state.axes[GAMEPAD_LY]    = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
            compacted_state.axes[GAMEPAD_DX]    = static_cast<float>(compacted_state.buttons[GAMEPAD_DR]) -
                                                  static_cast<float>(compacted_state.buttons[GAMEPAD_DL]);
            compacted_state.axes[GAMEPAD_DY]    = static_cast<float>(compacted_state.buttons[GAMEPAD_DU]) -
                                                  static_cast<float>(compacted_state.buttons[GAMEPAD_DD]);
            compacted_state.axes[GAMEPAD_BUMPERS] =
                static_cast<float>(compacted_state.buttons[GAMEPAD_RB]) -
                static_cast<float>(compacted_state.buttons[GAMEPAD_LB]);
            compacted_state.axes[GAMEPAD_TRIGGERS] =
                static_cast<float>(compacted_state.buttons[GAMEPAD_RT]) -
                static_cast<float>(compacted_state.buttons[GAMEPAD_LT]);
            getInstance()->gamepad_states[i] = compacted_state;
        }
        else
            getInstance()->gamepad_states.erase(i);
    }
}

bool Input::isGamepadButtonDown(const GamepadButton button, const int controller)
{ return getInstance()->gamepad_states[controller].buttons[button]; }

float Input::getGamepadAxis(const GamepadAxis axis, const int controller)
{ return getInstance()->gamepad_states[controller].axes[axis]; }

void Input::setCursorVisible(const bool visible)
{
    glfwSetInputMode(static_cast<GLFWwindow*>(getInstance()->window), GLFW_CURSOR,
        visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void Input::setCursorImage(CursorType type)
{
    glfwSetCursor(static_cast<GLFWwindow*>(getInstance()->window),
        static_cast<GLFWcursor*>(getInstance()->cursors[type]));
}

void Input::applyCallbackBindings()
{
    if (!getInstance())
        return;
    getInstance()->window = Window::getWindow();
    glfwSetKeyCallback(static_cast<GLFWwindow*>(getInstance()->window),
        reinterpret_cast<GLFWkeyfun>(Input::keyCallback));
    glfwSetMouseButtonCallback(static_cast<GLFWwindow*>(getInstance()->window),
        reinterpret_cast<GLFWmousebuttonfun>(Input::mouseButtonCallback));
    ImGui_ImplGlfw_InstallCallbacks(static_cast<GLFWwindow*>(getInstance()->window));
}

void Input::keyCallback(GPUHandle window, const int key, const int scancode, const int action,
    const int mods)
{
    if (ImGui::GetIO().WantTextInput) return;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) getInstance()->pressed_since_checked.insert(key);
}

void Input::mouseButtonCallback(GPUHandle window, const int button, const int action, const int mods)
{
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (action == GLFW_PRESS)
        getInstance()->pressed_since_checked_mouse.insert(static_cast<MouseButton>(button));
}

Input::Input(const InitParams& params, bool& success)
{
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    cursors[CURSOR_NORMAL]            = nullptr;
    cursors[CURSOR_RESIZE_HORIZONTAL] = glfwCreateStandardCursor(GLFW_RESIZE_EW_CURSOR);
    cursors[CURSOR_RESIZE_VERTICAL]   = glfwCreateStandardCursor(GLFW_RESIZE_NS_CURSOR);
    cursors[CURSOR_TEXT]              = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    cursors[CURSOR_CROSSHAIR]         = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);
    cursors[CURSOR_HAND]              = glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);
    cursors[CURSOR_BUSY]              = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
    applyCallbackBindings();
    success = true;
}
