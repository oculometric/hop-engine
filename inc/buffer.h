#pragma once

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

class Buffer
{
public:
	enum BufferUsage
	{
		BUFFER_USAGE_TRANSFER_SRC = 1,
		BUFFER_USAGE_TRANSFER_DST = 2,
		BUFFER_USAGE_UNIFORM = 4,
		BUFFER_USAGE_VERTEX = 8,
		BUFFER_USAGE_INDEX = 16
	};
	
private:
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize buffer_size = 0;
	void* mapped = nullptr;

public:
	DELETE_CONSTRUCTORS(Buffer);

	void* mapMemory();
	void unmapMemory();
	VkBuffer getBuffer() const { return buffer; }
	VkDeviceSize getSize() const { return buffer_size; }
	static uint32_t findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties);
	void copyToBuffer(Ref<Buffer> other) const;
	
	Buffer(VkDeviceSize size, BufferUsage usage, VkMemoryPropertyFlags properties);
	~Buffer();
};

}
