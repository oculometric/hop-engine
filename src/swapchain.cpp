#include "swapchain.h"

#include <limits>
#include <algorithm>
#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "graphics_environment.h"

using namespace HopEngine;
using namespace std;

Swapchain::Swapchain(uint32_t width, uint32_t height, VkSurfaceKHR surface)
{
    // calculate actual swapchain parameters
    const SwapchainSupportInfo support_info = Swapchain::getSupportInfo(GraphicsEnvironment::get()->getPhysicalDevice(), surface);
    VkSurfaceFormatKHR surface_format = Swapchain::getIdealSurfaceFormat(support_info);
    format = surface_format.format;
    extent = Swapchain::getIdealExtent(support_info, width, height);

    VkSwapchainCreateInfoKHR create_info{ };
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = support_info.surface_capabilities.minImageCount + 1;
    if (support_info.surface_capabilities.maxImageCount > 0)
        create_info.minImageCount = min(create_info.minImageCount, support_info.surface_capabilities.maxImageCount);
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // get info about which present mode we're going to use
    GraphicsEnvironment::QueueFamilies indices = GraphicsEnvironment::get()->getQueueFamilies(GraphicsEnvironment::get()->getPhysicalDevice());
    uint32_t queue_families[] = { indices.graphics_family.value(), indices.present_family.value() };
    if (indices.graphics_family != indices.present_family)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_families;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = support_info.surface_capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = getIdealPresentMode(support_info);
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    // create the swapchain
    if (vkCreateSwapchainKHR(GraphicsEnvironment::get()->getDevice(), &create_info, nullptr, &swapchain) != VK_SUCCESS)
        DBG_FAULT("vkCreateSwapchainKHR failed");

    // retreive images
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(GraphicsEnvironment::get()->getDevice(), swapchain, &image_count, nullptr);
    images.resize(image_count);
    vkGetSwapchainImagesKHR(GraphicsEnvironment::get()->getDevice(), swapchain, &image_count, images.data());

    // create image views
    image_views.resize(image_count);
    for (size_t i = 0; i < image_views.size(); ++i)
    {
        VkImageViewCreateInfo view_create_info{ };
        view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_create_info.image = images[i];
        view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_create_info.format = format;
        view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_create_info.subresourceRange.baseMipLevel = 0;
        view_create_info.subresourceRange.levelCount = 1;
        view_create_info.subresourceRange.baseArrayLayer = 0;
        view_create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(GraphicsEnvironment::get()->getDevice(), &view_create_info, nullptr, &image_views[i]) != VK_SUCCESS)
            DBG_FAULT("vkCreateImageView failed");
    }

    DBG_INFO("created swapchain at " + to_string(width) + "x" + to_string(height) + " with " + to_string(image_count) + " images in present mode " + string_VkPresentModeKHR(create_info.presentMode));
}

Swapchain::~Swapchain()
{
    DBG_INFO("destroying swapchain " + to_string((size_t)this));
    for (auto image_view : image_views)
        vkDestroyImageView(GraphicsEnvironment::get()->getDevice(), image_view, nullptr);
    
    vkDestroySwapchainKHR(GraphicsEnvironment::get()->getDevice(), swapchain, nullptr);
}

SwapchainSupportInfo Swapchain::getSupportInfo(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapchainSupportInfo info;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &(info.surface_capabilities));
   
    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);
    if (format_count != 0)
    {
        info.surface_formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, info.surface_formats.data());
    }

    uint32_t mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count, nullptr);
    if (mode_count != 0)
    {
        info.present_modes.resize(mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &mode_count, info.present_modes.data());
    }

    return info;
}

VkSurfaceFormatKHR Swapchain::getIdealSurfaceFormat(const SwapchainSupportInfo& info)
{
    for (const VkSurfaceFormatKHR& format : info.surface_formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return info.surface_formats[0];
}

VkPresentModeKHR Swapchain::getIdealPresentMode(const SwapchainSupportInfo& info)
{
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::getIdealExtent(const SwapchainSupportInfo& info, uint32_t window_width, uint32_t window_height)
{
    if (info.surface_capabilities.currentExtent.width != numeric_limits<uint32_t>::max())
        return info.surface_capabilities.currentExtent;
    else
    {
        VkExtent2D actual_extent =
        {
            window_width,
            window_height
        };

        actual_extent.width = clamp(actual_extent.width, info.surface_capabilities.minImageExtent.width, info.surface_capabilities.maxImageExtent.width);
        actual_extent.height = clamp(actual_extent.height, info.surface_capabilities.minImageExtent.height, info.surface_capabilities.maxImageExtent.height);

        return actual_extent;
    }
}
