#include "buffer.h"

#include "command_buffer.h"
#include "render_server.h"
#include "vulkan_helpers.h"

#include <map>
#include <vulkan/vulkan.hpp>

using namespace HopEngine;

static constexpr VkBufferUsageFlagBits vulkan_buffer_usage[5] = { VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT };

TO_STRING_IMPL_BITFLAGS(Buffer::Usage, 5,
    VARGS("TRANSFER_SRC", "TRANSFER_DST", "UNIFORM", "VERTEX", "INDEX"));

static constexpr VkMemoryPropertyFlagBits vulkan_memory_props[4] = { VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    VK_MEMORY_PROPERTY_HOST_CACHED_BIT };

TO_STRING_IMPL_BITFLAGS(MemoryProperties, 4,
    VARGS("DEVICE_LOCAL", "HOST_VISIBLE", "HOST_COHERENT", "HOST_CACHED"));

Buffer::Buffer(size_t size, const Usage usage, const MemoryProperties properties)
{
    if (size == 0)
    {
        DBG_ERROR("buffer size was zero, this is not allowed");
        size = 1;
    }

    // vulkan buffer creation
    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size  = size;
    buffer_create_info.usage = convertFlags<VkBufferUsageFlagBits, Usage, 5>(usage, vulkan_buffer_usage);
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(static_cast<VkDevice>(RenderServer::getDevice()), &buffer_create_info, nullptr,
            reinterpret_cast<VkBuffer*>(&buffer)) != VK_SUCCESS)
        DBG_FAULT("vkCreateBuffer failed");

    // then we need to find appropriate memory
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(static_cast<VkDevice>(RenderServer::getDevice()),
        static_cast<VkBuffer>(buffer), &memory_requirements);

    // and allocate it
    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize  = memory_requirements.size;
    allocate_info.memoryTypeIndex = Buffer::findMemoryType(memory_requirements.memoryTypeBits, properties);

    if (vkAllocateMemory(static_cast<VkDevice>(RenderServer::getDevice()), &allocate_info, nullptr,
            reinterpret_cast<VkDeviceMemory*>(&memory)) != VK_SUCCESS)
        DBG_FAULT("vkAllocateMemory failed");

    // and bind it to our buffer
    vkBindBufferMemory(static_cast<VkDevice>(RenderServer::getDevice()), static_cast<VkBuffer>(buffer),
        static_cast<VkDeviceMemory>(memory), 0);

    DBG_VERBOSE("created buffer of size " + std::to_string(size) + " with usage " + to_string(usage) +
                " and memory properties " + to_string(properties));

    buffer_size = size;
}

Buffer::~Buffer()
{
    DBG_VERBOSE("destroying buffer " + PTR(this));
    // make sure the buffer is unmapped
    unmapMemory();

    QUEUE_FREE(buffer, VkBuffer, vkDestroyBuffer);
    QUEUE_FREE(memory, VkDeviceMemory, vkFreeMemory);
}

uint32_t Buffer::findMemoryType(const uint32_t type_bits, const MemoryProperties _properties)
{
    static std::map<std::pair<uint32_t, MemoryProperties>, uint32_t> known_memory_types;

    auto it = known_memory_types.find({ type_bits, _properties });
    if (it != known_memory_types.end()) return it->second;

    // check through the available vulkan memory types to find one which fits the
    // requirements.
    const VkMemoryPropertyFlags properties =
        convertFlags<VkMemoryPropertyFlagBits, MemoryProperties, 4>(_properties, vulkan_memory_props);
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(static_cast<VkPhysicalDevice>(RenderServer::getPhysicalDevice()),
        &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if ((type_bits & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            known_memory_types[{ type_bits, _properties }] = i;
            return i;
        }
    }
    DBG_FAULT("failed to find suitable memory type");
    return 0;
}

void* Buffer::mapMemory()
{
    // only map the memory if it isn't already mapped (otherwise just
    // return the already-mapped pointer)
    if (mapped == nullptr)
        vkMapMemory(static_cast<VkDevice>(RenderServer::getDevice()), static_cast<VkDeviceMemory>(memory),
            0, buffer_size, 0, &mapped);

    return mapped;
}

void Buffer::unmapMemory()
{
    if (mapped == nullptr) return;

    vkUnmapMemory(static_cast<VkDevice>(RenderServer::getDevice()), static_cast<VkDeviceMemory>(memory));
    mapped = nullptr;
}

void Buffer::copyToBuffer(const Ref<Buffer>& other) const
{
    DBG_VERBOSE("copying from " + PTR(this) + " to buffer " + PTR(other.get()));
    Ref<TransientCommandBuffer> cmd_buf = new TransientCommandBuffer();

    // perform vulkan buffer-to-buffer copy on an immediate command buffer
    // this assumes both buffers are the same size!
    VkBufferCopy buffer_copy{};
    buffer_copy.srcOffset = 0;
    buffer_copy.dstOffset = 0;
    buffer_copy.size      = buffer_size;
    vkCmdCopyBuffer(static_cast<VkCommandBuffer>(cmd_buf->getHandle()), static_cast<VkBuffer>(buffer),
        static_cast<VkBuffer>(other->buffer), 1, &buffer_copy);

    cmd_buf->submit();
}

void Buffer::bind(WeakRef<DrawCommandBuffer> command_buffer, int type)
{
    if (type == 0) command_buffer->bindVertexBuffer(buffer);
    else if (type == 1)
        command_buffer->bindIndexBuffer(buffer);
}
