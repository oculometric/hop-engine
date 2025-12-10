#include "texture.h"

#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <vulkan/vk_enum_string_helper.h>

#include "buffer.h"
#include "graphics_environment.h"
#include "command_buffer.h"
#include "package.h"

using namespace HopEngine;
using namespace std;

Texture::Texture(size_t _width, size_t _height, VkFormat _format, void* data, VkImageUsageFlags _usage)
{
    format = _format;
    usage = _usage;
    width = _width; height = _height;

    if (data != nullptr || width == 0 || height == 0)
    {
        loadFromMemory(data);
        DBG_INFO("created image from memory with size " + to_string(width) + "x" + to_string(height) + " and format " + string_VkFormat(format));
    }
    else
    {
        if (width == 0)
        {
            width = 1;
            DBG_WARNING("image width is not allowed to be zero");
        }
        if (height == 0)
        {
            DBG_WARNING("image height is not allowed to be zero");
            height = 1;
        }
        createImage();
        DBG_INFO("created blank image with size " + to_string(width) + "x" + to_string(height) + " and format " + string_VkFormat(format));
    }
}

Texture::Texture(string file, VkImageUsageFlags _usage)
{
    auto file_data = Package::tryLoadFile(file);
    int img_width, img_height, img_channels;
    stbi_uc* pixels = stbi_load_from_memory(file_data.data(), static_cast<int>(file_data.size()), &img_width, &img_height, &img_channels, STBI_rgb_alpha);
    format = VK_FORMAT_R8G8B8A8_SRGB;
    usage = _usage;
    width = img_width; height = img_height;

    if (pixels == nullptr)
    {
        DBG_ERROR("failed to load image file");
        width = 1; height = 1;
        createImage();
    }
    else
    {
        size_t row_size = width * 4;
        void* tmp = new uint8_t[row_size];
        for (size_t i = 0; i < height / 2; ++i)
        {
            memcpy(tmp, pixels + (i * row_size), row_size);
            memcpy(pixels + (i * row_size), pixels + ((height - i - 1) * row_size), row_size);
            memcpy(pixels + ((height - i - 1) * row_size), tmp, row_size);
        }

        loadFromMemory(pixels);
        stbi_image_free(pixels);

        DBG_INFO("created image from " + file + " with size " + to_string(width) + "x" + to_string(height) + " and format " + string_VkFormat(format));
    }
}

Texture::~Texture()
{
    DBG_INFO("destroying image " + PTR(this));
    if (view != VK_NULL_HANDLE)
        vkDestroyImageView(RenderServer::get()->getDevice(), view, nullptr);
    vkDestroyImage(RenderServer::get()->getDevice(), image, nullptr);
    vkFreeMemory(RenderServer::get()->getDevice(), memory, nullptr);
}

void Texture::transitionLayout(VkImageLayout new_layout)
{
    DBG_VERBOSE("transitioning image " + PTR(this) + " layout from " + string_VkImageLayout(current_layout) + " to " + string_VkImageLayout(new_layout));

    VkImageMemoryBarrier memory_barrier{ };
    memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    memory_barrier.oldLayout = current_layout;
    memory_barrier.newLayout = new_layout;
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
    if (current_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        memory_barrier.srcAccessMask = 0;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (current_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
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

    Ref<CommandBuffer> cmd_buf = new CommandBuffer();

    vkCmdPipelineBarrier(cmd_buf->getBuffer(), src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &memory_barrier);

    cmd_buf->submit();
    current_layout = new_layout;
}

void Texture::copyBufferToImage(Ref<Buffer> buffer)
{
    DBG_VERBOSE("copying buffer " + PTR(buffer.get()) + " to image " + PTR(this));

    VkBufferImageCopy image_copy{ };
    image_copy.bufferOffset = 0;
    image_copy.bufferRowLength = 0;
    image_copy.bufferImageHeight = 0;

    image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_copy.imageSubresource.mipLevel = 0;
    image_copy.imageSubresource.baseArrayLayer = 0;
    image_copy.imageSubresource.layerCount = 1;

    image_copy.imageOffset = { 0, 0, 0 };
    image_copy.imageExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1
    };

    Ref<CommandBuffer> cmd_buf = new CommandBuffer();

    vkCmdCopyBufferToImage(cmd_buf->getBuffer(), buffer->getBuffer(), image, current_layout, 1, &image_copy);

    cmd_buf->submit();
}

VkImageView Texture::getView()
{
    if (view != VK_NULL_HANDLE)
        return view;

    DBG_VERBOSE("creating image view for image " + PTR(this));

    VkImageViewCreateInfo view_create_info{ };
    view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_create_info.image = image;
    view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_create_info.format = format;
    if (format == VK_FORMAT_D16_UNORM
        || format == VK_FORMAT_D16_UNORM_S8_UINT
        || format == VK_FORMAT_D32_SFLOAT_S8_UINT
        || format == VK_FORMAT_D32_SFLOAT
        || format == VK_FORMAT_D24_UNORM_S8_UINT)
    {
        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
        view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    view_create_info.subresourceRange.baseMipLevel = 0;
    view_create_info.subresourceRange.levelCount = 1;
    view_create_info.subresourceRange.baseArrayLayer = 0;
    view_create_info.subresourceRange.layerCount = 1;

    if (vkCreateImageView(RenderServer::get()->getDevice(), &view_create_info, nullptr, &view) != VK_SUCCESS)
        DBG_ERROR("vkCreateImageView failed");

    return view;
}

void Texture::createImage()
{
    VkImageCreateInfo image_create_info{ };
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.extent.width = static_cast<uint32_t>(width);
    image_create_info.extent.height = static_cast<uint32_t>(height);
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.format = format;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if ((usage & VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM) == VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM)
    {
        if (format == Texture::depth_format)
            usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        else if (format == Texture::data_format)
            usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        else
            usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    image_create_info.usage = usage;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    if (vkCreateImage(RenderServer::get()->getDevice(), &image_create_info, nullptr, &image) != VK_SUCCESS)
        DBG_FAULT("vkCreateImage failed");
    current_layout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(RenderServer::get()->getDevice(), image, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = Buffer::findMemoryType(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (vkAllocateMemory(RenderServer::get()->getDevice(), &allocate_info, nullptr, &memory) != VK_SUCCESS)
        DBG_FAULT("vkAllocateMemory failed");
    vkBindImageMemory(RenderServer::get()->getDevice(), image, memory, 0);
}

void Texture::loadFromMemory(void* data)
{
    VkDeviceSize image_length = width * height * 4;
    Ref<Buffer> staging_buffer = new Buffer(image_length, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(staging_buffer->mapMemory(), data, image_length);
    staging_buffer->unmapMemory();

    createImage();
    transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(staging_buffer);
    transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}
