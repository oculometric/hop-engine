#include "swapchain.h"

#include <limits>
#include <algorithm>
#include <vulkan/vulkan.hpp>

#include "render_server.h"
#include "vulkan_helpers.h"
#include "command_buffer.h"

using namespace HopEngine;
using namespace std;

Swapchain::SupportInfo Swapchain::getSwapchainSupportInfo(const VkPhysicalDevice device)
{
    SupportInfo si;
    si.surface_capabilities.resize(1);

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, RenderServer::getSurface(), si.surface_capabilities.data());

    uint32_t format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, RenderServer::getSurface(), &format_count, nullptr);
    if (format_count != 0)
    {
        si.surface_formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, RenderServer::getSurface(), &format_count, si.surface_formats.data());
    }

    return si;
}

glm::u32vec2 Swapchain::computeActualExtent(const glm::u32vec2 extent)
{
    SupportInfo si = getSwapchainSupportInfo(RenderServer::getPhysicalDevice());
    if (si.surface_capabilities[0].currentExtent.width != numeric_limits<uint32_t>::max())
        return { si.surface_capabilities[0].currentExtent.width, si.surface_capabilities[0].currentExtent.height };
    else
    {
        glm::u32vec2 actual_extent = glm::clamp(extent,
            { si.surface_capabilities[0].minImageExtent.width, si.surface_capabilities[0].minImageExtent.height },
            { si.surface_capabilities[0].maxImageExtent.width, si.surface_capabilities[0].maxImageExtent.height });

        return actual_extent;
    }
}

uint32_t HopEngine::Swapchain::computeImageCount()
{
    SupportInfo si = getSwapchainSupportInfo(RenderServer::getPhysicalDevice());
    uint32_t image_count = si.surface_capabilities[0].minImageCount + 1;
    if (si.surface_capabilities[0].maxImageCount > 0)
        image_count = min(image_count, si.surface_capabilities[0].maxImageCount);
    return image_count;
}

Swapchain::Swapchain(glm::u32vec2 new_extent)
{
    // calculate actual swapchain parameters
    format = Texture::FORMAT_SWAPCHAIN;
    extent = computeActualExtent(new_extent);

    createSwapchain();    
    createImageViews();
    createSyncObjects();

    DBG_INFO("created swapchain at " + ::to_string(new_extent.x) + "x" + ::to_string(new_extent.y) + " with " + ::to_string(images.size()) + " images");
}

Swapchain::~Swapchain()
{
    DBG_VERBOSE("destroying swapchain " + PTR(this));
    destroyResources();
}

uint32_t Swapchain::acquireNextImage()
{
    ++frame_index;

    CHECK_RESULT(
        vkWaitForFences, (RenderServer::getDevice(), 1, &in_flight_fences[frame_index % in_flight_fences.size()], VK_TRUE, 10000000),
        WARNING,
        return -1);
    CHECK_RESULT(
        vkResetFences, (RenderServer::getDevice(), 1, &in_flight_fences[frame_index % in_flight_fences.size()]),
        ERROR,
        return -1);

    uint32_t image_index;
    CHECK_RESULT(
        vkAcquireNextImageKHR, (RenderServer::getDevice(), swapchain, UINT64_MAX, image_available_semaphores[frame_index % in_flight_fences.size()], VK_NULL_HANDLE, &image_index),
        WARNING,
        return -1);
    DBG_BABBLE("acquired image " + ::to_string(image_index));

    return image_index;
}

bool Swapchain::submitCommands(WeakRef<DrawCommandBuffer> command_buffer, uint32_t image_index)
{
    VkSubmitInfo submit_info{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    const VkSemaphore wait_semaphores[] = { image_available_semaphores[frame_index % image_available_semaphores.size()] };
    constexpr VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = 1;
    VkCommandBuffer cmd_buf = command_buffer->getCommandBuffer();
    submit_info.pCommandBuffers = &cmd_buf;
    const VkSemaphore signal_semaphores[] = { render_finished_semaphores[image_index] };
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;
    CHECK_RESULT(
        vkQueueSubmit, (RenderServer::getGraphicsQueue(), 1, &submit_info, in_flight_fences[frame_index % in_flight_fences.size()]),
        ERROR, return false);

    VkPresentInfoKHR present_info{ };
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;
    CHECK_RESULT(
        vkQueuePresentKHR, (RenderServer::getPresentQueue(), &present_info),
        ERROR, return false);

    return true;
}

void Swapchain::resize(const glm::u32vec2 new_extent)
{
    DBG_VERBOSE("resizing swapchain to " + ::to_string(new_extent.x) + "x" + ::to_string(new_extent.y));
    
    destroyResources();

    extent = computeActualExtent(new_extent);

    createSwapchain();
    createImageViews();
    createSyncObjects();
}

void Swapchain::setVsync(bool enabled)
{
    if (vsync_enabled == enabled)
        return;

    destroyResources();

    vsync_enabled = enabled;

    createSwapchain();
    createImageViews();
    createSyncObjects();
}

bool Swapchain::getVsync()
{
    return vsync_enabled;
}

void Swapchain::createSwapchain()
{
    VkSwapchainCreateInfoKHR create_info{ };
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = RenderServer::getSurface();
    create_info.minImageCount = computeImageCount();
    create_info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    create_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    create_info.imageExtent = { extent.x, extent.y };
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    RenderServer::QueueFamilies indices = RenderServer::getQueueFamilies(RenderServer::getPhysicalDevice());
    uint32_t queue_families[2] = { 0 };
    queue_families[0] = indices.graphics_family.value();
    queue_families[1] = indices.present_family.value();
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
    SupportInfo si = Swapchain::getSwapchainSupportInfo(RenderServer::getPhysicalDevice());
    create_info.preTransform = si.surface_capabilities[0].currentTransform;
    if (si.surface_capabilities[0].supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
    else if (si.surface_capabilities[0].supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR)
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
    else
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = vsync_enabled ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    CHECK_RESULT(
        vkCreateSwapchainKHR, (RenderServer::getDevice(), &create_info, nullptr, &swapchain),
        FAULT, ;);
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
        view_create_info.format = toVulkanFormat(format);
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

void Swapchain::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphore_create_info{ };
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_create_info{ };
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    image_available_semaphores.resize(images.size());
    render_finished_semaphores.resize(images.size());
    in_flight_fences.resize(images.size());

    for (size_t i = 0; i < images.size(); ++i)
    {
        CHECK_RESULT(
            vkCreateSemaphore, (RenderServer::getDevice(), &semaphore_create_info, nullptr, &image_available_semaphores[i]),
            FAULT, ;);
        CHECK_RESULT(
            vkCreateSemaphore, (RenderServer::getDevice(), &semaphore_create_info, nullptr, &render_finished_semaphores[i]),
            FAULT, ;);
        CHECK_RESULT(
            vkCreateFence, (RenderServer::getDevice(), &fence_create_info, nullptr, &in_flight_fences[i]),
            FAULT, ;);
    }
}

void Swapchain::destroyResources()
{
    RenderServer::waitIdle();
    for (const auto image_view : image_views)
        vkDestroyImageView(RenderServer::getDevice(), image_view, nullptr);
    image_views.clear();
    for (size_t i = 0; i < image_available_semaphores.size(); ++i)
    {
        vkDestroySemaphore(RenderServer::getDevice(), image_available_semaphores[i], nullptr);
        vkDestroySemaphore(RenderServer::getDevice(), render_finished_semaphores[i], nullptr);
        vkDestroyFence(RenderServer::getDevice(), in_flight_fences[i], nullptr);
    }
    image_available_semaphores.clear();
    render_finished_semaphores.clear();
    in_flight_fences.clear();
    vkDestroySwapchainKHR(RenderServer::getDevice(), swapchain, nullptr);
}
