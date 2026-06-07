#include "command_buffer.h"
#include "framebuffer.h"
#include "render_server.h"
#include "vulkan_helpers.h"
#include "window.h"

#include <algorithm>
#include <vulkan/vulkan.hpp>

using namespace HopEngine;

Swapchain::SupportInfo Swapchain::getSwapchainSupportInfo(const GPUHandle device)
{
    SupportInfo si{};

    VkSurfaceCapabilitiesKHR capabilities{};
    CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR,
        (static_cast<VkPhysicalDevice>(device), static_cast<VkSurfaceKHR>(Window::getSurface()),
            &capabilities),
        FAULT,
        ;);
    si.current_extent    = { capabilities.currentExtent.width, capabilities.currentExtent.height };
    si.min_extent        = { capabilities.minImageExtent.width, capabilities.minImageExtent.height };
    si.max_extent        = { capabilities.maxImageExtent.width, capabilities.maxImageExtent.height };
    si.min_image_count   = capabilities.minImageCount;
    si.max_image_count   = capabilities.maxImageCount;
    si.current_transform = capabilities.currentTransform;
    si.supports_premultiplied_alpha_composite =
        (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR);

    uint32_t present_mode_count;
    CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR,
        (static_cast<VkPhysicalDevice>(device), static_cast<VkSurfaceKHR>(Window::getSurface()),
            &present_mode_count, nullptr),
        FAULT,
        ;);
    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(static_cast<VkPhysicalDevice>(device),
        static_cast<VkSurfaceKHR>(Window::getSurface()), &present_mode_count, present_modes.data());
    si.supports_immediate_present = false;
    for (auto mode : present_modes)
    {
        if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) si.supports_immediate_present = true;
    }

    return si;
}

glm::u32vec2 Swapchain::computeActualExtent(const glm::u32vec2 extent)
{
    SupportInfo si = getSwapchainSupportInfo(RenderServer::getPhysicalDevice());
    if (si.current_extent.x != UINT32_MAX) return si.current_extent;
    else
        return glm::clamp(extent, si.min_extent, si.max_extent);
}

uint32_t Swapchain::computeImageCount()
{
    SupportInfo si       = getSwapchainSupportInfo(RenderServer::getPhysicalDevice());
    uint32_t image_count = si.min_image_count + 1;
    if (si.max_image_count > 0) image_count = glm::min(image_count, si.max_image_count);
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
    framebuffer = new Framebuffer(this, Framebuffer::getSwapchainConfig());

    DBG_INFO("created swapchain at " + std::to_string(new_extent.x) + "x" + std::to_string(new_extent.y) +
             " with " + std::to_string(images.size()) + " images");
}

Swapchain::~Swapchain()
{
    DBG_VERBOSE("destroying swapchain " + PTR(this));
    destroyResources();
}

uint32_t Swapchain::acquireNextImage()
{
    ++frame_index;

    {
        VkResult _result = vkWaitForFences(static_cast<VkDevice>(RenderServer::getDevice()), 1,
            reinterpret_cast<VkFence*>(&in_flight_fences[frame_index % in_flight_fences.size()]), VK_TRUE,
            10000000);
        if (_result == VK_TIMEOUT) return UINT32_MAX;
        else if (_result != VK_SUCCESS)
        {
            DBG_ERROR("vkWaitForFences failed with error " + vk::to_string((vk::Result)_result));
            return UINT32_MAX;
        }
    }
    CHECK_RESULT(vkResetFences,
        (static_cast<VkDevice>(RenderServer::getDevice()), 1,
            reinterpret_cast<VkFence*>(&in_flight_fences[frame_index % in_flight_fences.size()])),
        ERROR, return UINT32_MAX);

    uint32_t image_index;
    CHECK_RESULT(vkAcquireNextImageKHR,
        (static_cast<VkDevice>(RenderServer::getDevice()), static_cast<VkSwapchainKHR>(swapchain),
            UINT64_MAX,
            static_cast<VkSemaphore>(image_available_semaphores[frame_index % in_flight_fences.size()]),
            VK_NULL_HANDLE, &image_index),
        WARNING, return UINT32_MAX);
    // DBG_VERBOSE("acquired image " + std::to_string(image_index));

    return image_index;
}

bool Swapchain::submitCommands(WeakRef<DrawCommandBuffer> command_buffer, uint32_t image_index)
{
    VkSubmitInfo submit_info{};
    submit_info.sType                            = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    const VkSemaphore wait_semaphores[]          = { static_cast<VkSemaphore>(
        image_available_semaphores[frame_index % image_available_semaphores.size()]) };
    constexpr VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submit_info.waitSemaphoreCount               = 1;
    submit_info.pWaitSemaphores                  = wait_semaphores;
    submit_info.pWaitDstStageMask                = wait_stages;
    submit_info.commandBufferCount               = 1;
    VkCommandBuffer cmd_buf     = static_cast<VkCommandBuffer>(command_buffer->getCommandBuffer());
    submit_info.pCommandBuffers = &cmd_buf;
    const VkSemaphore signal_semaphores[] = { static_cast<VkSemaphore>(
        render_finished_semaphores[image_index]) };
    submit_info.signalSemaphoreCount      = 1;
    submit_info.pSignalSemaphores         = signal_semaphores;
    CHECK_RESULT(vkQueueSubmit,
        (static_cast<VkQueue>(RenderServer::getGraphicsQueue()), 1, &submit_info,
            static_cast<VkFence>(in_flight_fences[frame_index % in_flight_fences.size()])),
        ERROR, return false);

    VkPresentInfoKHR present_info{};
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores    = signal_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = reinterpret_cast<VkSwapchainKHR*>(&swapchain);
    present_info.pImageIndices      = &image_index;
    CHECK_RESULT(vkQueuePresentKHR, (static_cast<VkQueue>(RenderServer::getPresentQueue()), &present_info),
        ERROR, return false);

    return true;
}

void Swapchain::resize(const glm::u32vec2 new_extent)
{
    DBG_VERBOSE(
        "resizing swapchain to " + std::to_string(new_extent.x) + "x" + std::to_string(new_extent.y));

    destroyResources();

    extent = computeActualExtent(new_extent);

    createSwapchain();
    createImageViews();
    createSyncObjects();
    framebuffer->resize(getExtent());
}

void Swapchain::setVsync(bool enabled)
{
    if (vsync_enabled == enabled) return;

    destroyResources();

    vsync_enabled = enabled;

    createSwapchain();
    createImageViews();
    createSyncObjects();
    framebuffer = new Framebuffer(this, Framebuffer::getSwapchainConfig());
}

void Swapchain::createSwapchain()
{
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface          = static_cast<VkSurfaceKHR>(Window::getSurface());
    create_info.minImageCount    = computeImageCount();
    create_info.imageFormat      = toVulkanFormat(format);
    create_info.imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    create_info.imageExtent      = { extent.x, extent.y };
    create_info.imageArrayLayers = 1;
    create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    auto queue_families          = RenderServer::getUniqueQueueIndices();
    if (queue_families.size() > 1)
    {
        create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices   = queue_families.data();
    }
    else
    {
        create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices   = nullptr;
    }
    SupportInfo si             = Swapchain::getSwapchainSupportInfo(RenderServer::getPhysicalDevice());
    create_info.preTransform   = static_cast<VkSurfaceTransformFlagBitsKHR>(si.current_transform);
    create_info.compositeAlpha = si.supports_premultiplied_alpha_composite
                                     ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR
                                     : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode    = (!vsync_enabled && si.supports_immediate_present)
                                     ? VK_PRESENT_MODE_IMMEDIATE_KHR
                                     : VK_PRESENT_MODE_FIFO_KHR;
    create_info.clipped        = VK_TRUE;
    create_info.oldSwapchain   = VK_NULL_HANDLE;

    CHECK_RESULT(vkCreateSwapchainKHR,
        (static_cast<VkDevice>(RenderServer::getDevice()), &create_info, nullptr,
            reinterpret_cast<VkSwapchainKHR*>(&swapchain)),
        FAULT,
        ;);
}

void Swapchain::createImageViews()
{
    // retrieve images
    uint32_t image_count = 0;
    CHECK_RESULT(vkGetSwapchainImagesKHR,
        (static_cast<VkDevice>(RenderServer::getDevice()), static_cast<VkSwapchainKHR>(swapchain),
            &image_count, nullptr),
        FAULT,
        ;);
    images.resize(image_count);
    vkGetSwapchainImagesKHR(static_cast<VkDevice>(RenderServer::getDevice()),
        static_cast<VkSwapchainKHR>(swapchain), &image_count, reinterpret_cast<VkImage*>(images.data()));

    // create image views
    image_views.resize(image_count);
    for (size_t i = 0; i < image_views.size(); ++i)
    {
        VkImageViewCreateInfo view_create_info{};
        view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_create_info.image                           = static_cast<VkImage>(images[i]);
        view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        view_create_info.format                          = toVulkanFormat(format);
        view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        view_create_info.subresourceRange.baseMipLevel   = 0;
        view_create_info.subresourceRange.levelCount     = 1;
        view_create_info.subresourceRange.baseArrayLayer = 0;
        view_create_info.subresourceRange.layerCount     = 1;

        CHECK_RESULT(vkCreateImageView,
            (static_cast<VkDevice>(RenderServer::getDevice()), &view_create_info, nullptr,
                reinterpret_cast<VkImageView*>(&image_views[i])),
            FAULT,
            ;);
    }
}

void Swapchain::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    image_available_semaphores.resize(images.size());
    render_finished_semaphores.resize(images.size());
    in_flight_fences.resize(images.size());

    for (size_t i = 0; i < images.size(); ++i)
    {
        CHECK_RESULT(vkCreateSemaphore,
            (static_cast<VkDevice>(RenderServer::getDevice()), &semaphore_create_info, nullptr,
                reinterpret_cast<VkSemaphore*>(&image_available_semaphores[i])),
            FAULT,
            ;);
        CHECK_RESULT(vkCreateSemaphore,
            (static_cast<VkDevice>(RenderServer::getDevice()), &semaphore_create_info, nullptr,
                reinterpret_cast<VkSemaphore*>(&render_finished_semaphores[i])),
            FAULT,
            ;);
        CHECK_RESULT(vkCreateFence,
            (static_cast<VkDevice>(RenderServer::getDevice()), &fence_create_info, nullptr,
                reinterpret_cast<VkFence*>(&in_flight_fences[i])),
            FAULT,
            ;);
    }
}

void Swapchain::destroyResources()
{
    RenderServer::waitIdle();
    for (const auto image_view : image_views)
        vkDestroyImageView(static_cast<VkDevice>(RenderServer::getDevice()),
            static_cast<VkImageView>(image_view), nullptr);
    image_views.clear();
    for (size_t i = 0; i < image_available_semaphores.size(); ++i)
    {
        vkDestroySemaphore(static_cast<VkDevice>(RenderServer::getDevice()),
            static_cast<VkSemaphore>(image_available_semaphores[i]), nullptr);
        vkDestroySemaphore(static_cast<VkDevice>(RenderServer::getDevice()),
            static_cast<VkSemaphore>(render_finished_semaphores[i]), nullptr);
        vkDestroyFence(static_cast<VkDevice>(RenderServer::getDevice()),
            static_cast<VkFence>(in_flight_fences[i]), nullptr);
    }
    image_available_semaphores.clear();
    render_finished_semaphores.clear();
    in_flight_fences.clear();
    vkDestroySwapchainKHR(static_cast<VkDevice>(RenderServer::getDevice()),
        static_cast<VkSwapchainKHR>(swapchain), nullptr);
    swapchain = nullptr;
}
