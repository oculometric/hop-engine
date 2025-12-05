#include "buffer.h"

#include <stdexcept>

#include "graphics_environment.h"

using namespace HopEngine;
using namespace std;

Buffer::Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo buffer_create_info{ };
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = size;
    buffer_create_info.usage = usage;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(GraphicsEnvironment::get()->getDevice(), &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
        throw runtime_error("vkCreateBuffer failed");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(GraphicsEnvironment::get()->getDevice(), buffer, &memory_requirements);

    VkMemoryAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = Buffer::findMemoryType(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(GraphicsEnvironment::get()->getDevice(), &allocate_info, nullptr, &memory) != VK_SUCCESS)
        throw runtime_error("vkAllocateMemory failed");

    vkBindBufferMemory(GraphicsEnvironment::get()->getDevice(), buffer, memory, 0);

    buffer_size = size;
}

Buffer::~Buffer()
{
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
