#pragma once

#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

class Sampler final : public Destructible
{
public:
	enum Filter
	{
		FILTER_NEAREST,
		FILTER_LINEAR
	};

	enum Address
	{
		ADDRESS_REPEAT,
		ADDRESS_MIRRORED,
		ADDRESS_CLAMP_EDGE
	};

	struct Builder final
	{
		Filter filtering_mode = FILTER_LINEAR;
		Address address_mode = ADDRESS_REPEAT;

		Builder& filter(const Filter value) { filtering_mode = value; return *this; }
		Builder& address(const Address value) { address_mode = value; return *this; }
	};
private:
	VkSampler sampler = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Sampler);
	Sampler(const Builder& config = Builder());
	~Sampler() override;
	
	VkSampler getSampler() const { return sampler; }
};

inline bool operator<(const Sampler::Builder& a, const Sampler::Builder& b)
{
	if (a.address_mode < b.address_mode)
		return true;
	if (a.filtering_mode < b.filtering_mode)
		return true;
	return false;
}

class Texture final : public Destructible
{
public:
	enum Usage
	{
		IMAGE_USAGE_DEFAULT = 0,
		IMAGE_USAGE_TRANSFER_SRC = 1,
		IMAGE_USAGE_TRANSFER_DST = 2,
		IMAGE_USAGE_SAMPLED = 4,
		IMAGE_USAGE_COLOR_ATTACHMENT = 8,
		IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 16,
	};

	enum Format
	{
		FORMAT_R8G8B8A8_SRGB,
		FORMAT_D32_SFLOAT_S8_UINT,
		FORMAT_R16G16B16A16_SFLOAT,
		FORMAT_B8G8R8A8_SRGB,
	};

	enum Layout
	{
		LAYOUT_UNDEFINED,
		LAYOUT_PRESENT_SRC,
		LAYOUT_COLOR_ATTACHMENT,
		LAYOUT_DEPTH_STENCIL_ATTACHMENT,
		LAYOUT_SHADER_READ_ONLY,
		LAYOUT_DEPTH_STENCIL_READ_ONLY,
		LAYOUT_TRANSFER_SRC,
		LAYOUT_TRANSFER_DST,
	};

	struct Builder final
	{
		void* data_ptr = nullptr;
		Usage usage_flags = IMAGE_USAGE_DEFAULT;
		glm::u32vec2 layer_arrangement = { 1, 1 };
		
		Builder& data(void* value) { data_ptr = value; return *this; }
		Builder& usage(const Usage value) { usage_flags = value; return *this; }
		Builder& layers(const glm::u32vec2 arrangement) { layer_arrangement = arrangement; return *this; }
	};

private:
	std::string origin;
	glm::u32vec3 extent;
	Layout current_layout;
	Format format;
	Usage usage;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkImageView stencil_view = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Texture);
	Texture(size_t _width, size_t _height, Format _format, const Builder& builder = Builder());
	Texture(const std::string& file, const Builder& builder = Builder());
	~Texture() override;
	
	static Format getDepthFormat();
	static Format getDataFormat();
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	glm::ivec2 getSize() const { return { extent.x, extent.y }; }
	bool is3D() const { return { extent.z != 1 }; }
	Format getFormat() const { return format; }
	VkImageView getView(bool stencil = false);
	void transitionLayout(Layout new_layout);
	void copyBufferToImage(Ref<Buffer> buffer) const;

private:
	void createImage();
	void loadFromMemory(void* data, glm::u32vec2 layers);
	void destroyResources();
};

TO_STRING_DEC(Sampler::Filter);
TO_STRING_DEC(Sampler::Address);
TO_STRING_DEC(Texture::Format);
ENUM_OPERATOR(Texture::Usage);
TO_STRING_DEC(Texture::Usage);
TO_STRING_DEC(Texture::Layout);

}
