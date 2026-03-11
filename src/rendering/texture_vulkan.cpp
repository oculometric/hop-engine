#include "texture.h"

#include <vulkan/vulkan.hpp>

#include "buffer.h"
#include "render_server.h"
#include "command_buffer.h"
#include "texture_vulkan.h"

using namespace HopEngine;
using namespace std;

constexpr VkFormat vulkan_image_format[4] = 
{
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_B8G8R8A8_SRGB,
};

VkFormat HopEngine::toVulkanFormat(Texture::Format format) { return vulkan_image_format[format]; }

constexpr VkImageLayout vulkan_image_layout[8] = 
{
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
};

VkImageLayout HopEngine::toVulkanLayout(Texture::Layout layout) { return vulkan_image_layout[layout]; }

VkImageView Texture::getView(const bool stencil)
{
    if (!stencil && view != VK_NULL_HANDLE)
        return view;
    if (stencil && stencil_view != VK_NULL_HANDLE)
        return stencil_view;

    DBG_BABBLE("creating image view for image '" + getOrigin() + '\'');

    VkImageViewCreateInfo view_create_info{ };
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image;
    view_create_info.viewType = extent.z == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
    view_create_info.format = toVulkanFormat(format);
    if (format == FORMAT_D32_SFLOAT_S8_UINT)
        view_create_info.subresourceRange.aspectMask = stencil ? VK_IMAGE_ASPECT_STENCIL_BIT : VK_IMAGE_ASPECT_DEPTH_BIT;
    else
        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(RenderServer::getDevice(), &view_create_info, nullptr, stencil ? &stencil_view : &view) != VK_SUCCESS)
        DBG_ERROR("vkCreateImageView failed");

    return stencil ? stencil_view : view;
}

void Texture::transitionLayout(const Layout new_layout)
{
    DBG_BABBLE("transitioning image '" + getOrigin() + "' layout from " + vk::to_string((vk::ImageLayout)current_layout) + " to " + vk::to_string((vk::ImageLayout)new_layout));

    VkImageMemoryBarrier memory_barrier{ };
    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.oldLayout = toVulkanLayout(current_layout);
    memory_barrier.newLayout = toVulkanLayout(new_layout);
    memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.image = image;
    memory_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    memory_barrier.subresourceRange.baseMipLevel = 0;
    memory_barrier.subresourceRange.levelCount = 1;
    memory_barrier.subresourceRange.baseArrayLayer = 0;
    memory_barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags dst_stage;
    VkPipelineStageFlags src_stage;
    if (current_layout == LAYOUT_UNDEFINED && new_layout == LAYOUT_TRANSFER_DST)
    {
        memory_barrier.srcAccessMask = 0;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (current_layout == LAYOUT_TRANSFER_DST && new_layout == LAYOUT_SHADER_READ_ONLY)
    {
        memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        DBG_ERROR("unsupported layout transition!");
        return;
    }

    Ref<TransientCommandBuffer> cmd_buf = new TransientCommandBuffer();

    vkCmdPipelineBarrier(cmd_buf->getHandle(), src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &memory_barrier);

    cmd_buf->submit();
    current_layout = new_layout;
}

void Texture::copyBufferToImage(Ref<Buffer> buffer) const
{
    DBG_BABBLE("copying buffer " + PTR(buffer.get()) + " to image '" + getOrigin() + '\'');

    VkBufferImageCopy image_copy{ };
    image_copy.bufferOffset = 0;
    image_copy.bufferRowLength = 0;
    image_copy.bufferImageHeight = 0;

    image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.imageSubresource.mipLevel = 0;
    image_copy.imageSubresource.baseArrayLayer = 0;
    image_copy.imageSubresource.layerCount = 1;

    image_copy.imageOffset = { 0, 0, 0 };
    image_copy.imageExtent = { extent.x, extent.y, extent.z };

    Ref<TransientCommandBuffer> cmd_buf = new TransientCommandBuffer();

    vkCmdCopyBufferToImage(cmd_buf->getHandle(), buffer->getHandle(), image, toVulkanLayout(current_layout), 1, &image_copy);

    cmd_buf->submit();
}

static constexpr VkImageUsageFlagBits vulkan_image_usage[5] = 
{
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
};

void Texture::createImage()
{
    VkImageCreateInfo image_create_info{ };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = extent.z == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    image_create_info.extent = { extent.x, extent.y, extent.z };
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = toVulkanFormat(format);
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (usage == IMAGE_USAGE_DEFAULT)
    {
        if (format == Texture::getDepthFormat())
            usage = IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT;
        else if (format == Texture::getDataFormat())
            usage = IMAGE_USAGE_COLOR_ATTACHMENT;
        else
            usage = IMAGE_USAGE_TRANSFER_DST | IMAGE_USAGE_SAMPLED;
    }
    image_create_info.usage = convertFlags<VkImageUsageFlagBits, Usage, 5>(usage, vulkan_image_usage);
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    if (vkCreateImage(RenderServer::getDevice(), &image_create_info, nullptr, &image) != VK_SUCCESS)
        DBG_FAULT("vkCreateImage failed");
    current_layout = LAYOUT_UNDEFINED;

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(RenderServer::getDevice(), image, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = Buffer::findMemoryType(memory_requirements.memoryTypeBits, MEMORY_PROPERTY_DEVICE_LOCAL);
    if (vkAllocateMemory(RenderServer::getDevice(), &allocate_info, nullptr, &memory) != VK_SUCCESS)
        DBG_FAULT("vkAllocateMemory failed");
    vkBindImageMemory(RenderServer::getDevice(), image, memory, 0);
}

void Texture::destroyResources()
{
    RenderServer::waitIdle();
    if (view != VK_NULL_HANDLE)
        vkDestroyImageView(RenderServer::getDevice(), view, nullptr);
    if (stencil_view != VK_NULL_HANDLE)
        vkDestroyImageView(RenderServer::getDevice(), stencil_view, nullptr);
    vkDestroyImage(RenderServer::getDevice(), image, nullptr);
    vkFreeMemory(RenderServer::getDevice(), memory, nullptr);
}
