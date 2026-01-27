#include "buffer.h"

#include <vulkan/vulkan.hpp>

#include "graphics_environment.h"
#include "command_buffer.h"

using namespace HopEngine;
using namespace std;

static constexpr VkBufferUsageFlagBits vulkan_buffer_usage[5] = 
{
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT
};

TO_STRING_DEF_BITFLAGS(BufferUsage, 5, VARGS("TRANSFER_SRC", "TRANSFER_DST", "UNIFORM", "VERTEX", "INDEX"));

static constexpr VkMemoryPropertyFlagBits vulkan_memory_props[4] = 
{
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    VK_MEMORY_PROPERTY_HOST_CACHED_BIT
};

TO_STRING_DEF_BITFLAGS(MemoryProperties, 4, VARGS("DEVICE_LOCAL", "HOST_VISIBLE", "HOST_COHERENT", "HOST_CACHED"));

Buffer::Buffer(VkDeviceSize size, const BufferUsage usage, const MemoryProperties properties)
{
    if (size == 0)
    {
        DBG_ERROR("buffer size was zero, this is not allowed");
        size = 1;
    }

    VkBufferCreateInfo buffer_create_info{ };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = convertFlags<VkBufferUsageFlagBits, BufferUsage, 5>(usage, vulkan_buffer_usage);
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(RenderServer::getDevice(), &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
        DBG_FAULT("vkCreateBuffer failed");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(RenderServer::getDevice(), buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = Buffer::findMemoryType(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(RenderServer::getDevice(), &allocate_info, nullptr, &memory) != VK_SUCCESS)
        DBG_FAULT("vkAllocateMemory failed");

    vkBindBufferMemory(RenderServer::getDevice(), buffer, memory, 0);

    DBG_VERBOSE("created buffer of size " + to_string(size) + " with usage " + to_string(usage) + " and memory properties " + to_string(properties));

    buffer_size = size;
}

Buffer::~Buffer()
{
    DBG_VERBOSE("destroying buffer " + PTR(this));
    unmapMemory();

    RenderServer::waitIdle();
    vkDestroyBuffer(RenderServer::getDevice(), buffer, nullptr);
    vkFreeMemory(RenderServer::getDevice(), memory, nullptr);
}

void* Buffer::mapMemory()
{
    if (mapped == nullptr)
        vkMapMemory(RenderServer::getDevice(), memory, 0, buffer_size, 0, &mapped);

    return mapped;
}

void Buffer::unmapMemory()
{
    if (mapped == nullptr)
        return;

    vkUnmapMemory(RenderServer::getDevice(), memory);
    mapped = nullptr;
}

uint32_t Buffer::findMemoryType(uint32_t type_bits, MemoryProperties _properties)
{
    const VkMemoryPropertyFlags properties = convertFlags<VkMemoryPropertyFlagBits, MemoryProperties, 4>(_properties, vulkan_memory_props);
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(RenderServer::getPhysicalDevice(), &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((type_bits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    DBG_FAULT("failed to find suitable memory type");
    return 0;
}

void Buffer::copyToBuffer(Ref<Buffer> other) const
{
    DBG_VERBOSE("copying from " + PTR(this) + " to buffer " + PTR(other.get()));
    Ref<CommandBuffer> cmd_buf = new CommandBuffer();

    VkBufferCopy buffer_copy{ };
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size = buffer_size;
    vkCmdCopyBuffer(cmd_buf->getBuffer(), buffer, other->buffer, 1, &buffer_copy);

    cmd_buf->submit();
}
