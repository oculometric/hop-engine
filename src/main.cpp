#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>

#include <iostream>

#include "window.h"
#include "graphics_environment.h"

int main() {
    HopEngine::Window::initEnvironment();
    HopEngine::Window* window = new HopEngine::Window(1024, 1024, "hop!");
    HopEngine::GraphicsEnvironment* ge = new HopEngine::GraphicsEnvironment(window);

    while (!window->getShouldClose())
    {
        window->pollEvents();
        ge->drawFrame();
    }

    delete ge;
    delete window;
    HopEngine::Window::terminateEnvironment();

    return 0;
}