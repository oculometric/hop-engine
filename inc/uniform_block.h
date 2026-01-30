#pragma once

#include <vector>
#include <map>

#include "common.h"
#include "vulkan_typedefs.h"
#include "shader.h"

namespace HopEngine
{

/**
 * @brief holds information about a combined texture-sampler descriptor binding.
 */
struct TextureBinding
{
	Ref<Texture> texture;		// texture reference to be bound
	Ref<Sampler> sampler;		// sampler reference to be bound
	bool use_stencil = false;	// whether the imageview should be used in stencil mode
};
	
class UniformBlock : public Destructible
{
private:
	// array of descriptor sets, one per frame-in-flight
	std::vector<VkDescriptorSet> descriptor_sets;
	// array of buffers containing uniform variables, one per descriptor set
	std::vector<Ref<Buffer>> uniform_buffers;
	// mapping between descriptor index and the texture binding
	std::map<uint32_t, TextureBinding> textures_in_use;
	// CPU-accessible block of data which the program can write to
	std::vector<uint8_t> live_uniform_buffer;
	VkDeviceSize size;		// size of the uniform buffer
	ShaderLayout layout;	// information about the size and offset of uniform variables

public:
	DELETE_CONSTRUCTORS(UniformBlock);
	/**
	 * @brief creates a uniform block from a corresponding shader layout.
	 * @param layout_info layout information listing the descriptor bindings.
	 */
	UniformBlock(const ShaderLayout& layout_info);
	~UniformBlock() override;
	
	VkDescriptorSet getDescriptorSet(const size_t index) const { return descriptor_sets[index]; }
	void* getBuffer() { return live_uniform_buffer.data(); }
	VkDeviceSize getSize() const { return size; }
	/**
	 * @brief update the bound texture (image view) for a given binding index.
	 * @param binding descriptor binding index, matching to that specified in the shader.
	 * @param image texture to bind. if this image is already bound, nothing will change.
	 * @param use_stencil whether the stencil view aspect should be used.
	 */
	void setTexture(uint32_t binding, const Ref<Texture>& image, bool use_stencil = false);
	/**
	 * @brief update the bound sampler for a given binding index.
	 * @param binding descriptor binding index, matching to that specified in the shader.
	 * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
	 */
	void setSampler(uint32_t binding, const Ref<Sampler>& sampler);
	/**
	 * @brief updates a given GPU uniform buffer corresponding to a specified descriptor
	 * set. prevents us from updating uniforms in the middle of a frame.
	 * @param index index of the descriptor set to update the uniform buffer of.
	 */
	void pushToDescriptorSet(size_t index);
	
	void drawImGuiDebug(const std::map<std::string, uint32_t>& texture_name_to_binding);

private:
	/**
	 * @brief issues descriptor set write commands to bind the uniform buffers and
	 * textures to the appropriate places.
	 */
	void applyDescriptorBindings();
};

}
