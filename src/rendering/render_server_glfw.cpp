#include "render_server.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "package.h"
#include "swapchain.h"
#include "render_pass.h"

using namespace HopEngine;
using namespace std;

bool RenderServer::getWindowShouldClose()
{ return glfwWindowShouldClose(getWindow()); }

void RenderServer::setTitle(const string &title)
{ glfwSetWindowTitle(getWindow(), title.c_str()); }

void RenderServer::setVisible(bool visible)
{
    if (visible)
        glfwShowWindow(getWindow());
    else
        glfwHideWindow(getWindow());
}

void RenderServer::setIcon(const string &path)
{
    GLFWimage image;

    // grab a png image from the package and read it
    const auto image_data = Package::load(path);
    int img_channels;
    image.pixels = stbi_load_from_memory(image_data.data(), static_cast<int>(image_data.size()), &image.width, &image.height, &img_channels, STBI_rgb_alpha);

    glfwSetWindowIcon(getWindow(), 1, &image);
    stbi_image_free(image.pixels);
}

void RenderServer::createWindow()
{
    glfwInit();
    // appropriate hints for vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // keep window invisible until vulkan is ready to draw. prevents a flashbang
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(window_size.x, window_size.y, "hop-engine", nullptr, nullptr);
    DBG_INFO("created window at " + ::to_string(window_size.x) + "x" + ::to_string(window_size.y));
}

bool RenderServer::resize()
{
    int width; int height;
    glfwGetFramebufferSize(window, &width, &height);
    glm::u32vec2 new_size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    if (new_size == window_size)
        return false;
    
    RenderServer::waitIdle();
    window_size = new_size;
    swapchain->resize(window_size.x, window_size.y);
    final_render_pass->resize();
    return true;
}

void RenderServer::destroyWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}
