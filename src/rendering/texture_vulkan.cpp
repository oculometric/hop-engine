#include "buffer.h"
#include "command_buffer.h"
#include "render_server.h"
#include "texture.h"
#include "vulkan_helpers.h"

#include <vulkan/vulkan.hpp>

using namespace HopEngine;

constexpr VkFormat vulkan_image_format[4] = {
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_R16G16B16A16_SFLOAT,
    VK_FORMAT_B8G8R8A8_SRGB,
};

VkFormat HopEngine::toVulkanFormat(Texture::Format format) { return vulkan_image_format[format]; }

constexpr VkImageLayout vulkan_image_layout[8] = { VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL };

VkImageLayout HopEngine::toVulkanLayout(Texture::Layout layout) { return vulkan_image_layout[layout]; }

void Texture::transitionLayout(const Layout new_layout)
{
    DBG_VERBOSE("transitioning image '" + getOrigin() + "' layout from " +
                vk::to_string((vk::ImageLayout)current_layout) + " to " +
                vk::to_string((vk::ImageLayout)new_layout));

    VkImageMemoryBarrier memory_barrier{};
    memory_barrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.oldLayout           = toVulkanLayout(current_layout);
    memory_barrier.newLayout           = toVulkanLayout(new_layout);
    memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    memory_barrier.image               = static_cast<VkImage>(image);
    memory_barrier.subresourceRange.aspectMask =
        format == FORMAT_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    memory_barrier.subresourceRange.baseMipLevel   = 0;
    memory_barrier.subresourceRange.levelCount     = 1;
    memory_barrier.subresourceRange.baseArrayLayer = 0;
    memory_barrier.subresourceRange.layerCount     = 1;

    VkPipelineStageFlags dst_stage;
    VkPipelineStageFlags src_stage;
    if (current_layout == LAYOUT_UNDEFINED && new_layout == LAYOUT_TRANSFER_DST)
    {
        if (!(usage & IMAGE_USAGE_READABLE))
            DBG_WARNING(
                "attempt to transition an image to transfer destination layout, but it does not support being a transfer destination");
        memory_barrier.srcAccessMask = 0;
        src_stage                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_stage                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (current_layout == LAYOUT_TRANSFER_DST && new_layout == LAYOUT_SHADER_READ)
    {
        if (!(usage & IMAGE_USAGE_SHADER))
            DBG_WARNING(
                "attempt to transition an image to shader read only layout, but it does not support being sampled");
        memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dst_stage                    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (current_layout == LAYOUT_UNDEFINED && new_layout == LAYOUT_SHADER_READ)
    {
        if (!(usage & IMAGE_USAGE_SHADER))
            DBG_WARNING(
                "attempt to transition an image to shader read only layout, but it does not support being sampled");
        memory_barrier.srcAccessMask = 0;
        src_stage                    = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dst_stage                    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (current_layout == LAYOUT_SHADER_READ && new_layout == LAYOUT_TRANSFER_SRC)
    {
        if (!(usage & IMAGE_USAGE_WRITEABLE))
            DBG_WARNING(
                "attempt to transition an image to transfer source layout, but it does not support being a transfer source");
        memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        src_stage                    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        dst_stage                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (current_layout == LAYOUT_TRANSFER_SRC && new_layout == LAYOUT_SHADER_READ)
    {
        if (!(usage & IMAGE_USAGE_SHADER))
            DBG_WARNING(
                "attempt to transition an image to shader read only layout, but it does not support being sampled");
        memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        src_stage                    = VK_PIPELINE_STAGE_TRANSFER_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dst_stage                    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        DBG_ERROR(
            "unsupported layout transition: " + to_string(current_layout) + " -> " + to_string(new_layout));
        return;
    }

    Ref<TransientCommandBuffer> cmd_buf = new TransientCommandBuffer();

    vkCmdPipelineBarrier(static_cast<VkCommandBuffer>(cmd_buf->getHandle()), src_stage, dst_stage, 0, 0,
        nullptr, 0, nullptr, 1, &memory_barrier);

    cmd_buf->submit();
    current_layout = new_layout;
}

DataBlock Texture::download()
{
    current_layout                  = Texture::LAYOUT_SHADER_READ; // assume the position
    Texture::Layout previous_layout = current_layout;
    transitionLayout(Texture::LAYOUT_TRANSFER_SRC);
    Ref<Buffer> buffer = new Buffer(allocated_size, Buffer::BUFFER_USAGE_TRANSFER_DST,
        MemoryProperties::MEMORY_PROPERTY_HOST_COHERENT | MemoryProperties::MEMORY_PROPERTY_HOST_VISIBLE);

    VkBufferImageCopy image_copy{};
    image_copy.bufferOffset      = 0;
    image_copy.bufferRowLength   = 0;
    image_copy.bufferImageHeight = 0;

    image_copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.imageSubresource.mipLevel       = 0;
    image_copy.imageSubresource.baseArrayLayer = 0;
    image_copy.imageSubresource.layerCount     = 1;

    image_copy.imageOffset = { 0, 0, 0 };
    image_copy.imageExtent = { extent.x, extent.y, extent.z };

    Ref<TransientCommandBuffer> cmd_buf = new TransientCommandBuffer();

    vkCmdCopyImageToBuffer(static_cast<VkCommandBuffer>(cmd_buf->getHandle()), static_cast<VkImage>(image),
        toVulkanLayout(current_layout), static_cast<VkBuffer>(buffer->getHandle()), 1, &image_copy);

    cmd_buf->submit();

    transitionLayout(previous_layout);
    DataBlock data;
    data.resize(buffer->getSize());
    memcpy(data.data(), buffer->mapMemory(), data.size());
    return data;
}

void Texture::copyBufferToImage(Ref<Buffer> buffer) const
{
    DBG_VERBOSE("copying buffer " + PTR(buffer.get()) + " to image '" + getOrigin() + '\'');

    if (!(usage & IMAGE_USAGE_READABLE))
        DBG_WARNING("attempt to copy to an image which does not support being a transfer destination");

    VkBufferImageCopy image_copy{};
    image_copy.bufferOffset      = 0;
    image_copy.bufferRowLength   = 0;
    image_copy.bufferImageHeight = 0;

    image_copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.imageSubresource.mipLevel       = 0;
    image_copy.imageSubresource.baseArrayLayer = 0;
    image_copy.imageSubresource.layerCount     = 1;

    image_copy.imageOffset = { 0, 0, 0 };
    image_copy.imageExtent = { extent.x, extent.y, extent.z };

    Ref<TransientCommandBuffer> cmd_buf = new TransientCommandBuffer();

    vkCmdCopyBufferToImage(static_cast<VkCommandBuffer>(cmd_buf->getHandle()),
        static_cast<VkBuffer>(buffer->getHandle()), static_cast<VkImage>(image),
        toVulkanLayout(current_layout), 1, &image_copy);

    cmd_buf->submit();
}

static constexpr VkImageUsageFlagBits vulkan_image_usage[5] = { VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT };

void Texture::createImage()
{
    VkImageCreateInfo image_create_info{};
    image_create_info.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType     = extent.z == 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_3D;
    image_create_info.extent        = { extent.x, extent.y, extent.z };
    image_create_info.mipLevels     = 1;
    image_create_info.arrayLayers   = 1;
    image_create_info.format        = toVulkanFormat(format);
    image_create_info.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_create_info.usage       = convertFlags<VkImageUsageFlagBits, Usage, 5>(usage, vulkan_image_usage);
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples     = VK_SAMPLE_COUNT_1_BIT;
    CHECK_RESULT(vkCreateImage,
        (static_cast<VkDevice>(RenderServer::getDevice()), &image_create_info, nullptr,
            reinterpret_cast<VkImage*>(&image)),
        FAULT, return;);
    current_layout = LAYOUT_UNDEFINED;

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(static_cast<VkDevice>(RenderServer::getDevice()),
        static_cast<VkImage>(image), &memory_requirements);

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocated_size               = memory_requirements.size;
    allocate_info.memoryTypeIndex =
        Buffer::findMemoryType(memory_requirements.memoryTypeBits, MEMORY_PROPERTY_DEVICE_LOCAL);
    CHECK_RESULT(vkAllocateMemory,
        (static_cast<VkDevice>(RenderServer::getDevice()), &allocate_info, nullptr,
            reinterpret_cast<VkDeviceMemory*>(&memory)),
        FAULT, return;);
    CHECK_RESULT(vkBindImageMemory,
        (static_cast<VkDevice>(RenderServer::getDevice()), static_cast<VkImage>(image),
            static_cast<VkDeviceMemory>(memory), 0),
        FAULT, return;);
}

void Texture::createView()
{
    VkImageViewCreateInfo view_create_info{};
    view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image    = static_cast<VkImage>(image);
    view_create_info.viewType = extent.z == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_3D;
    view_create_info.format   = toVulkanFormat(format);
    view_create_info.subresourceRange.aspectMask =
        format == FORMAT_DEPTH ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    view_create_info.subresourceRange.baseMipLevel   = 0;
    view_create_info.subresourceRange.levelCount     = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount     = 1;

    CHECK_RESULT(vkCreateImageView,
        (static_cast<VkDevice>(RenderServer::getDevice()), &view_create_info, nullptr,
            reinterpret_cast<VkImageView*>(&view)),
        ERROR, return;);
}

void Texture::destroyResources()
{
    QUEUE_FREE(image, VkImage, vkDestroyImage);
    QUEUE_FREE(view, VkImageView, vkDestroyImageView);
    QUEUE_FREE(memory, VkDeviceMemory, vkFreeMemory);
}

Ref<Texture> Texture::duplicate()
{
    auto new_tex = new Texture(getSize(), getFormat(), nullptr);
    auto old_layout = current_layout;
    transitionLayout(Texture::LAYOUT_TRANSFER_SRC);
    new_tex->transitionLayout(Texture::LAYOUT_TRANSFER_DST);

    VkImageCopy image_copy{};
    image_copy.srcOffset = { 0, 0, 0 };
    image_copy.dstOffset = { 0, 0, 0 };

    image_copy.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.dstSubresource.mipLevel       = 0;
    image_copy.dstSubresource.baseArrayLayer = 0;
    image_copy.dstSubresource.layerCount     = 1;

    image_copy.srcSubresource = image_copy.dstSubresource;

    image_copy.extent = { extent.x, extent.y, extent.z };

    Ref<TransientCommandBuffer> cmd_buf = new TransientCommandBuffer();

    vkCmdCopyImage(static_cast<VkCommandBuffer>(cmd_buf->getHandle()), static_cast<VkImage>(image),
        toVulkanLayout(current_layout), static_cast<VkImage>(new_tex->image),
        toVulkanLayout(new_tex->current_layout), 1, &image_copy);

    cmd_buf->submit();
    transitionLayout(old_layout);
    new_tex->transitionLayout(Texture::LAYOUT_SHADER_READ);

    return new_tex;
}
