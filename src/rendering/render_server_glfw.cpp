#include "render_server.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include "package.h"
#include "swapchain.h"
#include "render_pass.h"
#include "input.h"
#include "engine.h"
#include "scene.h"
#include "command_buffer.h"

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

void RenderServer::setBorderless(bool borderless)
{
    glfwSetWindowAttrib(getWindow(), GLFW_DECORATED, !borderless);
}

void RenderServer::createWindow()
{
    glfwInit();
    // appropriate hints for vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // keep window invisible until vulkan is ready to draw. prevents a flashbang
    //glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    window = glfwCreateWindow(window_size.x, window_size.y, "hop-engine", fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);
    glfwPollEvents();
    DBG_INFO("created window at " + ::to_string(window_size.x) + "x" + ::to_string(window_size.y));
}

bool RenderServer::resize(bool force_resize)
{
    RenderServer::waitIdle();
    if (wants_fullscreen_update)
    {
        destroyImGui();
        swapchain = nullptr;
        vkDestroySurfaceKHR(instance, surface, nullptr);
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

        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            DBG_FAULT("glfwCreateWindowSurface failed");

        QueueFamilies queueFamilyIndices = getQueueFamilies(physical_device);
        
        int window_x;
        int window_y;
        glfwGetFramebufferSize(window, &window_x, &window_y);
        window_size = { static_cast<uint32_t>(window_x), static_cast<uint32_t>(window_y) };
        swapchain = new Swapchain(window_size);
        window_size = swapchain->getExtent();
        swapchain->setVsync(vsync);
        final_render_pass = new RenderPass(swapchain, { 0, true });

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
        if (new_size == window_size && !wants_vsync_update && !force_resize)
            return false;

        window_size = new_size;
        if (wants_vsync_update)
        {
            swapchain->setVsync(vsync);
            wants_vsync_update = false;
        }
        swapchain->resize(window_size);
        final_render_pass->resize();

        return true;
    }

    return false;
}

FrameStats RenderServer::drawFrame()
{
    if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
        return { };

    if (resize())
        return { };

    size_t frame_index = Engine::getFrameCount();
    DBG_BABBLE("drawing frame " + ::to_string(frame_index));

    FrameStats stats{ };

    uint32_t image_index = swapchain->acquireNextImage();
    if (image_index == (uint32_t)-1)
        return { };

    SceneUniforms scene_uniforms;
    scene_uniforms.time = Engine::getEngineTime();
    scene_uniforms.eye_position = { 0, 0, 0 };
    scene_uniforms.viewport_size = swapchain->getExtent();
    scene_uniforms.world_to_view = glm::mat4(1);
    scene_uniforms.view_to_clip = glm::mat4(1);
    scene_uniforms.clip_to_view = glm::mat4(1);
    scene_uniforms.view_to_world = glm::mat4(1);
    scene_uniforms.near_far = { -1, 1 };
    memcpy(final_pass_uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));
    
    size_t valid_scenes = 0;
    for (auto& scene : scenes)
    {
        if (!scene.scene)
            continue;
        ++valid_scenes;
    }
    if (valid_scenes == 0)
        DBG_WARNING("no scene attached to server");

    const auto record_start = chrono::steady_clock::now();
    recordRenderCommands(image_index, stats);
    const chrono::duration<float> record_duration = chrono::steady_clock::now() - record_start;
    stats.record_time = record_duration.count();
    
    swapchain->submitCommands(command_buffers[image_index], image_index);
    
    command_buffers[image_index]->extractTiming();
    
    updateTextMesh();
    tryFreeResources();

    return stats;
}

void RenderServer::destroyWindow()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}
