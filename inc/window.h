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
#include "events.h"
#include "glm/glm.hpp"

namespace HopEngine
{

class Window final
{
    friend class InitMachine;

public:
    struct InitParams final
    {
        bool transparent_framebuffer = false;
        bool fullscreen              = false;
        bool borderless              = false;
        bool resizable               = true;
        bool vsync                   = true;
        std::string title            = "window";
        glm::u32vec2 size            = { 1024, 1024 };
    };

    /**
     * @brief window-specific event IDs which can be subscribed to via the event server.
     */
    enum Events : EventServer::TypeID
    {
        EVENT_TYPE_RESIZE = 0x30000001, // called when the framebuffer size changes
    };

private:
    GPUHandle window                    = nullptr; // main window that the user will see and interact with
    GPUHandle surface                   = nullptr;
    glm::u32vec2 size                   = { 1024, 1024 }; // current size of the window, and the surface
    glm::u32vec2 size_before_fullscreen = { 1024, 1024 };
    bool transparent                    = false; // if `true`, the window should be transparently composited
    bool fullscreen                     = false; // if `true`, the window should be borderless fullscreen
    std::string title                   = "window";
    bool borderless                     = false;
    bool visible                        = true;
    bool resizable                      = true;
    bool vsync                          = true;
    DataBlock icon_data;

    bool swapchain_needs_reset = false;
    Ref<Swapchain> swapchain   = nullptr;

public:
    static glm::u32vec2 getSize() { return getInstance()->size; }
    /**
     * @brief resizes the window.
     * @param size intended new size for the window. you should check that the framebuffer size
     * has been updated correctly.
     */
    static void setSize(glm::u32vec2 size);
    static glm::u32vec2 getPosition();
    static void setPosition(glm::u32vec2 position);
    static bool isTransparent() { return getInstance()->transparent; }
    static void setTransparent(bool transparent);
    static bool isFullscreen() { return getInstance()->fullscreen; }
    /**
     * @brief toggles whether the window is set in fullscreen mode or not. when fullscreened, the window
     * takes up the entire monitor, shows on top of all other windows, and lacks decorations. when the
     * window becomes unfocused in this state, it becomes entirely invisible until focused again.
     * @param fullscreen `true` if fullscreen mode should be used, or `false` if standard windowed mode
     * should be used.
     */
    static void setFullscreen(bool fullscreen);
    static std::string getTitle() { return getInstance()->title; }
    /**
     * @brief updates the window title.
     * @param title new text to be displayed in the window title, if the title bar is visible.
     */
    static void setTitle(const std::string& title);
    static bool isBorderless() { return getInstance()->borderless; }
    /**
     * @brief toggles window decoration.
     * @param borderless if `true`, then no decorations will be shown (window borders and title bar),
     * otherwise the platform-default decorations are drawn. window title bar controls may also be
     * unavailable to the user.
     */
    static void setBorderless(bool borderless);
    static bool isVsyncEnabled() { return getInstance()->vsync; }
    /**
     * @brief toggles whether image presentation to the window is tied to the screen's refresh rate.
     * generally this should be enabled unless you're trying to find out how fast she can go or you're
     * trying to flex.
     * @param enabled if `true`, images will not be presented to the screen faster than the screen's refresh
     * rate. if `false`, images will be presented to the screen as fast as they can be rendered.
     */
    static void setVsyncEnabled(bool enabled);
    static bool isVisible() { return getInstance()->visible; }
    /**
     * @brief toggles window visibility.
     * @param visible if `true`, the window will be shown, otherwise the window will be made invisible.
     */
    static void setVisible(bool visible);
    static bool isResizable() { return getInstance()->resizable; }
    /**
     * @brief toggles window resizability.
     * @param resizable if `true` the user is able to resize the window, otherwise the windows size is fixed
     * (until `setSize` is used).
     */
    static void setResizable(bool resizable);
    static void setAspectRatioLock(int numerator, int denominator);
    static void clearAspectRatioLock();
    /**
     * @brief updates the window icon.
     * @param path path to the window icon file.
     */
    static void setIcon(const std::string& path);
    static void setIcon(const DataBlock& data);
    static bool isMinimised();
    /**
     * @brief checks if the GLFW window is waiting to be terminated.
     * @returns `true` if the user has just clicked the close button or otherwise closed the window,
     * otherwise `false`.
     */
    static bool shouldClose();

    /**
     * @brief checks for pending swapchain-altering operations: window resize, fullscreen toggle, and vsync
     * toggle, and resizes and recreates internal resources as needed.
     */
    static bool refreshSwapchain();
    static Ref<Swapchain> getSwapchain() { return getInstance()->swapchain; }

    // INTERNAL
    static GPUHandle getWindow() { return getInstance()->window; }
    static GPUHandle getSurface();

private:
    Window(const InitParams& params, bool& success);
    ~Window();
    static Window* getInstance();

    /**
     * @brief initialises GLFW and creates the window.
     */
    void createWindow();
    void createSurface();
    void destroySurface();
    /**
     * @brief destroys the window and de-initialises GLFW.
     */
    void destroyWindow();
};

} // namespace HopEngine
