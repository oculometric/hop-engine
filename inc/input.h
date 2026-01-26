#pragma once

#include <glm/vec2.hpp>
#include <set>
#include <map>

#include "common.h"
#include "window.h"

struct GLFWwindow;

namespace HopEngine
{
	
class Input
{
public:
	enum GamepadButton
	{
		GAMEPAD_RTRIGGER,
		GAMEPAD_LTRIGGER,
		
		GAMEPAD_A,
		GAMEPAD_B,
		GAMEPAD_X,
		GAMEPAD_Y,
		
		GAMEPAD_UP,
		GAMEPAD_DOWN,
		GAMEPAD_LEFT,
		GAMEPAD_RIGHT,
		
		GAMEPAD_RBUTTON,
		GAMEPAD_LBUTTON,
		
		GAMEPAD_RSTICK,
		GAMEPAD_LSTICK
	};
	
	enum GamepadAxis
	{
		GAMEPAD_RX = 2,
		GAMEPAD_RY,
		
		GAMEPAD_LX,
		GAMEPAD_LY,
		
		GAMEPAD_BX,
		GAMEPAD_BY,
		
		GAMEPAD_BUTTONS
	};
	
	enum MouseButton
	{
		MOUSE_LEFT,
		MOUSE_RIGHT,
		MOUSE_MIDDLE,
	};
	
	enum KeyboardKey
	{
		KEY_ESCAPE        = 256,
		KEY_ENTER         = 257,
		KEY_TAB           = 258,
		KEY_BACKSPACE     = 259,
		KEY_INSERT        = 260,
		KEY_DELETE        = 261,
		KEY_RIGHT         = 262,
		KEY_LEFT          = 263,
		KEY_DOWN          = 264,
		KEY_UP            = 265,
		KEY_PAGE_UP       = 266,
		KEY_PAGE_DOWN     = 267,
		KEY_HOME          = 268,
		KEY_END           = 269,
		KEY_CAPS_LOCK     = 280,
		KEY_SCROLL_LOCK   = 281,
		KEY_NUM_LOCK      = 282,
		KEY_PRINT_SCREEN  = 283,
		KEY_PAUSE         = 284,
		KEY_F1            = 290,
		KEY_F2            = 291,
		KEY_F3            = 292,
		KEY_F4            = 293,
		KEY_F5            = 294,
		KEY_F6            = 295,
		KEY_F7            = 296,
		KEY_F8            = 297,
		KEY_F9            = 298,
		KEY_F10           = 299,
		KEY_F11           = 300,
		KEY_F12           = 301,
		KEY_F13           = 302,
		KEY_F14           = 303,
		KEY_F15           = 304,
		KEY_F16           = 305,
		KEY_F17           = 306,
		KEY_F18           = 307,
		KEY_F19           = 308,
		KEY_F20           = 309,
		KEY_F21           = 310,
		KEY_F22           = 311,
		KEY_F23           = 312,
		KEY_F24           = 313,
		KEY_F25           = 314,
		KEY_KP_0          = 320,
		KEY_KP_1          = 321,
		KEY_KP_2          = 322,
		KEY_KP_3          = 323,
		KEY_KP_4          = 324,
		KEY_KP_5          = 325,
		KEY_KP_6          = 326,
		KEY_KP_7          = 327,
		KEY_KP_8          = 328,
		KEY_KP_9          = 329,
		KEY_KP_DECIMAL    = 330,
		KEY_KP_DIVIDE     = 331,
		KEY_KP_MULTIPLY   = 332,
		KEY_KP_SUBTRACT   = 333,
		KEY_KP_ADD        = 334,
		KEY_KP_ENTER      = 335,
		KEY_KP_EQUAL      = 336,
		KEY_LEFT_SHIFT    = 340,
		KEY_LEFT_CONTROL  = 341,
		KEY_LEFT_ALT      = 342,
		KEY_LEFT_SUPER    = 343,
		KEY_RIGHT_SHIFT   = 344,
		KEY_RIGHT_CONTROL = 345,
		KEY_RIGHT_ALT     = 346,
		KEY_RIGHT_SUPER   = 347,
		KEY_MENU          = 348
	};
	
	struct GamepadState
	{
		bool buttons[14] = { false };
		float axes[9] = { 0.0f };
	};
	
private:
	Ref<Window> window;
	std::set<int> pressed_since_checked;
	std::set<MouseButton> pressed_since_checked_mouse;
	std::map<int, GamepadState> gamepad_states;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Input);

	static void init(Ref<Window> window);
	static void destroy();

	static bool isKeyDown(int key);
	static bool wasKeyPressed(int key);
	static float getAxis(int key_negative, int key_positive);
	static glm::vec2 getMouseDelta();
	static void resetMouseDelta();
	static glm::vec2 getMousePosition();
	static bool isMouseDown(MouseButton button);
	static bool wasMousePressed(MouseButton button);
	static void setCursorVisible(bool visible);
	
	static void pollGamepads();
	static bool isGamepadButtonDown(GamepadButton button, int controller = 0);
	static float getGamepadAxis(GamepadAxis axis, int controller = 0);

private:
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	
	Input(Ref<Window> window);
	~Input();
};

}
