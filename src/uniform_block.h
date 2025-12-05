#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "shader.h"
#include "common.h"

namespace HopEngine
{

class Buffer;
class Image;
class Sampler;

class UniformBlock
{
private:
	std::vector<VkDescriptorSet> descriptor_sets;
	std::vector<Ref<Buffer>> uniform_buffers;
	std::vector<std::pair<Ref<Image>, Ref<Sampler>>> textures_in_use;
	std::vector<uint8_t> live_uniform_buffer;
	VkDeviceSize size;
	ShaderLayout layout;

public:
	DELETE_CONSTRUCTORS(UniformBlock);

	UniformBlock(ShaderLayout layout_info);
	~UniformBlock();

	inline void* getBuffer() { return live_uniform_buffer.data(); }
	void setTexture(size_t index, Ref<Image> image);
	void setSampler(size_t index, Ref<Sampler> sampler);
	inline VkDeviceSize getSize() { return size; }
	void pushToDescriptorSet(size_t index);
	inline VkDescriptorSet getDescriptorSet(size_t index) { return descriptor_sets[index]; }

private:
	void applyDescriptorBindings();
};

}
