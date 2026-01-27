#pragma once

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

enum BufferUsage
{
	BUFFER_USAGE_TRANSFER_SRC = 1,
	BUFFER_USAGE_TRANSFER_DST = 2,
	BUFFER_USAGE_UNIFORM = 4,
	BUFFER_USAGE_VERTEX = 8,
	BUFFER_USAGE_INDEX = 16
};
ENUM_OPERATOR(BufferUsage)
TO_STRING_DEC(BufferUsage);

enum MemoryProperties
{
	MEMORY_PROPERTY_DEVICE_LOCAL = 1,
	MEMORY_PROPERTY_HOST_VISIBLE = 2,
	MEMORY_PROPERTY_HOST_COHERENT = 4,
	MEMORY_PROPERTY_HOST_CACHED = 8,
};
ENUM_OPERATOR(MemoryProperties)
TO_STRING_DEC(MemoryProperties);

class Buffer
{
private:
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize buffer_size = 0;
	void* mapped = nullptr;

public:
	DELETE_CONSTRUCTORS(Buffer);
	Buffer(VkDeviceSize size, BufferUsage usage, MemoryProperties properties);
	~Buffer();
	
	VkBuffer getBuffer() const { return buffer; }
	VkDeviceSize getSize() const { return buffer_size; }
	void* mapMemory();
	void unmapMemory();
	static uint32_t findMemoryType(uint32_t type_bits, MemoryProperties properties);
	void copyToBuffer(Ref<Buffer> other) const;
};

}
