#pragma once

#include <vector>
#include <map>
#include <vulkan/vulkan.hpp>

#include "shader.h"
#include "common.h"

namespace HopEngine
{

class UniformBlock : public Destructible
{
private:
	std::vector<VkDescriptorSet> descriptor_sets;
	std::vector<Ref<Buffer>> uniform_buffers;
	std::map<uint32_t, std::pair<Ref<Texture>, Ref<Sampler>>> textures_in_use;
	std::vector<uint8_t> live_uniform_buffer;
	VkDeviceSize size;
	ShaderLayout layout;

public:
	DELETE_CONSTRUCTORS(UniformBlock);

	UniformBlock(ShaderLayout layout_info);
	~UniformBlock() override;

	inline void* getBuffer() { return live_uniform_buffer.data(); }
	void setTexture(uint32_t binding, Ref<Texture> image);
	void setSampler(uint32_t binding, Ref<Sampler> sampler);
	inline VkDeviceSize getSize() const { return size; }
	void pushToDescriptorSet(size_t index);
	inline VkDescriptorSet getDescriptorSet(size_t index) const { return descriptor_sets[index]; }
	void drawImGuiDebug(const std::map<std::string, uint32_t>& texture_name_to_binding);

private:
	void applyDescriptorBindings();
};

}
