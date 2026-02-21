#include "render_server.h"

#include <vector>
#include <array>
#include <chrono>
#include <filesystem>
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "hop_engine.h"
#include "command_buffer.h"

using namespace HopEngine;
using namespace std;

static RenderServer* server = nullptr;

void RenderServer::init(const Ref<Window>& main_window)
{
    DBG_INFO("initialising graphics server");
    if (server == nullptr)
        server = new RenderServer(main_window);
}

void RenderServer::destroy()
{
    DBG_INFO("destroying graphics server");
    if (server != nullptr)
    {
        delete server;
        server = nullptr;
    }
}

RenderServer::RenderServer(const Ref<Window>& main_window)
{
    server = this;
    window = main_window;

    createInstance();
    if (glfwCreateWindowSurface(instance, window->getWindow(), nullptr, &surface) != VK_SUCCESS)
        DBG_FAULT("glfwCreateWindowSurface failed");
    createDevice();

    const auto framebuffer_size = window->getSize();
    swapchain = new Swapchain(framebuffer_size.x, framebuffer_size.y, surface);
    MAX_FRAMES_IN_FLIGHT = static_cast<int>(swapchain->getImageCount());
    DBG_VERBOSE("adjusted frames in flight to " + ::to_string(MAX_FRAMES_IN_FLIGHT));

    createDescriptorPoolAndSets();
    createCommandPool();
    createSyncObjects();

    final_render_pass = new RenderPass(swapchain, { 0, true });
    offscreen_pass = new RenderPass(1, 1, { 3, true });

    uint8_t default_image_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    default_image = new Texture(1, 1, FORMAT_R8G8B8A8_SRGB, TextureBuilder().data(default_image_data));
    default_sampler = new Sampler(SamplerBuilder());

    quad = new Mesh({
                        { { -1, -1, 0, 1 }, {}, {}, {}, { 0, 0 } },
                        { { 1, -1, 0, 1 }, {}, {}, {}, { 1, 0 } },
                        { { -1, 1, 0, 1 }, {}, {}, {}, { 0, 1 } },
                        { { 1, 1, 0, 1 }, {}, {}, {}, { 1, 1 } }
                    }, { 0, 3, 1, 0, 2, 3 });
    skybox_cube = new Mesh("res://engine/meshes/skybox.obj");

    default_material = new Material(new Shader("res://engine/shaders/default_shader.glsl"));
    final_pass_uniforms = new UniformBlock(ShaderLayout{
                    server->scene_descriptor_set_layout, 
                    {{ 0, UNIFORM, sizeof(SceneUniforms) } }
                });
    initImGui();

    DBG_VERBOSE("graphics server initialised");
}

size_t RenderServer::getFramesInFlight()
{ return server->MAX_FRAMES_IN_FLIGHT; }

VkDevice RenderServer::getDevice()
{ return server->device; }

VkPhysicalDevice RenderServer::getPhysicalDevice()
{ return server->physical_device; }

RenderServer::QueueFamilies RenderServer::getQueueFamilies(const VkPhysicalDevice device)
{
    QueueFamilies families;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
    int i = 0;
    for (const auto& queueFamily : queue_families)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            families.graphics_family = i;
        VkBool32 queue_has_present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, server->surface, &queue_has_present_support);
        if (queue_has_present_support)
            families.present_family = i;

        ++i;
    }

    return families;
}

VkQueue RenderServer::getGraphicsQueue()
{ return server->graphics_queue; }

VkCommandPool RenderServer::getCommandPool()
{ return server->command_pool; }

Ref<RenderPass> RenderServer::getMainRenderPass()
{ return server->offscreen_pass; }

Ref<RenderPass> RenderServer::getFinalRenderPass()
{ return server->final_render_pass; }

glm::vec2 RenderServer::getFramebufferSize()
{
    const auto ext = server->swapchain->getExtent();
    return glm::vec2{ static_cast<float>(ext.x), static_cast<float>(ext.y) };
}

VkDescriptorPool RenderServer::getDescriptorPool()
{ return server->descriptor_pool; }

VkDescriptorSetLayout RenderServer::getSceneDescriptorSetLayout()
{ return server->scene_descriptor_set_layout; }

VkDescriptorSetLayout RenderServer::getObjectDescriptorSetLayout()
{ return server->object_descriptor_set_layout; }

pair<Ref<Texture>, Ref<Sampler>> RenderServer::getDefaultTextureSampler()
{ return { server->default_image, server->default_sampler }; }

Ref<Material> RenderServer::getDefaultMaterial()
{ return server->default_material; }

Ref<Mesh> RenderServer::getSkyboxCube()
{ return server->skybox_cube; }

Ref<Mesh> RenderServer::getQuad()
{ return server->quad; }

void RenderServer::waitIdle()
{ vkDeviceWaitIdle(server->device); }

FrameStats RenderServer::draw()
{ return server->drawFrame(); }

void RenderServer::resize()
{
    RenderServer::waitIdle();
    server->resizeSwapchain();
}

void RenderServer::setSingleScene(const Ref<Scene>& scene)
{
    setMultiScene({ { scene, glm::vec2{ 0, 0 }, glm::vec2{ 1, 1 } } });
}

void RenderServer::setMultiScene(const vector<MultiSceneRenderSpec>& multi_scenes)
{
    RenderServer::waitIdle();
    server->scenes.clear();
    for (auto& scene : multi_scenes)
        server->scenes.emplace_back(scene);
}

FrameStats RenderServer::drawFrame()
{
    static size_t frame_index = 0;
    ++frame_index;
    DBG_BABBLE("drawing frame " + to_string(frame_index));

    static auto start_time = chrono::steady_clock::now();
    FrameStats stats;
    const auto now_time = chrono::steady_clock::now();
    const chrono::duration<float> since_start = now_time - start_time;

    vkWaitForFences(device, 1, &in_flight_fences[frame_index % MAX_FRAMES_IN_FLIGHT], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fences[frame_index % MAX_FRAMES_IN_FLIGHT]);

    uint32_t image_index;
    vkAcquireNextImageKHR(device, swapchain->getSwapchain(), UINT64_MAX, image_available_semaphores[frame_index % MAX_FRAMES_IN_FLIGHT], VK_NULL_HANDLE, &image_index);
    DBG_BABBLE("acquired image " + to_string(image_index));

    const auto build_start = chrono::steady_clock::now();
    updateUniforms(image_index, since_start.count(), stats);
    const chrono::duration<float> build_duration = chrono::steady_clock::now() - build_start;
    stats.build_time = build_duration.count();

    const auto record_start = chrono::steady_clock::now();
    recordRenderCommands(image_index, stats);
    const chrono::duration<float> record_duration = chrono::steady_clock::now() - record_start;
    stats.record_time = record_duration.count();
    
    VkSubmitInfo submit_info{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    const VkSemaphore wait_semaphores[] = { image_available_semaphores[frame_index % MAX_FRAMES_IN_FLIGHT] };
    constexpr VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    VkCommandBuffer cmd_buf = command_buffers[image_index]->getCommandBuffer();
    submit_info.pCommandBuffers = &cmd_buf;
    const VkSemaphore signal_semaphores[] = { render_finished_semaphores[image_index] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    DBG_BABBLE("submitting command buffer");
    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[frame_index % MAX_FRAMES_IN_FLIGHT]) != VK_SUCCESS)
        DBG_FAULT("vkQueueSubmit failed");

    VkPresentInfoKHR present_info{ };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    const VkSwapchainKHR swapchains[] = { swapchain->getSwapchain() };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    DBG_BABBLE("submitting present");
    vkQueuePresentKHR(present_queue, &present_info);
    
    command_buffers[image_index]->extractTiming();
    return stats;
}

void RenderServer::resizeSwapchain()
{
    vkDeviceWaitIdle(device);

    const auto new_size = window->getSize();
    swapchain->resize(new_size.x, new_size.y);
    
    final_render_pass->resize();
}

void RenderServer::recordRenderCommands(uint32_t image_index, FrameStats& stats)
{
    Ref<DrawCommandBuffer> command_buffer = command_buffers[image_index];
    command_buffer->begin(image_index, &stats);
    
    for (auto& scene : scenes)
    {
        if (scene.scene && scene.scene->render_graph)
            scene.scene->render_graph->recordCommandBuffer(command_buffer, scene.scene, stats);
    }
    
    final_render_pass->begin(command_buffer, glm::vec3{ 0.02f, 0.02f, 0.02f });
    
    for (auto& scene : scenes)
    {
        if (scene.scene && scene.scene->render_graph)
        {
            scene.scene->render_graph->bind(command_buffer);
            final_pass_uniforms->bind(command_buffer, 0);
        
            command_buffer->setScissorViewport(scene.start_uv, scene.size_uv, swapchain->getExtent());
            quad->draw(command_buffer);
        }
    }
    
    command_buffer->drawImGui();

    command_buffer->end();
}

void RenderServer::updateUniforms(uint32_t image_index, float time_since_start, FrameStats& stats)
{
    size_t valid_scenes = 0;
        
    SceneUniforms scene_uniforms;
    scene_uniforms.time = time_since_start;
    scene_uniforms.eye_position = { 0, 0, 0 };
    scene_uniforms.viewport_size = swapchain->getExtent();
    scene_uniforms.world_to_view = glm::mat4(1);
    scene_uniforms.view_to_clip = glm::mat4(1);
    scene_uniforms.clip_to_view = glm::mat4(1);
    scene_uniforms.view_to_world = glm::mat4(1);
    scene_uniforms.near_far = { -1, 1 };
    memcpy(final_pass_uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));
    final_pass_uniforms->pushToDescriptorSet(image_index);
    
    for (auto& scene : scenes)
    {
        if (!scene.scene)
            continue;
        scene.scene->updateUniforms(image_index, time_since_start, glm::vec2(swapchain->getExtent()) * scene.size_uv, stats);
        ++valid_scenes;
    }
    if (valid_scenes == 0)
        DBG_WARNING("no scene attached to server");
}

bool DrawCommand::operator()(const DrawCommand& a, const DrawCommand& b) const
{
    if (a.draw_priority <= b.draw_priority)
        return false;
    if (a.material->getShader() > b.material->getShader())
        return false;
    if (a.material > b.material)
        return false;
    if (a.uniforms > b.uniforms)
        return false;
    if (a.mesh > b.mesh)
        return false;
    return true;
}
