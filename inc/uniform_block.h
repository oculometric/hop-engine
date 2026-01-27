#pragma once

#include <vector>
#include <map>

#include "common.h"
#include "vulkan_typedefs.h"
#include "shader.h"

namespace HopEngine
{
	
struct TextureBinding
{
	Ref<Texture> texture;
	Ref<Sampler> sampler;
	bool use_stencil = false;
};
	
class UniformBlock : public Destructible
{
private:
	std::vector<VkDescriptorSet> descriptor_sets;
	std::vector<Ref<Buffer>> uniform_buffers;
	std::map<uint32_t, TextureBinding> textures_in_use;
	std::vector<uint8_t> live_uniform_buffer;
	VkDeviceSize size;
	ShaderLayout layout;

public:
	DELETE_CONSTRUCTORS(UniformBlock);
	UniformBlock(const ShaderLayout& layout_info);
	~UniformBlock() override;
	
	VkDescriptorSet getDescriptorSet(const size_t index) const { return descriptor_sets[index]; }
	void* getBuffer() { return live_uniform_buffer.data(); }
	VkDeviceSize getSize() const { return size; }
	void setTexture(uint32_t binding, const Ref<Texture>& image, bool use_stencil = false);
	void setSampler(uint32_t binding, const Ref<Sampler>& sampler);
	void pushToDescriptorSet(size_t index);
	
	void drawImGuiDebug(const std::map<std::string, uint32_t>& texture_name_to_binding);

private:
	void applyDescriptorBindings();
};

}
