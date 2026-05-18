/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"

#include <array>
#include <glm/vec2.hpp>
#include <map>
#include <set>

struct GLFWwindow;
struct GLFWcursor;

namespace HopEngine
{

/**
 * @brief singleton class which provides functionality to query and handle keyboard, mouse, and gamepad
 * input.
 */
class Input final
{
    friend class InitMachine;
public:
    /**
     * @brief enumerates existing gamepad button types. triggers are treated as buttons for
     * compatibility.
     */
    enum GamepadButton : uint8_t
    {
        GAMEPAD_RT, // right trigger
        GAMEPAD_LT, // left trigger
        GAMEPAD_A,  // A face button
        GAMEPAD_B,  // B face button
        GAMEPAD_X,  // X face button
        GAMEPAD_Y,  // Y face button
        GAMEPAD_DU, // DPAD up button
        GAMEPAD_DD, // DPAD down button
        GAMEPAD_DL, // DPAD left button
        GAMEPAD_DR, // DPAD right button
        GAMEPAD_RB, // right bumper
        GAMEPAD_LB, // left bumper
        GAMEPAD_RS, // right stick button
        GAMEPAD_LS  // left stick button
    };

    /**
     * @brief enumerates gamepad axes. some paired buttons can be read as axes for convenience. up and
     * right are always treated as positive.
     */
    enum GamepadAxis : uint8_t
    {
        GAMEPAD_RX = 2,   // right stick X-axis
        GAMEPAD_RY,       // right stick Y-axis
        GAMEPAD_LX,       // left stick X-axis
        GAMEPAD_LY,       // left stick Y-axis
        GAMEPAD_DX,       // DPAD X-axis
        GAMEPAD_DY,       // DPAD Y-axis
        GAMEPAD_BUMPERS,  // bumpers
        GAMEPAD_TRIGGERS, // triggers
    };

    /**
     * @brief describes the current state of a gamepad. multiple gamepads may be connected at the same
     * time. `buttons` and `axes` are indexed via the `GamepadButton` and `GamepadAxis` enums
     * respectively.
     * when a button is released, its value is `false`; when held it is `true`. when an axis is neutral
     * (i.e. neither direction is active), its value is `0`; when both directions are active to the same
     * extent (as is possible with some axes), its value is also `0`. for axes, right and up are
     * considered positive directions, and left and down are considered negative directions.
     */
    struct GamepadState final
    {
        bool buttons[14] = { false }; // states of the gamepad buttons, indexed with `GamepadButton`
        float axes[9]    = { 0.0f };  // states of the gamepad axes, indexed with `GamepadAxis`
    };

    /**
     * @brief enumerates mouse buttons.
     */
    enum MouseButton : uint8_t
    {
        MOUSE_LEFT,
        MOUSE_RIGHT,
        MOUSE_MIDDLE,
    };

    /**
     * @brief enumerates available cursor image types.
     */
    enum CursorType : uint8_t
    {
        CURSOR_NORMAL,            // regular cursor
        CURSOR_RESIZE_HORIZONTAL, // horizontal resize arrows
        CURSOR_RESIZE_VERTICAL,   // vertical resize arrows
        CURSOR_TEXT,              // I-beam text cursor
        CURSOR_CROSSHAIR,         // plus-shaped crosshair
        CURSOR_HAND,              // grabby hand
        CURSOR_BUSY,              // loading wheel or sand-timer cursor
        CURSOR_MAX_ENUM           // invalid cursor type used for iterating the enum
    };

    /**
     * @brief enumerates non-character keyboard keys.
     */
    enum KeyboardKey : uint16_t
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
        KEY_NUMP_0        = 320,
        KEY_NUMP_1        = 321,
        KEY_NUMP_2        = 322,
        KEY_NUMP_3        = 323,
        KEY_NUMP_4        = 324,
        KEY_NUMP_5        = 325,
        KEY_NUMP_6        = 326,
        KEY_NUMP_7        = 327,
        KEY_NUMP_8        = 328,
        KEY_NUMP_9        = 329,
        KEY_NUMP_DECIMAL  = 330,
        KEY_NUMP_DIVIDE   = 331,
        KEY_NUMP_MULTIPLY = 332,
        KEY_NUMP_SUBTRACT = 333,
        KEY_NUMP_ADD      = 334,
        KEY_NUMP_ENTER    = 335,
        KEY_NUMP_EQUAL    = 336,
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

private:
    GLFWwindow* window; // GLFW window object handle, initialised by `RenderServer`
    std::array<GLFWcursor*, CURSOR_MAX_ENUM> cursors; // GLFW cursors created for each cursor type
    // set of keyboard keys which have been pressed since the last time they were queried
    std::set<uint16_t> pressed_since_checked;
    // set of mouse buttons which have been pressed down since the last time they were queried
    std::set<MouseButton> pressed_since_checked_mouse;
    // map of states of currently attached gamepads
    std::map<int, GamepadState> gamepad_states;
    glm::vec2 mouse_delta;    // distance (in pixels) moved by the mouse between this frame and the last
    glm::vec2 mouse_position; // position (in pixels) of the mouse from the top-left window corner
    glm::vec2 mouse_lock_min; // minimum coordinate (in pixels) of the mouse lock rect
    glm::vec2 mouse_lock_max; // maximum coordinate (in pixels) of the mouse lock rect
    bool lock_mouse = false;  // whether the mouse is currently being locked to a rectangle

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Input);

    /**
     * @brief checks if there are new input events (mouse/keyboard) and queries the gamepad states. also
     * recalculates the mouse delta.
     */
    static void pollInput();
    /**
     * @brief updates the input manager's reference to the current window, and installs input callbacks,
     * for GLFW events (and also hooks up ImGui input callbacks).
     */
    static void applyCallbackBindings();

    /**
     * @brief checks if a key is currently being held down. immediately queries GLFW.
     * @param key the key to check for. may be a printable ASCII character, or a member of the
     * `KeyboardKey` enum.
     * @returns `true` if the key is currently pressed down, otherwise `false`.
     */
    static bool isKeyDown(int key);
    /**
     * @brief checks if a key has begun being pressed since this function was last called for that key.
     * this means that this function will not return `true` again for the same key until the user lifts
     * the key and presses it again.
     * @param key the key to check for. may be a printable ASCII character, or a member of the
     * `KeyboardKey` enum.
     * @returns `true` if the key has begun being pressed, or `false` if no new key-down event has
     * happened for this key.
     */
    static bool wasKeyPressed(int key);
    /**
     * @brief checks the states of two keys, and converts their combination into an axis value of either
     * -1, 0, or 1. immediately queries GLFW.
     * @param key_negative key which contributes negatively to the value if pressed.
     * @param key_positive key which contributes positively to the value if pressed.
     * @returns floating point value of either -1 (if `key_negative` is currently pressed down), 1 (if
     * `key_positive` is currently pressed down), or 0 (if both keys are currently pressed down).
     */
    static float getAxis(int key_negative, int key_positive);

    /**
     * @brief checks if a mouse button is currently being held down. immediately queries GLFW.
     * @param button mouse button for which to check the state of.
     * @returns `true` if the button is currently pressed down, otherwise `false`.
     */
    static bool isMouseDown(MouseButton button);
    /**
     * @brief checks if a mouse button has begun being pressed since this function was last called for
     * that button. this means that this function will not return `true` again for the same button until
     * the user lifts their finger and presses it again.
     * @param button the button to check for.
     * @returns `true` if the button has begun being pressed, or `false` if no new mouse-down event has
     * happened for this button.
     */
    static bool wasMousePressed(MouseButton button);
    /**
     * @brief queries how the mouse has moved since the last frame.
     * @returns mouse movement delta in pixels.
     */
    static glm::vec2 getMouseDelta();
    /**
     * @brief queries the current mouse position. only updated at the beginning of the frame.
     * @returns mouse position in pixels, relative to the top-left corner of the window.
     */
    static glm::vec2 getMousePosition();
    /**
     * @brief locks the mouse position to be contained within a rectangle, defined by a minimum and
     * maximum, relative to the top-left corner of the window. if the mouse is already locked, the lock
     * rect is updated.
     * @param min minimum position (top-left corner) of the lock rect, in pixels.
     * @param max maximum position (bottom-right corner) of the lock rect, in pixels.
     */
    static void lockMouseToRectangle(glm::vec2 min, glm::vec2 max);
    /**
     * @brief unlocks the mouse position, allowing it to be moved freely.
     */
    static void unlockMouse();
    /**
     * @brief toggles cursor visibility. while invisible, the cursor's position is locked in place,
     * although the mouse delta still behaves as normal.
     * @param visible `true` if the cursor should be free and visible, `false` if the cursor should be
     * invisible and locked in place.
     */
    static void setCursorVisible(bool visible);
    /**
     * @brief updates the currently used cursor representation.
     * @param type cursor image type to use from now on.
     */
    static void setCursorImage(CursorType type);

    /**
     * @brief checks if a gamepad button is currently pressed down.
     * @param button which button to query the state for.
     * @param controller optionally, which controller index to query.
     * @returns `true` if the button is currently pressed down, otherwise `false`.
     */
    static bool isGamepadButtonDown(GamepadButton button, int controller = 0);
    /**
     * @brief evaluates a gamepad axis, giving a value between -1 and 1. up and right are considered to
     * be positive directions, while down and left are considered to be negative directions.
     * @param axis which axis to query the value of.
     * @param controller optionally, which controller index to query.
     * @returns floating point value between -1 and 1, where 0 represents a neutral state.
     */
    static float getGamepadAxis(GamepadAxis axis, int controller = 0);

private:
    Input();
    ~Input() = default;
    static Input* getInstance();

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
};

} // namespace HopEngine
