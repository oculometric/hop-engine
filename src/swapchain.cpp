#include "swapchain.h"

#include <limits>
#include <algorithm>
#include <vulkan/vulkan.hpp>

#include "swapchain_vulkan.h"
#include "graphics_environment.h"

using namespace HopEngine;
using namespace std;

SwapchainSupportInfo HopEngine::getSwapchainSupportInfo(const VkPhysicalDevice device, const VkSurfaceKHR surface)
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

VkSurfaceFormatKHR HopEngine::getIdealSurfaceFormat(const SwapchainSupportInfo& info)
{
    for (const VkSurfaceFormatKHR& format : info.surface_formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return info.surface_formats[0];
}

VkPresentModeKHR HopEngine::getIdealPresentMode(const SwapchainSupportInfo& info)
{ return VK_PRESENT_MODE_FIFO_KHR; }

glm::u32vec2 HopEngine::getIdealExtent(const SwapchainSupportInfo& info, const uint32_t window_width, const uint32_t window_height)
{
    if (info.surface_capabilities.currentExtent.width != numeric_limits<uint32_t>::max())
        return { info.surface_capabilities.currentExtent.width, info.surface_capabilities.currentExtent.height };
    else
    {
        VkExtent2D actual_extent =
        {
            window_width,
            window_height
        };

        actual_extent.width = clamp(actual_extent.width, info.surface_capabilities.minImageExtent.width, info.surface_capabilities.maxImageExtent.width);
        actual_extent.height = clamp(actual_extent.height, info.surface_capabilities.minImageExtent.height, info.surface_capabilities.maxImageExtent.height);

        return { actual_extent.width, actual_extent.height };
    }
}

Swapchain::Swapchain(const uint32_t width, const uint32_t height, const VkSurfaceKHR _surface)
{
    surface = _surface;
    create_info.resize(1);

    // calculate actual swapchain parameters
    const SwapchainSupportInfo support_info = getSwapchainSupportInfo(RenderServer::getPhysicalDevice(), surface);
    VkSurfaceFormatKHR surface_format = getIdealSurfaceFormat(support_info);
    format = surface_format.format;
    extent = getIdealExtent(support_info, width, height);

    create_info[0] = VkSwapchainCreateInfoKHR{ };
    create_info[0].sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info[0].surface = surface;
    create_info[0].minImageCount = support_info.surface_capabilities.minImageCount + 1;
    if (support_info.surface_capabilities.maxImageCount > 0)
        create_info[0].minImageCount = min(create_info[0].minImageCount, support_info.surface_capabilities.maxImageCount);
    create_info[0].imageFormat = surface_format.format;
    create_info[0].imageColorSpace = surface_format.colorSpace;
    create_info[0].imageExtent = { extent.x, extent.y };
    create_info[0].imageArrayLayers = 1;
    create_info[0].imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // get info about which present mode we're going to use
    RenderServer::QueueFamilies indices = RenderServer::getQueueFamilies(RenderServer::getPhysicalDevice());
    queue_families[0] = indices.graphics_family.value();
    queue_families[1] = indices.present_family.value();
    if (indices.graphics_family != indices.present_family)
    {
        create_info[0].imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info[0].queueFamilyIndexCount = 2;
        create_info[0].pQueueFamilyIndices = queue_families;
    }
    else
    {
        create_info[0].imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info[0].queueFamilyIndexCount = 0;
        create_info[0].pQueueFamilyIndices = nullptr;
    }

    create_info[0].preTransform = support_info.surface_capabilities.currentTransform;
    create_info[0].compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info[0].presentMode = getIdealPresentMode(support_info);
    create_info[0].clipped = VK_TRUE;
    create_info[0].oldSwapchain = VK_NULL_HANDLE;

    // create the swapchain
    if (vkCreateSwapchainKHR(RenderServer::getDevice(), create_info.data(), nullptr, &swapchain) != VK_SUCCESS)
        DBG_FAULT("vkCreateSwapchainKHR failed");
    createImageViews();

    DBG_INFO("created swapchain at " + to_string(width) + "x" + to_string(height) + " with " + to_string(images.size()) + " images in present mode " + vk::to_string(static_cast<vk::PresentModeKHR>(create_info[0].presentMode)));
}

Swapchain::~Swapchain()
{
    DBG_VERBOSE("destroying swapchain " + PTR(this));
    destroyResources();
}

void Swapchain::resize(const uint32_t width, const uint32_t height)
{
    DBG_VERBOSE("resizing swapchain to " + to_string(width) + "x" + to_string(height));
    destroyResources();

    const SwapchainSupportInfo support_info = getSwapchainSupportInfo(RenderServer::getPhysicalDevice(), surface);
    extent = getIdealExtent(support_info, width, height);
    create_info[0].imageExtent = { extent.x, extent.y };

    if (vkCreateSwapchainKHR(RenderServer::getDevice(), create_info.data(), nullptr, &swapchain) != VK_SUCCESS)
        DBG_FAULT("vkCreateSwapchainKHR failed");
    createImageViews();
}

void Swapchain::createImageViews()
{
    // retrieve images
    uint32_t image_count = 0;
    vkGetSwapchainImagesKHR(RenderServer::getDevice(), swapchain, &image_count, nullptr);
    images.resize(image_count);
    vkGetSwapchainImagesKHR(RenderServer::getDevice(), swapchain, &image_count, images.data());

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

        if (vkCreateImageView(RenderServer::getDevice(), &view_create_info, nullptr, &image_views[i]) != VK_SUCCESS)
            DBG_FAULT("vkCreateImageView failed");
    }
}

void Swapchain::destroyResources() const
{
    for (const auto image_view : image_views)
        vkDestroyImageView(RenderServer::getDevice(), image_view, nullptr);
    vkDestroySwapchainKHR(RenderServer::getDevice(), swapchain, nullptr);
}
