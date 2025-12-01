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
#include "counted_ref.h"

using namespace HopEngine;

int main() {
    Window::initEnvironment();
    Ref<Window> window = new HopEngine::Window(1024, 1024, "hop!");
    Ref<GraphicsEnvironment> ge = new HopEngine::GraphicsEnvironment(window);

    while (!window.get()->getShouldClose())
    {
        window.get()->pollEvents();
        ge.get()->drawFrame();
    }

    ge = nullptr;
    window = nullptr;
    Window::terminateEnvironment();

    return 0;
}