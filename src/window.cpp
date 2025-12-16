#include "window.h"

#include <stb_image.h>

#include "package.h"

using namespace HopEngine;
using namespace std;

Window::Window(uint32_t _width, uint32_t _height, string title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    width = _width;
    height = _height;
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

void Window::pollEvents() const
{
    DBG_BABBLE("polling window events");
    glfwPollEvents();
}

bool Window::getShouldClose() const
{
    return glfwWindowShouldClose(window);
}

bool HopEngine::Window::isMinified() const
{
    return glfwGetWindowAttrib(window, GLFW_ICONIFIED);
}

bool Window::isResized()
{
    int new_width;
    int new_height;
    glfwGetFramebufferSize(window, &new_width, &new_height);
    if (new_width != width || new_height != height)
    {
        width = new_width;
        height = new_height;
        return true;
    }

    return false;
}

pair<uint32_t, uint32_t> Window::getSize()
{
    glfwGetFramebufferSize(window, &width, &height);
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

void Window::setTitle(string title)
{
    glfwSetWindowTitle(window, title.c_str());
}

void Window::setVisible(bool visible)
{
    if (visible)
        glfwShowWindow(window);
    else
        glfwHideWindow(window);
}

void Window::setIcon(string path)
{
    GLFWimage image;

    auto image_data = Package::tryLoadFile(path);
    int img_channels;
    image.pixels = stbi_load_from_memory(image_data.data(), static_cast<int>(image_data.size()), &image.width, &image.height, &img_channels, STBI_rgb_alpha);

    glfwSetWindowIcon(window, 1, &image);
    stbi_image_free(image.pixels);
}

Window::~Window()
{
    DBG_INFO("destroying window " + PTR(this));
    glfwDestroyWindow(window);
}
