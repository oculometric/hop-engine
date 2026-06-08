#include "window.h"

#include "../rendering/vulkan_helpers.h"
#include "graphics_server.h"
#include "package.h"
#include "stb_image.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

using namespace HopEngine;

void Window::setSize(glm::u32vec2 size)
{ glfwSetWindowSize(static_cast<GLFWwindow*>(getWindow()), size.x, size.y); }

glm::u32vec2 Window::getPosition()
{
    int x_pos, y_pos;
    glfwGetWindowPos(static_cast<GLFWwindow*>(getWindow()), &x_pos, &y_pos);
    return { x_pos, y_pos };
}

void Window::setPosition(glm::u32vec2 position)
{ glfwSetWindowPos(static_cast<GLFWwindow*>(getWindow()), position.x, position.y); }

void Window::setTransparent(bool transparent)
{
    getInstance()->transparent           = transparent;
    getInstance()->swapchain_needs_reset = true;
}

void Window::setBorderless(bool borderless)
{
    int old_client_x;
    int old_client_y;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(getWindow()), &old_client_x, &old_client_y);

    glfwSetWindowAttrib(static_cast<GLFWwindow*>(getWindow()), GLFW_DECORATED, !borderless);
    glfwWaitEvents();

    int new_client_x;
    int new_client_y;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(getWindow()), &new_client_x, &new_client_y);

    static int differential_x = 0;
    static int differential_y = 0;

    if (!isBorderless() && borderless)
    {
        differential_x = new_client_x - old_client_x;
        differential_y = new_client_y - old_client_y;
    }
    else if (isBorderless() && !borderless)
    {
        glfwSetWindowSize(static_cast<GLFWwindow*>(getWindow()), old_client_x - differential_x,
            old_client_y - differential_y);
    }
    getInstance()->borderless = borderless;
}

void Window::setFullscreen(bool fullscreen)
{
    if (isFullscreen() && !fullscreen) getInstance()->size = getInstance()->size_before_fullscreen;
    else if (!isFullscreen() && fullscreen)
        getInstance()->size_before_fullscreen = getInstance()->size;
    getInstance()->fullscreen            = fullscreen;
    getInstance()->swapchain_needs_reset = true;
}

void Window::setTitle(const std::string& title)
{
    glfwSetWindowTitle(static_cast<GLFWwindow*>(getWindow()), title.c_str());
    getInstance()->title = title;
}

void Window::setVsyncEnabled(bool enabled)
{
    getInstance()->vsync                 = enabled;
    getInstance()->swapchain_needs_reset = true;
}

void Window::setVisible(bool visible)
{
    if (visible) glfwShowWindow(static_cast<GLFWwindow*>(getWindow()));
    else
        glfwHideWindow(static_cast<GLFWwindow*>(getWindow()));
    getInstance()->visible = visible;
}

void Window::setResizable(bool resizable)
{
    glfwSetWindowAttrib(static_cast<GLFWwindow*>(getWindow()), GLFW_RESIZABLE, resizable);
    getInstance()->resizable = resizable;
}

void Window::setAspectRatioLock(int numerator, int denominator)
{ glfwSetWindowAspectRatio(static_cast<GLFWwindow*>(getWindow()), numerator, denominator); }

void Window::clearAspectRatioLock()
{ glfwSetWindowAspectRatio(static_cast<GLFWwindow*>(getWindow()), GLFW_DONT_CARE, GLFW_DONT_CARE); }

void Window::setIcon(const std::string& path)
{
    GLFWimage image;

    const auto image_data = Package::load(path);
    int img_channels;
    image.pixels = stbi_load_from_memory(image_data.data(), static_cast<int>(image_data.size()),
        &image.width, &image.height, &img_channels, STBI_rgb_alpha);

    glfwSetWindowIcon(static_cast<GLFWwindow*>(getWindow()), 1, &image);
    stbi_image_free(image.pixels);
}

bool Window::isMinimised()
{ return glfwGetWindowAttrib(static_cast<GLFWwindow*>(getWindow()), GLFW_ICONIFIED); }

bool Window::shouldClose() { return glfwWindowShouldClose(static_cast<GLFWwindow*>(getWindow())); }

bool Window::refreshSwapchain()
{
    int window_x;
    int window_y;
    glfwGetFramebufferSize(static_cast<GLFWwindow*>(getWindow()), &window_x, &window_y);
    glm::u32vec2 new_size =
        glm::u32vec2{ static_cast<uint32_t>(window_x), static_cast<uint32_t>(window_y) };
    glm::u32vec2 old_size = getSize();

    if (!getSwapchain())
    {
        getInstance()->swapchain = new Swapchain(new_size);
        getInstance()->size      = getSwapchain()->getExtent();
        getSwapchain()->setVsync(getInstance()->vsync);
        return true;
    }

    if (!getInstance()->swapchain_needs_reset)
    {
        if (old_size != new_size)
        {
            // you MUST resize the swapchain, or destroy it first. trying to create multiple swapchains on
            // the same surfaces makes vulkan explode :(
            getInstance()->swapchain->resize(new_size);
            getInstance()->size = getSwapchain()->getExtent();
            EventServer::dispatch(EVENT_TYPE_RESIZE);
            return true;
        }
        return false;
    }
    getInstance()->swapchain_needs_reset = false;

    GraphicsServer::waitIdle();
    glfwWaitEvents();
    getInstance()->swapchain = nullptr;
    getInstance()->destroyWindow();
    vkDestroySurfaceKHR(static_cast<VkInstance>(GraphicsServer::getVulkanInstance()),
        static_cast<VkSurfaceKHR>(getSurface()), nullptr);

    if (isFullscreen())
    {
        int width;
        int height;
        glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), nullptr, nullptr, &width, &height);
        getInstance()->size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    }
    getInstance()->createWindow();
    setBorderless(isBorderless());
    glfwHideWindow(static_cast<GLFWwindow*>(getWindow()));
    getInstance()->createSurface();

    glfwGetFramebufferSize(static_cast<GLFWwindow*>(getWindow()), &window_x, &window_y);
    getInstance()->size      = { static_cast<uint32_t>(window_x), static_cast<uint32_t>(window_y) };
    getInstance()->swapchain = new Swapchain(getSize());
    getInstance()->size      = getSwapchain()->getExtent();
    getSwapchain()->setVsync(getInstance()->vsync);

    glfwShowWindow(static_cast<GLFWwindow*>(getWindow()));

    if (old_size != getInstance()->size) EventServer::dispatch(EVENT_TYPE_RESIZE);

    return true;
}

GPUHandle Window::getSurface()
{
    if (!getInstance()->surface) getInstance()->createSurface();
    return getInstance()->surface;
}

Window::Window(const InitParams& params, bool& success)
{
    transparent = params.transparent_framebuffer;
    fullscreen  = params.fullscreen;
    vsync       = params.vsync;
    title       = params.title;
    size        = params.size;

    createWindow();
    setBorderless(params.borderless);
    setResizable(params.resizable);

    success = true;
}

Window::~Window()
{
    glfwWaitEvents();
    swapchain = nullptr;
    destroySurface();
    destroyWindow();
}

void Window::createWindow()
{
#if defined(_WIN32)
#else
    // this fucking sucks, fuck you X11 and fuck you renderdoc (its ok renderdoc i forgive you)
    if (!transparent) glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
    glfwInit();
    // appropriate hints for vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    if (transparent) glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    else
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);
    // keep window invisible until vulkan is ready to draw. prevents a flashbang
    window = glfwCreateWindow(size.x, size.y, title.c_str(), fullscreen ? glfwGetPrimaryMonitor() : nullptr,
        nullptr);
    if (transparent && !glfwGetWindowAttrib(static_cast<GLFWwindow*>(window), GLFW_TRANSPARENT_FRAMEBUFFER))
        DBG_ERROR("unable to make window framebuffer transparent");
    glfwFocusWindow(static_cast<GLFWwindow*>(window));
    DBG_INFO("created window at " + std::to_string(size.x) + "x" + std::to_string(size.y));
}

void Window::createSurface()
{
    CHECK_RESULT(glfwCreateWindowSurface,
        (static_cast<VkInstance>(GraphicsServer::getVulkanInstance()), static_cast<GLFWwindow*>(window),
            nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)),
        FAULT,
        ;);
}

void Window::destroySurface()
{
    GraphicsServer::waitIdle();
    vkDestroySurfaceKHR(static_cast<VkInstance>(GraphicsServer::getVulkanInstance()),
        static_cast<VkSurfaceKHR>(surface), nullptr);
}

void Window::destroyWindow()
{
    glfwDestroyWindow(static_cast<GLFWwindow*>(window));
    glfwTerminate();
}
