#include "window.h"

using namespace HopEngine;
using namespace std;

Window::Window(uint32_t width, uint32_t height, string title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    DBG_INFO("created window at " + to_string(width) + "x" + to_string(height) + ", titled '" + title + "'");
}

void Window::initEnvironment()
{
    DBG_INFO("initialising GLFW");
    glfwInit();
}

void Window::terminateEnvironment()
{
    DBG_INFO("terminating GLFW");
    glfwTerminate();
}

void Window::pollEvents()
{
    DBG_BABBLE("polling window events");
    glfwPollEvents();
}

bool Window::getShouldClose()
{
    return glfwWindowShouldClose(window);
}

pair<uint32_t, uint32_t> Window::getSize()
{
    int width; int height;
    glfwGetFramebufferSize(window, &width, &height);
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

Window::~Window()
{
    DBG_INFO("destroying window " + PTR(this));
    glfwDestroyWindow(window);
}
