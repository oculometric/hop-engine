#include "graphics_environment.h"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <chrono>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "window.h"
#include "swapchain.h"
#include "pipeline.h"
#include "render_pass.h"
#include "shader.h"
#include "mesh.h"
#include "buffer.h"
#include "material.h"
#include "uniform_block.h"
#include "object.h"
#include "texture.h"
#include "sampler.h"
#include "scene.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

static GraphicsEnvironment* environment = nullptr;

#if !defined(NDEBUG)
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    switch (message_severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        DBG_VERBOSE("[ VALIDATION ]: " + string(callback_data->pMessage));
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        DBG_INFO("[ VALIDATION ]: " + string(callback_data->pMessage));
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        DBG_WARNING("[ VALIDATION ]: " + string(callback_data->pMessage));
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        DBG_ERROR("[ VALIDATION ]: " + string(callback_data->pMessage));
        break;
    }

    return VK_FALSE;
}
#endif

GraphicsEnvironment::GraphicsEnvironment(Ref<Window> main_window)
{
    DBG_INFO("initialising graphics environment");
    environment = this;
    
    window = main_window;

	createInstance();
    if (glfwCreateWindowSurface(instance, window->getWindow(), nullptr, &surface) != VK_SUCCESS)
        DBG_FAULT("glfwCreateWindowSurface failed");
    createDevice();
    createDescriptorPoolAndSets();
    auto framebuffer_size = window->getSize();
    swapchain = new Swapchain(framebuffer_size.first, framebuffer_size.second, surface);
    MAX_FRAMES_IN_FLIGHT = swapchain->getImageCount();
    DBG_VERBOSE("adjusted frames in flight to " + to_string(MAX_FRAMES_IN_FLIGHT));
    createCommandPool();
    render_pass = new RenderPass(swapchain, { 3, true });

    uint8_t default_image_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    default_image = new Texture(1, 1, VK_FORMAT_R8G8B8A8_SRGB, default_image_data);
    default_sampler = new Sampler(VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    createSyncObjects();

    initImGui();

    DBG_INFO("graphics environment initialised");
}

GraphicsEnvironment::~GraphicsEnvironment()
{
    DBG_INFO("destroying graphics environment");

    vkDeviceWaitIdle(device);

    DBG_VERBOSE("\033[31mkilling imgui with a gun\033[0m");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    DBG_VERBOSE("destroying sync resources");
    for (size_t i = 0; i < image_available_semaphores.size(); ++i)
    {
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);
    }

    DBG_VERBOSE("destroying command pool");
    vkDestroyCommandPool(device, command_pool, nullptr);

    default_image = nullptr;
    default_sampler = nullptr;
    scene = nullptr;

    DBG_VERBOSE("destroying descriptors");
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device, scene_descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(device, object_descriptor_set_layout, nullptr);

    render_pass = nullptr;
    swapchain = nullptr;

#if !defined(NDEBUG)
    DBG_VERBOSE("\033[31mkilling the (debug) messenger\033[0m");
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    func(instance, debug_messenger, nullptr);
#endif

    DBG_VERBOSE("destroying device");
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    environment = nullptr;
}

Ref<RenderPass> GraphicsEnvironment::getRenderPass() { return render_pass; }

GraphicsEnvironment::QueueFamilies GraphicsEnvironment::getQueueFamilies(VkPhysicalDevice device)
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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &queue_has_present_support);
        if (queue_has_present_support)
            families.present_family = i;

        ++i;
    }

    return families;
}

pair<Ref<Texture>, Ref<Sampler>> GraphicsEnvironment::getDefaultTextureSampler()
{
    return { default_image, default_sampler };
}

GraphicsEnvironment* GraphicsEnvironment::get()
{
    return environment;
}

void GraphicsEnvironment::drawImGui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (draw_imgui_function)
        draw_imgui_function();

    ImGui::Render();
}

void GraphicsEnvironment::drawFrame()
{
    static size_t frame_index = 0;
    DBG_BABBLE("drawing frame " + to_string(frame_index));
    static auto start_time = chrono::steady_clock::now();

    drawImGui();

    vkWaitForFences(device, 1, &in_flight_fences[frame_index % MAX_FRAMES_IN_FLIGHT], VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fences[frame_index % MAX_FRAMES_IN_FLIGHT]);

    uint32_t image_index;
    vkAcquireNextImageKHR(device, swapchain->getSwapchain(), UINT64_MAX, image_available_semaphores[frame_index % MAX_FRAMES_IN_FLIGHT], VK_NULL_HANDLE, &image_index);
    DBG_BABBLE("acquired image " + to_string(image_index));

    if (scene.isValid())
    {
        auto now_time = chrono::steady_clock::now();
        chrono::duration<float> since_start = now_time - start_time;
        VkExtent2D framebuffer_size = swapchain->getExtent();
        scene->camera->pushToDescriptorSet(image_index, { framebuffer_size.width, framebuffer_size.height }, since_start.count());

        for (Ref<Object>& object : scene->objects)
        {
            object->pushToDescriptorSet(image_index);
            object->material->pushToDescriptorSet(image_index);
        }
    }
    else
        DBG_WARNING("no scene attached to environment");

    vkResetCommandBuffer(command_buffers[image_index], 0);
    recordRenderCommands(command_buffers[image_index], image_index);

    VkSubmitInfo submit_info{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore wait_semaphores[] = { image_available_semaphores[frame_index % MAX_FRAMES_IN_FLIGHT] };
    VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &(command_buffers[image_index]);
    VkSemaphore signal_semaphores[] = { render_finished_semaphores[image_index] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    DBG_BABBLE("submitting command buffer");
    if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[frame_index % MAX_FRAMES_IN_FLIGHT]) != VK_SUCCESS)
        DBG_FAULT("vkQueueSubmit failed");

    VkPresentInfoKHR present_info{ };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    VkSwapchainKHR swapchains[] = { swapchain->getSwapchain() };
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;
    DBG_BABBLE("submitting present");
    vkQueuePresentKHR(present_queue, &present_info);
}

void GraphicsEnvironment::createInstance()
{
    // application info
    VkApplicationInfo app_info{ };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "HopEngine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "HopEngine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 3, 0);
    app_info.apiVersion = VK_API_VERSION_1_4;

    // use extensions required by GLFW
    VkInstanceCreateInfo create_info{ };
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    uint32_t extension_count;
    auto glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
    vector<const char*> extensions_to_enable(extension_count);
    memcpy(extensions_to_enable.data(), glfw_extensions, extensions_to_enable.size() * sizeof(const char*));
    extensions_to_enable.insert(extensions_to_enable.end(), required_instance_extensions.begin(), required_instance_extensions.end());
    create_info.enabledExtensionCount = static_cast<uint32_t>(extensions_to_enable.size());
    create_info.ppEnabledExtensionNames = extensions_to_enable.data();

    // apply validation layers for debug
#if !defined(NDEBUG)
    uint32_t validation_layer_count;
    vkEnumerateInstanceLayerProperties(&validation_layer_count, nullptr);
    vector<VkLayerProperties> available_validation_layers(validation_layer_count);
    vkEnumerateInstanceLayerProperties(&validation_layer_count, available_validation_layers.data());
    // check if all the required validation layers exist
    set<string> required_layers_unmet(required_validation_layers.begin(), required_validation_layers.end());
    for (const VkLayerProperties& layer : available_validation_layers)
        required_layers_unmet.erase(layer.layerName);
    if (!required_layers_unmet.empty())
    {
        DBG_FAULT("validation layer not found: " + *(required_layers_unmet.begin()));
    }
    create_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
    create_info.ppEnabledLayerNames = required_validation_layers.data();
#else
    create_info.enabledLayerCount = 0;
#endif
    DBG_VERBOSE("enabling " + to_string(create_info.enabledExtensionCount) + " extensions:");
    for (size_t i = 0; i < create_info.enabledExtensionCount; ++i)
        DBG_VERBOSE(create_info.ppEnabledExtensionNames[i]);
    DBG_VERBOSE("enabling " + to_string(create_info.enabledLayerCount) + " layers:");
    for (size_t i = 0; i < create_info.enabledLayerCount; ++i)
        DBG_VERBOSE(create_info.ppEnabledLayerNames[i]);

    // create the vulkan instance
    DBG_VERBOSE("creating vulkan instance");
    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
        DBG_FAULT("vkCreateInstance failed");

#if !defined(NDEBUG)
    DBG_VERBOSE("creating debug messenger");
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{ };
    debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_create_info.pfnUserCallback = vulkanDebugCallback;
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!func)
    {
        DBG_FAULT("debug utils not found");
        return;
    }
    if (func(instance, &debug_create_info, nullptr, &debug_messenger) != VK_SUCCESS)
        DBG_FAULT("unable to create debug messenger");
#endif
}

void GraphicsEnvironment::createDevice()
{
    // list out physical devices which are vulkan-compatible
    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    if (physical_device_count == 0)
        DBG_FAULT("found no valid VkPhysicalDevice");
    vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());
    DBG_VERBOSE("found " + to_string(physical_device_count) + " physical devices");

    // score each device. a score of zero indicates the device
    // is not usable for some reason
    multimap<int, VkPhysicalDevice> device_scores;
    for (const VkPhysicalDevice& device : physical_devices)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(device, &features);
        
        // discrete GPUs are preferred, etc
        int score = 0;
        switch (properties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1000; break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: score += 200; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 200; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU: score += 100; break;
        }

        score += properties.limits.maxImageDimension2D;

        if (features.fillModeNonSolid == VK_FALSE
            || features.samplerAnisotropy == VK_FALSE
            || features.independentBlend == VK_FALSE)
            score = 0;

        // check that the necessary queues are present
        auto queue_families = getQueueFamilies(device);
        if (!queue_families.graphics_family.has_value()
            || !queue_families.present_family.has_value())
            score = 0;

        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());
        set<string> required_extensions_unmet(required_extensions.begin(), required_extensions.end());
        for (const VkExtensionProperties& extension : available_extensions)
            required_extensions_unmet.erase(extension.extensionName);
        if (!required_extensions_unmet.empty())
            score = 0;

        if (score != 0)
        {
            auto swapchain_info = Swapchain::getSupportInfo(device, surface);
            if (swapchain_info.surface_formats.empty() || swapchain_info.present_modes.empty())
                score = 0;
        }

        DBG_BABBLE("found device " + string(properties.deviceName) + ", scored " + to_string(score));
        device_scores.insert({ score, device});
    }
    // check if any of the devices were suitable, and if so,
    // use the highest scoring
    if (device_scores.rbegin()->first > 0)
        physical_device = device_scores.rbegin()->second;
    else
        DBG_FAULT("unable to find suitable VkPhysicalDevice");
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    DBG_INFO("selected device: " + string(properties.deviceName));

    // create queues, for our queue families
    QueueFamilies queue_family_indices = getQueueFamilies(physical_device);
    vector<VkDeviceQueueCreateInfo> queue_create_infos;
    set<uint32_t> unique_queue_families = { queue_family_indices.graphics_family.value(),
                                            queue_family_indices.present_family.value() };
    float queue_priority = 1.0f;
    for (uint32_t family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{ };
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures features{ };
    features.fillModeNonSolid = VK_TRUE;
    features.samplerAnisotropy = VK_TRUE;
    features.independentBlend = VK_TRUE;
    
    // actually create the logical device
    VkDeviceCreateInfo device_create_info{ };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pEnabledFeatures = &features;
    device_create_info.ppEnabledExtensionNames = required_extensions.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
    
    DBG_INFO("creating device");
    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
        DBG_FAULT("vkCreateDevice failed");

    // extract queues
    DBG_VERBOSE("extracting queues");
    vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices.present_family.value(), 0, &present_queue);
}

void GraphicsEnvironment::createDescriptorPoolAndSets()
{
    array<VkDescriptorPoolSize, 2> descriptor_pool_sizes;
    descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_pool_sizes[0].descriptorCount = static_cast<uint32_t>(512 * 3 * MAX_FRAMES_IN_FLIGHT);
    descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_pool_sizes[1].descriptorCount = static_cast<uint32_t>(512 * 4 * MAX_FRAMES_IN_FLIGHT);
    VkDescriptorPoolCreateInfo descriptor_pool_create_info{ };
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
    descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets = static_cast<uint32_t>(512 * 8 * MAX_FRAMES_IN_FLIGHT);

    DBG_INFO("creating descriptor pool with " + to_string(descriptor_pool_create_info.maxSets) + " max sets");
    if (vkCreateDescriptorPool(device, &descriptor_pool_create_info, nullptr, &descriptor_pool) != VK_SUCCESS)
        DBG_FAULT("vkCreateDescriptorPool failed");

    VkDescriptorSetLayoutBinding uniform_layout_binding{ };
    uniform_layout_binding.binding = 0;
    uniform_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_layout_binding.descriptorCount = 1;
    uniform_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layout_create_info{ };
    layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_create_info.bindingCount = 1;
    layout_create_info.pBindings = &uniform_layout_binding;

    DBG_INFO("creating scene and object descriptor sets");
    if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &scene_descriptor_set_layout) != VK_SUCCESS)
        DBG_FAULT("vkCreateDescriptorSetLayout failed");

    if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &object_descriptor_set_layout) != VK_SUCCESS)
        DBG_FAULT("vkCreateDescriptorSetLayout failed");
}

void GraphicsEnvironment::createCommandPool()
{
    DBG_INFO("creating command pool and buffers");

    QueueFamilies queue_families = getQueueFamilies(physical_device);

    VkCommandPoolCreateInfo pool_create_info{ };
    pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_create_info.queueFamilyIndex = queue_families.graphics_family.value();

    if (vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool) != VK_SUCCESS)
        DBG_FAULT("vkCreateCommandPool failed");

    VkCommandBufferAllocateInfo buffer_allocate_info{ };
    buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_allocate_info.commandPool = command_pool;
    buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_allocate_info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateCommandBuffers(device, &buffer_allocate_info, command_buffers.data()) != VK_SUCCESS)
        DBG_FAULT("vkAllocateCommandBuffers failed");
}

void GraphicsEnvironment::createSyncObjects()
{
    DBG_INFO("creating sync objects");
    VkSemaphoreCreateInfo semaphore_create_info{ };
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_create_info{ };
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(device, &semaphore_create_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphore_create_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fence_create_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS)
            DBG_FAULT("vkCreateSemaphore failed");
    }
}

void GraphicsEnvironment::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    ImGui_ImplGlfw_InitForVulkan(window->getWindow(), true);
    ImGui_ImplVulkan_InitInfo init_info{ };
    init_info.ApiVersion = VK_API_VERSION_1_4;
    init_info.Instance = instance;
    init_info.PhysicalDevice = physical_device;
    init_info.Device = device;
    init_info.QueueFamily = getQueueFamilies(physical_device).graphics_family.value();
    init_info.Queue = graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptor_pool;
    init_info.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
    init_info.Allocator = nullptr;
    init_info.PipelineInfoMain.RenderPass = render_pass->getRenderPass();
    init_info.PipelineInfoMain.Subpass = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&init_info);
}

void GraphicsEnvironment::recordRenderCommands(VkCommandBuffer command_buffer, uint32_t image_index)
{
    DBG_BABBLE("recording command buffer");
    VkCommandBufferBeginInfo cmd_buffer_begin_info{ };
    cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(command_buffer, &cmd_buffer_begin_info) != VK_SUCCESS)
        DBG_FAULT("vkBeginCommandBuffer failed");

    VkRenderPassBeginInfo render_pass_begin_info{ };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = render_pass->getRenderPass();
    render_pass_begin_info.framebuffer = render_pass->getFramebuffer(image_index);
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = swapchain->getExtent();
    vector<VkClearValue> clear_values = render_pass->getClearValues();
    if (scene.isValid())
        clear_values[0].color = { scene->background_colour.r, scene->background_colour.g, scene->background_colour.b };
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain->getExtent().width);
    viewport.height = static_cast<float>(swapchain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{ };
    scissor.offset = { 0, 0 };
    scissor.extent = swapchain->getExtent();
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    if (scene.isValid())
    {
        for (Ref<Object>& object : scene->objects)
        {
            if (!object->material.isValid() || !object->mesh.isValid())
            {
                DBG_WARNING("object " + PTR(object.get()) + " had invalid material or mesh");
                continue;
            }

            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object->material->getPipeline());

            VkDescriptorSet scene_descriptor_set = scene->camera->getDescriptorSet(image_index);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object->material->getPipelineLayout(), 0, 1, &scene_descriptor_set, 0, nullptr);
            VkDescriptorSet material_descriptor_set = object->material->getDescriptorSet(image_index);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object->material->getPipelineLayout(), 2, 1, &material_descriptor_set, 0, nullptr);
            VkDescriptorSet object_descriptor_set = object->getDescriptorSet(image_index);
            vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object->material->getPipelineLayout(), 1, 1, &object_descriptor_set, 0, nullptr);

            VkBuffer vertex_buffers[] = { object->mesh->getVertexBuffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffer, object->mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
            vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(object->mesh->getIndexCount()), 1, 0, 0, 0);
        }
    }

    ImDrawData* draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);

    vkCmdEndRenderPass(command_buffer);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        DBG_FAULT("vkEndCommandBuffer failed");
}
