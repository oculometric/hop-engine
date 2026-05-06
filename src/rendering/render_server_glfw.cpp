#include "render_server.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "command_buffer.h"
#include "engine.h"
#include "input.h"
#include "package.h"
#include "scene.h"
#include "framebuffer.h"
#include "vulkan_helpers.h"

#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <thread>

using namespace HopEngine;

bool RenderServer::getWindowShouldClose() { return glfwWindowShouldClose(getWindow()); }

void RenderServer::setTitle(const std::string& title) { glfwSetWindowTitle(getWindow(), title.c_str()); }

void RenderServer::setVisible(bool visible)
{
    if (visible) glfwShowWindow(getWindow());
    else
        glfwHideWindow(getWindow());
}

void RenderServer::setIcon(const std::string& path)
{
    GLFWimage image;

    const auto image_data = Package::load(path);
    int img_channels;
    image.pixels = stbi_load_from_memory(image_data.data(), static_cast<int>(image_data.size()),
        &image.width, &image.height, &img_channels, STBI_rgb_alpha);

    glfwSetWindowIcon(getWindow(), 1, &image);
    stbi_image_free(image.pixels);
}

void RenderServer::setBorderless(bool borderless)
{ glfwSetWindowAttrib(getWindow(), GLFW_DECORATED, !borderless); }

void RenderServer::createWindow()
{
    // glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
    glfwInit();
    // appropriate hints for vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    // keep window invisible until vulkan is ready to draw. prevents a flashbang
    window = glfwCreateWindow(window_size.x, window_size.y, "hop-engine",
        fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
    if (!glfwGetWindowAttrib(window, GLFW_TRANSPARENT_FRAMEBUFFER))
        DBG_ERROR("unable to make window framebuffer transparent");
    glfwFocusWindow(window);
    DBG_INFO("created window at " + std::to_string(window_size.x) + "x" + std::to_string(window_size.y));
}

bool RenderServer::resize(bool force_resize)
{
    if (wants_fullscreen_update)
    {
        RenderServer::waitIdle();
        glfwWaitEvents();
        destroyImGui();
        swapchain = nullptr;
        vkDestroySurfaceKHR(static_cast<VkInstance>(instance), static_cast<VkSurfaceKHR>(surface), nullptr);
        glfwDestroyWindow(window);

        if (fullscreen)
        {
            size_before_fullscreen = window_size;
            int width;
            int height;
            glfwGetMonitorWorkarea(glfwGetPrimaryMonitor(), nullptr, nullptr, &width, &height);
            window_size = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        }
        else
            window_size = size_before_fullscreen;

        createWindow();
        glfwHideWindow(window);
        CHECK_RESULT(glfwCreateWindowSurface,
            (static_cast<VkInstance>(instance), window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)),
            FAULT,
            ;);

        int window_x;
        int window_y;
        glfwGetFramebufferSize(window, &window_x, &window_y);
        window_size = { static_cast<uint32_t>(window_x), static_cast<uint32_t>(window_y) };
        swapchain   = new Swapchain(window_size);
        window_size = swapchain->getExtent();
        swapchain->setVsync(vsync);

        initImGui();

        Input::applyCallbackBindings();

        glfwShowWindow(window);
        wants_fullscreen_update = false;
        return true;
    }
    else
    {
        int window_x;
        int window_y;
        glfwGetFramebufferSize(window, &window_x, &window_y);
        glm::u32vec2 new_size = { static_cast<uint32_t>(window_x), static_cast<uint32_t>(window_y) };
        if (new_size == window_size && !wants_vsync_update && !force_resize) return false;

        window_size = new_size;
        if (wants_vsync_update)
        {
            swapchain->setVsync(vsync);
            wants_vsync_update = false;
        }
        swapchain->resize(window_size);

        return true;
    }
}

FrameStats RenderServer::draw()
{
    if (glfwGetWindowAttrib(getInstance()->window, GLFW_ICONIFIED))
    {
        std::this_thread::sleep_for(std::chrono::duration<float>(0.125f));
        return {};
    }

    if (getInstance()->resize()) return {};

    DBG_VERBOSE("drawing frame " + std::to_string(Engine::getFrameCount()));

    FrameStats stats{};

    uint32_t image_index = getInstance()->swapchain->acquireNextImage();
    if (image_index == (uint32_t)-1) return {};

    const auto record_start = std::chrono::steady_clock::now();
    auto command_buffer     = getInstance()->recordRenderCommands(image_index, stats);
    const std::chrono::duration<float> record_duration = std::chrono::steady_clock::now() - record_start;
    stats.record_time                                  = record_duration.count();

    getInstance()->swapchain->submitCommands(command_buffer, image_index);

    command_buffer->extractTiming();

    getInstance()->updateTextMesh();
    getInstance()->tryFreeResources();

    return stats;
}

void RenderServer::destroyWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}
