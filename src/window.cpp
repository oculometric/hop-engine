#include "window.h"

using namespace std;
using namespace HopEngine;

Window::Window(uint32_t width, uint32_t height, string title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

void Window::initEnvironment()
{
    glfwInit();
}

void Window::terminateEnvironment()
{
    glfwTerminate();
}

void Window::pollEvents()
{
    glfwPollEvents();
}

bool Window::getShouldClose()
{
    return glfwWindowShouldClose(window);
}

std::pair<uint32_t, uint32_t> HopEngine::Window::getSize()
{
    int width; int height;
    glfwGetFramebufferSize(window, &width, &height);
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

Window::~Window()
{
    glfwDestroyWindow(window);
}
