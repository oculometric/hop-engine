#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Buffer;

class UniformBlock
{
private:
	std::vector<VkDescriptorSet> descriptor_sets;
	std::vector<Ref<Buffer>> uniform_buffers;
	std::vector<uint8_t> live_uniform_buffer;
	VkDeviceSize size;

public:
	DELETE_CONSTRUCTORS(UniformBlock);

	UniformBlock(VkDescriptorSetLayout layout, VkDeviceSize buffer_size);
	~UniformBlock();

	inline void* getBuffer() { return live_uniform_buffer.data(); }
	inline VkDeviceSize getSize() { return size; }
	void pushToDescriptorSet(size_t index);
	inline VkDescriptorSet getDescriptorSet(size_t index) { return descriptor_sets[index]; }
};

}
