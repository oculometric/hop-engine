#include "window.h"

#include <stb_image.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "package.h"

using namespace HopEngine;
using namespace std;

Window::Window(const uint32_t _width, const uint32_t _height, const string& title)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    width = static_cast<int>(_width);
    height = static_cast<int>(_height);
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    DBG_INFO("created window at " + to_string(width) + "x" + to_string(height) + ", titled '" + title + "'");
}

Window::~Window()
{
    DBG_INFO("destroying window " + PTR(this));
    glfwDestroyWindow(window);
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

glm::u32vec2 Window::getSize()
{
    glfwGetFramebufferSize(window, &width, &height);
    return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
}

bool Window::getShouldClose() const
{ return glfwWindowShouldClose(window); }

bool Window::isMinified() const
{ return glfwGetWindowAttrib(window, GLFW_ICONIFIED); }

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

void Window::setTitle(const string& title) const
{
    glfwSetWindowTitle(window, title.c_str());
}

void Window::setVisible(const bool visible) const
{
    if (visible)
        glfwShowWindow(window);
    else
        glfwHideWindow(window);
}

void Window::setIcon(const string& path) const
{
    GLFWimage image;

    const auto image_data = Package::tryLoadFile(path);
    int img_channels;
    image.pixels = stbi_load_from_memory(image_data.data(), static_cast<int>(image_data.size()), &image.width, &image.height, &img_channels, STBI_rgb_alpha);

    glfwSetWindowIcon(window, 1, &image);
    stbi_image_free(image.pixels);
}
