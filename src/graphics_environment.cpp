#include "graphics_environment.h"

#include <stdexcept>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <GLFW/glfw3.h>

#include "swapchain.h"
#include "pipeline.h"
#include "render_pass.h"
#include "shader.h"

using namespace HopEngine;
using namespace std;

static GraphicsEnvironment* environment = nullptr;

GraphicsEnvironment::GraphicsEnvironment(Window* window)
{
    environment = this;

	createInstance();
    if (glfwCreateWindowSurface(instance, window->getWindow(), nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("glfwCreateWindowSurface failed");
    createDevice();

    auto framebuffer_size = window->getSize();
    swapchain = new Swapchain(framebuffer_size.first, framebuffer_size.second, surface);
    render_pass = new RenderPass(swapchain);
    shader = new Shader("shader");
    pipeline = new Pipeline(shader, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, render_pass->getRenderPass());
}

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

GraphicsEnvironment* GraphicsEnvironment::get()
{
    return environment;
}

GraphicsEnvironment::~GraphicsEnvironment()
{
    delete pipeline;
    delete shader;
    delete render_pass;
    delete swapchain;

    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    environment = nullptr;
}

void GraphicsEnvironment::createInstance()
{
    // application info
    VkApplicationInfo app_info{ };
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "HopEngine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "HopEngine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // use extensions required by GLFW
    VkInstanceCreateInfo create_info{ };
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.ppEnabledExtensionNames = glfwGetRequiredInstanceExtensions(&(create_info.enabledExtensionCount));

    // apply validation layers for debug
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
        cout << "validation layer '" << *(required_layers_unmet.begin()) << "' not found." << endl;
        throw runtime_error("missing validation layer");
    }
    create_info.enabledLayerCount = static_cast<uint32_t>(required_validation_layers.size());
    create_info.ppEnabledLayerNames = required_validation_layers.data();
    
    // create the vulkan instance
    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
        throw runtime_error("vkCreateInstance failed");
}

void GraphicsEnvironment::createDevice()
{
    // list out physical devices which are vulkan-compatible
    uint32_t physical_device_count = 0;
    vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
    if (physical_device_count == 0)
        throw runtime_error("found no valid VkPhysicalDevice");
    vector<VkPhysicalDevice> physical_devices(physical_device_count);
    vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices.data());

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

        if (features.fillModeNonSolid == VK_FALSE)
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

        device_scores.insert({ score, device});
    }
    // check if any of the devices were suitable, and if so,
    // use the highest scoring
    if (device_scores.rbegin()->first > 0)
        physical_device = device_scores.rbegin()->second;
    else
        throw runtime_error("unable to find suitable VkPhysicalDevice");

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

    // we aren't using any device features
    VkPhysicalDeviceFeatures features{ };
    features.fillModeNonSolid = VK_TRUE;
    
    // actually create the logical device
    VkDeviceCreateInfo device_create_info{ };
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pEnabledFeatures = &features;
    device_create_info.ppEnabledExtensionNames = required_extensions.data();
    device_create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
    
    if (vkCreateDevice(physical_device, &device_create_info, nullptr, &device) != VK_SUCCESS)
        throw runtime_error("vkCreateDevice failed");

    // extract queues
    vkGetDeviceQueue(device, queue_family_indices.graphics_family.value(), 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices.present_family.value(), 0, &present_queue);
}
