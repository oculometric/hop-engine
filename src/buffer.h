#pragma once

#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Buffer
{
private:
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkDeviceSize buffer_size = 0;
	void* mapped = nullptr;

public:
	DELETE_CONSTRUCTORS(Buffer);

	Buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	~Buffer();

	void* mapMemory();
	void unmapMemory();
	inline VkBuffer getBuffer() { return buffer; }
	inline VkDeviceSize getSize() { return buffer_size; }
	static uint32_t findMemoryType(uint32_t type_bits, VkMemoryPropertyFlags properties);
	// TODO: function to copy buffer-to-buffer
};

}
