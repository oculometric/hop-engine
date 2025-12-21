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
	
	struct GamepadState
	{
		bool buttons[14] = { false };
		float axes[9] = { 0.0f };
	};
	
private:
	Ref<Window> window;
	std::set<int> pressed_since_checked;
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
	static bool isMouseDown(int button);
	static bool wasMousePressed(int button);
	
	static void pollGamepads();
	static bool isGamepadButtonDown(GamepadButton button, int controller = 0);
	static float getGamepadAxis(GamepadAxis axis, int controller = 0);

private:
	Input(Ref<Window> window);
	~Input();

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};

}
