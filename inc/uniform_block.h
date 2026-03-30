#pragma once

#include <vector>
#include <map>

#include "common.h"
#include "vulkan_typedefs.h"
#include "material.h"

namespace HopEngine
{
	
class UniformBlock final : public Destructible
{
public:
	/**
	 * @brief holds information about a combined texture-sampler descriptor binding.
	 */
	struct TextureBinding final
	{
		Ref<Texture> texture;		// texture reference to be bound
		Ref<Sampler> sampler;		// sampler reference to be bound
	};

private:
	// array of descriptor sets
	std::vector<VkDescriptorSet> descriptor_sets;
	// array of buffers containing uniform variables, one per descriptor set
	std::vector<Ref<Buffer>> uniform_buffers;
	// mapping between descriptor index and the texture binding
	std::map<uint32_t, TextureBinding> textures_in_use;
	// CPU-accessible block of data which the program can write to
	std::vector<uint8_t> live_uniform_buffer;
	VkDeviceSize size;		// size of the uniform buffer
	Shader::Layout layout;	// information about the size and offset of uniform variables
	uint32_t set_index;
    bool rebind_needed = true;

public:
	DELETE_CONSTRUCTORS(UniformBlock);
	/**
	 * @brief creates a uniform block from a corresponding shader layout.
	 * @param layout_info layout information listing the descriptor bindings.
	 */
	UniformBlock(const Shader::Layout& layout_info);
	~UniformBlock() override;
	
	void bind(WeakRef<DrawCommandBuffer> command_buffer);
	void* getBuffer() { return live_uniform_buffer.data(); }
	VkDeviceSize getSize() const { return size; }
	/**
	 * @brief update the bound texture (image view) for a given binding index.
	 * @param binding descriptor binding index, matching to that specified in the shader.
	 * @param image texture to bind. if this image is already bound, nothing will change.
	 * @param use_stencil whether the stencil view aspect should be used.
	 */
	void setTexture(uint32_t binding, Ref<Texture> image);
	/**
	 * @brief update the bound sampler for a given binding index.
	 * @param binding descriptor binding index, matching to that specified in the shader.
	 * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
	 */
	void setSampler(uint32_t binding, Ref<Sampler> sampler);
    void setTextureSampler(uint32_t binding, Ref<Texture> texture, Ref<Sampler> sampler);
	
	void drawImGuiDebug(const std::map<std::string, uint32_t>& texture_name_to_binding);

private:
	/**
	 * @brief issues descriptor set write commands to bind the uniform buffers and
	 * textures to the appropriate places.
	 */
	void applyDescriptorBindings();
};

}
