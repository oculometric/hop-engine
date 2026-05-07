#include "render_server.h"

#include <chrono>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "command_buffer.h"
#include "engine.h"
#include "framebuffer.h"
#include "material.h"
#include "vulkan_helpers.h"

#include <GLFW/glfw3.h>

using namespace HopEngine;

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    switch (message_severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        DBG_VERBOSE("[ VALIDATION ]: " + std::string(callback_data->pMessage));
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        DBG_INFO("[ VALIDATION ]: " + std::string(callback_data->pMessage));
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        DBG_WARNING("[ VALIDATION ]: " + std::string(callback_data->pMessage));
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        DBG_ERROR("[ VALIDATION ]: " + std::string(callback_data->pMessage));
        break;
    default: break;
    }

    return VK_FALSE;
}

void RenderServer::waitIdle() { vkDeviceWaitIdle(static_cast<VkDevice>(getInstance()->device)); }

std::vector<uint32_t> RenderServer::getUniqueQueueIndices()
{
    std::vector<uint32_t> unique_queue_families;
    unique_queue_families.push_back(getInstance()->queue_families.graphics_family.value());
    if (getInstance()->queue_families.graphics_family.has_value())
    {
        uint32_t present = getInstance()->queue_families.present_family.value();
        if (unique_queue_families[0] != present) unique_queue_families.push_back(present);
    }
    return unique_queue_families;
}

RenderServer::QueueFamilies RenderServer::queryQueueFamilies(GPUHandle _device)
{
    QueueFamilies families;
    VkPhysicalDevice device = static_cast<VkPhysicalDevice>(_device);

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());
    int i = 0;
    for (const auto& queueFamily : queue_families)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) families.graphics_family = i;
        VkBool32 queue_has_present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, static_cast<VkSurfaceKHR>(getInstance()->surface),
            &queue_has_present_support);
        if (queue_has_present_support) families.present_family = i;

        ++i;
    }

    return families;
}

GPUHandle RenderServer::createPipelineLayout(GPUHandle set_2)
{
    VkDescriptorSetLayout layouts[3] = {};
    layouts[0] = static_cast<VkDescriptorSetLayout>(getInstance()->scene_descriptor_set_layout);
    layouts[1] = static_cast<VkDescriptorSetLayout>(getInstance()->object_descriptor_set_layout);
    layouts[2] = static_cast<VkDescriptorSetLayout>(set_2);
    VkPipelineLayoutCreateInfo layout_create_info{};
    layout_create_info.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_create_info.setLayoutCount = 3;
    layout_create_info.pSetLayouts    = layouts;

    VkPipelineLayout pipeline_layout;
    if (vkCreatePipelineLayout(static_cast<VkDevice>(RenderServer::getDevice()), &layout_create_info,
            nullptr, &pipeline_layout) != VK_SUCCESS)
        DBG_FAULT("vkCreatePipelineLayout failed");

    return pipeline_layout;
}

void RenderServer::queueFree(std::function<void()> destructor)
{ getInstance()->free_list.push_back(destructor); }

void RenderServer::createVulkan(bool enable_validation)
{
    createInstance(enable_validation);

    CHECK_RESULT(glfwCreateWindowSurface,
        (static_cast<VkInstance>(instance), window, nullptr, reinterpret_cast<VkSurfaceKHR*>(&surface)),
        FAULT,
        ;);

    createDevice();

    uint32_t frames_in_flight = Swapchain::computeImageCount();
    DBG_VERBOSE("adjusted frames in flight to " + std::to_string(frames_in_flight));

    {
        std::array<VkDescriptorPoolSize, 2> descriptor_pool_sizes;
        descriptor_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_pool_sizes[0].descriptorCount = static_cast<uint32_t>(512 * 3 * frames_in_flight);
        descriptor_pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_sizes[1].descriptorCount = static_cast<uint32_t>(512 * 4 * frames_in_flight);
        VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
        descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
        descriptor_pool_create_info.pPoolSizes    = descriptor_pool_sizes.data();
        descriptor_pool_create_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptor_pool_create_info.maxSets       = static_cast<uint32_t>(512 * 8 * frames_in_flight);

        DBG_VERBOSE("creating descriptor pool with " + std::to_string(descriptor_pool_create_info.maxSets) +
                    " max sets");
        CHECK_RESULT(vkCreateDescriptorPool,
            (static_cast<VkDevice>(device), &descriptor_pool_create_info, nullptr,
                reinterpret_cast<VkDescriptorPool*>(&descriptor_pool)),
            FAULT,
            ;);
    }

    {
        VkCommandPoolCreateInfo pool_create_info{};
        pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_create_info.queueFamilyIndex = getGraphicsQueueIndex();

        CHECK_RESULT(vkCreateCommandPool,
            (static_cast<VkDevice>(device), &pool_create_info, nullptr,
                reinterpret_cast<VkCommandPool*>(&command_pool)),
            FAULT,
            ;);

        for (uint32_t i = 0; i < static_cast<uint32_t>(frames_in_flight); ++i)
            command_buffers.push_back(new DrawCommandBuffer());
    }
}

void RenderServer::createInstance(bool debug)
{
    // application info
    VkApplicationInfo app_info{};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "HopEngine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "HopEngine";
    app_info.engineVersion      = VK_MAKE_VERSION(0, 3, 0);
    app_info.apiVersion         = VK_API_VERSION_1_4;

    // use extensions required by GLFW
    VkInstanceCreateInfo create_info{};
    create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    // query which instance extensions are needed by GLFW
    uint32_t glfw_extension_count;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char*> extensions_to_enable(glfw_extension_count);
    memcpy(extensions_to_enable.data(), glfw_extensions, extensions_to_enable.size() * sizeof(const char*));
    if (debug) extensions_to_enable.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions_to_enable.size());
    create_info.ppEnabledExtensionNames = extensions_to_enable.data();
    DBG_VERBOSE("enabling " + std::to_string(create_info.enabledExtensionCount) + " extensions:");
    DBG_VERBOSE("found " + std::to_string(glfw_extension_count) + " GLFW extensions.");
    for (size_t i = 0; i < create_info.enabledExtensionCount; ++i)
        DBG_VERBOSE(create_info.ppEnabledExtensionNames[i]);

    std::vector<std::string> desired_instance_layers;
    if (debug) desired_instance_layers.push_back("VK_LAYER_KHRONOS_validation");
    // query available instance layers
    uint32_t instance_layer_count;
    CHECK_RESULT(vkEnumerateInstanceLayerProperties, (&instance_layer_count, nullptr), FAULT, ;);
    std::vector<VkLayerProperties> available_instance_layers(instance_layer_count);
    vkEnumerateInstanceLayerProperties(&instance_layer_count, available_instance_layers.data());
    // check if all the required instance layers exist
    std::vector<const char*> enabled_instance_layers;
    for (const VkLayerProperties& layer : available_instance_layers)
    {
        auto it =
            std::find(desired_instance_layers.begin(), desired_instance_layers.end(), layer.layerName);
        if (it == desired_instance_layers.end()) continue;
        enabled_instance_layers.push_back(layer.layerName);
        desired_instance_layers.erase(it);
    }
    for (const std::string& layer : desired_instance_layers)
        DBG_ERROR("instance layer not found: " + layer + ", we will continue without it.");
    // enable whichever layers were selected, if available
    create_info.enabledLayerCount   = static_cast<uint32_t>(enabled_instance_layers.size());
    create_info.ppEnabledLayerNames = enabled_instance_layers.data();
    DBG_VERBOSE("enabling " + std::to_string(create_info.enabledLayerCount) + " layers:");
    for (size_t i = 0; i < create_info.enabledLayerCount; ++i)
        DBG_VERBOSE(create_info.ppEnabledLayerNames[i]);

    // create the vulkan instance
    CHECK_RESULT(vkCreateInstance, (&create_info, nullptr, reinterpret_cast<VkInstance*>(&instance)), FAULT,
        ;);

    if (debug)
    {
        DBG_VERBOSE("debug enabled, creating debug messenger");
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType             = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback         = vulkanDebugCallback;
        const auto vkCreateDebugUtilsMessengerExt = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(static_cast<VkInstance>(instance), "vkCreateDebugUtilsMessengerEXT"));
        if (!vkCreateDebugUtilsMessengerExt)
            DBG_ERROR("debug utils extension not found! debug messenger cannot be created!");
        else
            CHECK_RESULT(vkCreateDebugUtilsMessengerExt,
                (static_cast<VkInstance>(instance), &debug_create_info, nullptr,
                    reinterpret_cast<VkDebugUtilsMessengerEXT*>(&debug_messenger)),
                FAULT,
                ;);
    }
}

int RenderServer::queryPhysicalDevice(GPUHandle device, std::vector<const char*> extensions)
{
    VkPhysicalDevice test_device = static_cast<VkPhysicalDevice>(device);
    int score                    = 0;

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(test_device, &properties);
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(test_device, &features);

    // check that features we intend to use are present
    if (features.fillModeNonSolid == VK_FALSE || features.samplerAnisotropy == VK_FALSE ||
        features.independentBlend == VK_FALSE)
        return 0;

    // check that the necessary queues are present
    auto [graphics_family, present_family] = queryQueueFamilies(test_device);
    if (!graphics_family.has_value() || !present_family.has_value()) return 0;

    // check that the necessary extensions are available
    uint32_t available_extension_count;
    CHECK_RESULT(vkEnumerateDeviceExtensionProperties,
        (test_device, nullptr, &available_extension_count, nullptr), FAULT,
        ;);
    std::vector<VkExtensionProperties> available_extensions(available_extension_count);
    vkEnumerateDeviceExtensionProperties(test_device, nullptr, &available_extension_count,
        available_extensions.data());
    std::vector<const char*> required_extensions = extensions;
    for (const auto& [extension_name, spec_version] : available_extensions)
    {
        auto it = required_extensions.begin();
        for (; it != required_extensions.end(); ++it)
        {
            if (strncmp(*it, extension_name, 255)) break;
        }
        if (it != required_extensions.end()) required_extensions.erase(it);
    }
    if (!required_extensions.empty()) return 0;

    // score higher if the device is an actual GPU
    switch (properties.deviceType)
    {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   score += 1000; break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    score += 200; break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:            score += 100; break;
    default:                                     break;
    }

    // score higher if the device can do bigger images
    score += static_cast<int>(properties.limits.maxImageDimension2D);

    // score higher if the device can do immediate presentation and alpha compositing
    auto swapchain_info = Swapchain::getSwapchainSupportInfo(test_device);
    if (swapchain_info.supports_immediate_present) score += 300;
    if (swapchain_info.supports_premultiplied_alpha_composite) score += 400;

    DBG_VERBOSE("found device " + std::string(properties.deviceName) + ", scored " + std::to_string(score));
    return score;
}

void RenderServer::createDevice()
{
    // list out physical devices which are vulkan-compatible
    uint32_t physical_device_count = 0;
    CHECK_RESULT(vkEnumeratePhysicalDevices,
        (static_cast<VkInstance>(instance), &physical_device_count, nullptr), FAULT,
        ;);
    if (physical_device_count == 0) DBG_FAULT("found no valid physical devices");
    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(static_cast<VkInstance>(instance), &physical_device_count,
        physical_devices.data());
    DBG_VERBOSE("found " + std::to_string(physical_device_count) + " physical devices");

    static const std::vector<const char*> required_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    // score each device. a score of zero indicates the device is not usable for some reason
    VkPhysicalDevice best_device = VK_NULL_HANDLE;
    int best_device_score        = 0;
    for (VkPhysicalDevice test_device : physical_devices)
    {
        int score = queryPhysicalDevice(test_device, required_extensions);
        if (score > best_device_score)
        {
            best_device       = test_device;
            best_device_score = score;
        }
    }
    // check if any of the devices were suitable, and if so, use the highest scoring
    if (best_device != VK_NULL_HANDLE) physical_device = best_device;
    else
        DBG_FAULT("unable to find suitable VkPhysicalDevice");

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(static_cast<VkPhysicalDevice>(physical_device), &properties);
    DBG_INFO("selected device: " + std::string(properties.deviceName) + ", scored " +
             std::to_string(best_device_score));

    // create queues, for our queue families
    queue_families = queryQueueFamilies(static_cast<VkPhysicalDevice>(physical_device));
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    auto unique_queue_families = getUniqueQueueIndices();
    float queue_priority       = 1.0f;
    for (uint32_t family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = family;
        queue_create_info.queueCount       = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures features{};
    features.fillModeNonSolid  = VK_TRUE;
    features.samplerAnisotropy = VK_TRUE;
    features.independentBlend  = VK_TRUE;

    // actually create the logical device
    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos       = queue_create_infos.data();
    device_create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pEnabledFeatures        = &features;
    device_create_info.ppEnabledExtensionNames = required_extensions.data();
    device_create_info.enabledExtensionCount   = static_cast<uint32_t>(required_extensions.size());

    CHECK_RESULT(vkCreateDevice,
        (static_cast<VkPhysicalDevice>(physical_device), &device_create_info, nullptr,
            reinterpret_cast<VkDevice*>(&device)),
        FAULT,
        ;);

    // extract queues
    vkGetDeviceQueue(static_cast<VkDevice>(device), getGraphicsQueueIndex(), 0,
        reinterpret_cast<VkQueue*>(&graphics_queue));
    vkGetDeviceQueue(static_cast<VkDevice>(device), queue_families.present_family.value(), 0,
        reinterpret_cast<VkQueue*>(&present_queue));
}

void RenderServer::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplGlfw_InitForVulkan(window, false);
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.ApiVersion     = VK_API_VERSION_1_4;
    init_info.Instance       = static_cast<VkInstance>(instance);
    init_info.PhysicalDevice = static_cast<VkPhysicalDevice>(physical_device);
    init_info.Device         = static_cast<VkDevice>(device);
    init_info.QueueFamily    = getGraphicsQueueIndex();
    init_info.Queue          = static_cast<VkQueue>(graphics_queue);
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.DescriptorPool = static_cast<VkDescriptorPool>(descriptor_pool);
    Swapchain::getSwapchainSupportInfo(physical_device);
    init_info.MinImageCount = Swapchain::computeImageCount();
    init_info.ImageCount    = Swapchain::computeImageCount();
    init_info.Allocator     = nullptr;
    init_info.PipelineInfoMain.RenderPass =
        static_cast<VkRenderPass>(getRenderPass(swapchain->getFramebuffer()->getConfig()));
    init_info.PipelineInfoMain.Subpass     = 0;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    ImGui_ImplVulkan_Init(&init_info);
}

void RenderServer::destroyImGui()
{
    DBG_VERBOSE("\033[31mkilling imgui with a gun\033[0m");
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void RenderServer::destroyVulkan()
{
    tryFreeResources(true);

    vkDestroyCommandPool(static_cast<VkDevice>(device), static_cast<VkCommandPool>(command_pool), nullptr);

    vkDestroyDescriptorPool(static_cast<VkDevice>(device), static_cast<VkDescriptorPool>(descriptor_pool),
        nullptr);
    vkDestroyDescriptorSetLayout(static_cast<VkDevice>(device),
        static_cast<VkDescriptorSetLayout>(scene_descriptor_set_layout), nullptr);
    vkDestroyDescriptorSetLayout(static_cast<VkDevice>(device),
        static_cast<VkDescriptorSetLayout>(object_descriptor_set_layout), nullptr);
    vkDestroyDescriptorSetLayout(static_cast<VkDevice>(device),
        static_cast<VkDescriptorSetLayout>(default_descriptor_set_layout), nullptr);
    vkDestroyPipelineLayout(static_cast<VkDevice>(device),
        static_cast<VkPipelineLayout>(default_pipeline_layout), nullptr);

    if (debug_messenger)
    {
        DBG_VERBOSE("\033[31mkilling the (debug) messenger\033[0m");
        const auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(static_cast<VkInstance>(instance), "vkDestroyDebugUtilsMessengerEXT"));
        vkDestroyDebugUtilsMessengerEXT(static_cast<VkInstance>(instance),
            static_cast<VkDebugUtilsMessengerEXT>(debug_messenger), nullptr);
    }

    vkDestroyDevice(static_cast<VkDevice>(device), nullptr);
    vkDestroySurfaceKHR(static_cast<VkInstance>(instance), static_cast<VkSurfaceKHR>(surface), nullptr);
    vkDestroyInstance(static_cast<VkInstance>(instance), nullptr);
}
