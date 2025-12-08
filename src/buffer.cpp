#include "buffer.h"

#include <stdexcept>
#include <vulkan/vk_enum_string_helper.h>

#include "graphics_environment.h"
#include "command_buffer.h"

using namespace HopEngine;
using namespace std;

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    if (size == 0)
    {
        DBG_ERROR("buffer size was zero, this is not allowed");
        size = 1;
    }

    VkBufferCreateInfo buffer_create_info{ };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(GraphicsEnvironment::get()->getDevice(), &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
        DBG_FAULT("vkCreateBuffer failed");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(GraphicsEnvironment::get()->getDevice(), buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = Buffer::findMemoryType(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(GraphicsEnvironment::get()->getDevice(), &allocate_info, nullptr, &memory) != VK_SUCCESS)
        DBG_FAULT("vkAllocateMemory failed");

    vkBindBufferMemory(GraphicsEnvironment::get()->getDevice(), buffer, memory, 0);

    DBG_VERBOSE("created buffer of size " + to_string(size) + " with usage " + string_VkBufferUsageFlagBits((VkBufferUsageFlagBits)usage) + " and memory type " + to_string(allocate_info.memoryTypeIndex));

    buffer_size = size;
}

Buffer::~Buffer()
{
    DBG_VERBOSE("destroying buffer " + PTR(this));
    unmapMemory();

    vkDestroyBuffer(GraphicsEnvironment::get()->getDevice(), buffer, nullptr);
    vkFreeMemory(GraphicsEnvironment::get()->getDevice(), memory, nullptr);
}

void* Buffer::mapMemory()
{
    if (mapped == nullptr)
        vkMapMemory(GraphicsEnvironment::get()->getDevice(), memory, 0, buffer_size, 0, &mapped);

    return mapped;
}

void Buffer::unmapMemory()
{
    if (mapped == nullptr)
        return;

    vkUnmapMemory(GraphicsEnvironment::get()->getDevice(), memory);
    mapped = nullptr;
}

uint32_t Buffer::findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(GraphicsEnvironment::get()->getPhysicalDevice(), &memory_properties);

    uint32_t memory_type_index = -1;
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((type_bits & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
            return i;
    }
    throw runtime_error("failed to find suitable memory type");
}

void Buffer::copyToBuffer(Ref<Buffer> other)
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
