#include "render_server.h"

#include <chrono>
#include <vulkan/vulkan.hpp>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "uniform_block.h"
#include "render_pass.h"
#include "swapchain.h"
#include "engine.h"
#include "material.h"
#include "command_buffer.h"
#include "vulkan_helpers.h"

#define VK_DEBUG

using namespace HopEngine;
using namespace std;

static const vector<const char*> required_validation_layers =
{
#if defined(VK_DEBUG)
    "VK_LAYER_KHRONOS_validation"
#endif
};

static const vector<const char*> required_instance_extensions =
{
#if defined(VK_DEBUG)
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};

static const vector<const char*> required_extensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#if defined(VK_DEBUG)
static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    switch (message_severity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        Debug::write("[ VALIDATION ]: " + string(callback_data->pMessage), Debug::DEBUG_VERBOSE);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        Debug::write("[ VALIDATION ]: " + string(callback_data->pMessage), Debug::DEBUG_INFO);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        Debug::write("[ VALIDATION ]: " + string(callback_data->pMessage), Debug::DEBUG_WARNING);
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        Debug::write("[ VALIDATION ]: " + string(callback_data->pMessage), Debug::DEBUG_ERROR);
        break;
    default: break;
    }

    return VK_FALSE;
}
#endif

void RenderServer::waitIdle()
{ vkDeviceWaitIdle(getInstance()->device); }

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
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, getInstance()->surface, &queue_has_present_support);
        if (queue_has_present_support)
            families.present_family = i;

        ++i;
    }

    return families;
}

VkPipelineLayout RenderServer::createPipelineLayout(VkDescriptorSetLayout set_2)
{
    VkPipelineLayoutCreateInfo layout_create_info{ };
	layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_create_info.setLayoutCount = 3;
	const VkDescriptorSetLayout layouts[3] =
	{
		getInstance()->scene_descriptor_set_layout,
		getInstance()->object_descriptor_set_layout,
		set_2
	};
	layout_create_info.pSetLayouts = layouts;

    VkPipelineLayout pipeline_layout;
	if (vkCreatePipelineLayout(RenderServer::getDevice(), &layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
		DBG_FAULT("vkCreatePipelineLayout failed");

    return pipeline_layout;
}

void RenderServer::free(std::function<void()> destructor)
{ getInstance()->free_list.push_back(destructor); }

void RenderServer::free(VkBuffer& resource)
{
    RenderServer::free([resource]()
    { vkDestroyBuffer(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkDescriptorSetLayout& resource)
{
    RenderServer::free([resource]()
    { vkDestroyDescriptorSetLayout(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkDescriptorSet& resource)
{
    RenderServer::free([resource]()
    { vkFreeDescriptorSets(getInstance()->device, getInstance()->descriptor_pool, 1, &resource); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkDeviceMemory& resource)
{
    RenderServer::free([resource]
    { vkFreeMemory(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkImage& resource)
{
    RenderServer::free([resource]()
    { vkDestroyImage(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkImageView& resource)
{
    RenderServer::free([resource]()
    { vkDestroyImageView(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkPipelineLayout& resource)
{
    RenderServer::free([resource]()
    { vkDestroyPipelineLayout(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkPipeline& resource)
{
    RenderServer::free([resource]()
    { vkDestroyPipeline(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkShaderModule& resource)
{
    RenderServer::free([resource]()
    { vkDestroyShaderModule(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkSampler& resource)
{
    RenderServer::free([resource]()
    { vkDestroySampler(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkFramebuffer& resource)
{
    RenderServer::free([resource]()
    { vkDestroyFramebuffer(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkCommandBuffer& resource)
{
    RenderServer::free([resource]()
    { vkFreeCommandBuffers(getInstance()->device, getInstance()->command_pool, 1, &resource); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkQueryPool& resource)
{
    RenderServer::free([resource]()
    { vkDestroyQueryPool(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::free(VkRenderPass& resource)
{
    RenderServer::free([resource]()
    { vkDestroyRenderPass(getInstance()->device, resource, nullptr); });
    resource = VK_NULL_HANDLE;
}

void RenderServer::createVulkan()
{
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
#if defined(VK_DEBUG)
        uint32_t validation_layer_count;
        vkEnumerateInstanceLayerProperties(&validation_layer_count, nullptr);
        vector<VkLayerProperties> available_validation_layers(validation_layer_count);
        vkEnumerateInstanceLayerProperties(&validation_layer_count, available_validation_layers.data());
        // check if all the required validation layers exist
        set<string> required_layers_unmet(required_validation_layers.begin(), required_validation_layers.end());
        vector<const char*> enabled_layers;
        for (const VkLayerProperties& layer : available_validation_layers)
        {
            if (required_layers_unmet.erase(layer.layerName))
                enabled_layers.push_back(layer.layerName);
        }
        if (!required_layers_unmet.empty())
            DBG_WARNING("validation layer not found: " + *(required_layers_unmet.begin()) + ", we will continue without it.");
        create_info.enabledLayerCount = static_cast<uint32_t>(enabled_layers.size());
        create_info.ppEnabledLayerNames = enabled_layers.data();
#else
        create_info.enabledLayerCount = 0;
#endif
        DBG_VERBOSE("enabling " + ::to_string(create_info.enabledExtensionCount) + " extensions:");
        for (size_t i = 0; i < create_info.enabledExtensionCount; ++i)
            DBG_VERBOSE(create_info.ppEnabledExtensionNames[i]);
        DBG_VERBOSE("enabling " + ::to_string(create_info.enabledLayerCount) + " layers:");
        for (size_t i = 0; i < create_info.enabledLayerCount; ++i)
            DBG_VERBOSE(create_info.ppEnabledLayerNames[i]);

        // create the vulkan instance
        DBG_VERBOSE("creating vulkan instance");
        if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
            DBG_FAULT("vkCreateInstance failed");

#if defined(VK_DEBUG)
        DBG_VERBOSE("creating debug messenger");
        VkDebugUtilsMessengerCreateInfoEXT debug_create_info{ };
        debug_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debug_create_info.pfnUserCallback = vulkanDebugCallback;
        const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        if (!func)
        {
            DBG_FAULT("debug utils not found");
            return;
        }
        if (func(instance, &debug_create_info, nullptr, &debug_messenger) != VK_SUCCESS)
            DBG_FAULT("unable to create debug messenger");
#endif
    }

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        DBG_FAULT("glfwCreateWindowSurface failed");

    {
        // list out physical devices which are vulkan-compatible
        uint32_t physical_device_count = 0;
        vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
        if (physical_device_count == 0)
            DBG_FAULT("found no valid VkPhysicalDevice");
        vector<VkPhysicalDevice> physical_devices(physical_device_count);
        vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());
        DBG_VERBOSE("found " + ::to_string(physical_device_count) + " physical devices");

        // score each device. a score of zero indicates the device
        // is not usable for some reason
        multimap<int, VkPhysicalDevice> device_scores;
        for (const VkPhysicalDevice& test_device : physical_devices)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(test_device, &properties);
            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(test_device, &features);
            
            // discrete GPUs are preferred, etc
            int score = 0;
            switch (properties.deviceType)
            {
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: score += 1000; break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: score += 200; break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU: score += 100; break;
            default:
                break;
            }

            score += static_cast<int>(properties.limits.maxImageDimension2D);

            if (features.fillModeNonSolid == VK_FALSE
                || features.samplerAnisotropy == VK_FALSE
                || features.independentBlend == VK_FALSE)
                score = 0;

            // check that the necessary queues are present
            auto [graphics_family, present_family] = getQueueFamilies(test_device);
            if (!graphics_family.has_value()
                || !present_family.has_value())
                score = 0;

            uint32_t extension_count;
            vkEnumerateDeviceExtensionProperties(test_device, nullptr, &extension_count, nullptr);
            vector<VkExtensionProperties> available_extensions(extension_count);
            vkEnumerateDeviceExtensionProperties(test_device, nullptr, &extension_count, available_extensions.data());
            set<string> required_extensions_unmet(required_extensions.begin(), required_extensions.end());
            for (const auto& [extension_name, spec_version] : available_extensions)
                required_extensions_unmet.erase(extension_name);
            if (!required_extensions_unmet.empty())
                score = 0;

            if (score != 0)
            {
                auto swapchain_info = Swapchain::getSwapchainSupportInfo(test_device);
                if (swapchain_info.surface_formats.empty())
                    score = 0;
            }

            DBG_BABBLE("found device " + string(properties.deviceName) + ", scored " + ::to_string(score));
            device_scores.insert({ score, test_device });
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
        
        DBG_VERBOSE("creating device");
        if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
            DBG_FAULT("vkCreateDevice failed");

        // extract queues
        DBG_VERBOSE("extracting queues");
        vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
        vkGetDeviceQueue(device, queue_family_indices.present_family.value(), 0, &present_queue);
    }

    Swapchain::getSwapchainSupportInfo(physical_device);
    uint32_t frames_in_flight = Swapchain::computeImageCount();
    DBG_VERBOSE("adjusted frames in flight to " + ::to_string(frames_in_flight));

    {
        array<VkDescriptorPoolSize, 2> descriptor_pool_sizes;
        descriptor_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_pool_sizes[0].descriptorCount = static_cast<uint32_t>(512 * 3 * frames_in_flight);
        descriptor_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptor_pool_sizes[1].descriptorCount = static_cast<uint32_t>(512 * 4 * frames_in_flight);
        VkDescriptorPoolCreateInfo descriptor_pool_create_info{ };
        descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptor_pool_create_info.poolSizeCount = static_cast<uint32_t>(descriptor_pool_sizes.size());
        descriptor_pool_create_info.pPoolSizes = descriptor_pool_sizes.data();
        descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        descriptor_pool_create_info.maxSets = static_cast<uint32_t>(512 * 8 * frames_in_flight);

        DBG_VERBOSE("creating descriptor pool with " + ::to_string(descriptor_pool_create_info.maxSets) + " max sets");
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

        DBG_VERBOSE("creating scene and object descriptor sets");
        if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &scene_descriptor_set_layout) != VK_SUCCESS)
            DBG_FAULT("vkCreateDescriptorSetLayout failed");

        if (vkCreateDescriptorSetLayout(device, &layout_create_info, nullptr, &object_descriptor_set_layout) != VK_SUCCESS)
            DBG_FAULT("vkCreateDescriptorSetLayout failed");
    }

    {
        DBG_VERBOSE("creating command pool and buffers");

        auto [graphics_family, present_family] = getQueueFamilies(physical_device);

        VkCommandPoolCreateInfo pool_create_info{ };
        pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_create_info.queueFamilyIndex = graphics_family.value();

        if (vkCreateCommandPool(device, &pool_create_info, nullptr, &command_pool) != VK_SUCCESS)
            DBG_FAULT("vkCreateCommandPool failed");

        for (uint32_t i = 0; i < static_cast<uint32_t>(frames_in_flight); ++i)
            command_buffers.push_back(new DrawCommandBuffer());
    }
}

void RenderServer::initImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplGlfw_InitForVulkan(window, false);
    ImGui_ImplVulkan_InitInfo init_info{ };
    init_info.ApiVersion = VK_API_VERSION_1_4;
    init_info.Instance = instance;
    init_info.PhysicalDevice = physical_device;
    init_info.Device = device;
    init_info.QueueFamily = getQueueFamilies(physical_device).graphics_family.value();
    init_info.Queue = graphics_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptor_pool;
    Swapchain::getSwapchainSupportInfo(physical_device);
    init_info.MinImageCount = Swapchain::computeImageCount();
    init_info.ImageCount = Swapchain::computeImageCount();
    init_info.Allocator = nullptr;
    init_info.PipelineInfoMain.RenderPass = final_render_pass->getRenderPass();
    init_info.PipelineInfoMain.Subpass = 0;
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

    DBG_VERBOSE("destroying command pool");
    vkDestroyCommandPool(device, command_pool, nullptr);
    
    DBG_VERBOSE("destroying descriptors");
    vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    vkDestroyDescriptorSetLayout(device, scene_descriptor_set_layout, nullptr);
    vkDestroyDescriptorSetLayout(device, object_descriptor_set_layout, nullptr);

#if defined(VK_DEBUG)
    DBG_VERBOSE("\033[31mkilling the (debug) messenger\033[0m");
    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    func(instance, debug_messenger, nullptr);
#endif

    DBG_VERBOSE("destroying device");
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}
