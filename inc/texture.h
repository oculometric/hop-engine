#pragma once

#include <string>
#include <glm/glm.hpp>

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
	Sampler(const Builder& config);
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
		FORMAT_SRGB_8X4,
		FORMAT_DEPTH,
		FORMAT_FLOAT_16X4,
		FORMAT_SWAPCHAIN,
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

private:
	std::string origin;
	glm::u32vec3 extent;
	Layout current_layout;
	Format format;
	Usage usage;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Texture);
	Texture(glm::u32vec3 image_extent, Format image_format, void* data_ptr = nullptr);
	~Texture() override;

	static Ref<Texture> loadImage(const std::string& path);
	static Ref<Texture> loadImage3D(const std::string& path, glm::u32vec2 segments);

	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	glm::u32vec3 getSize() const { return extent; }
	bool is3D() const { return (extent.z != 1); }
	Format getFormat() const { return format; }
	VkImageView getView() const { return view; }
	void transitionLayout(Layout new_layout);
	// TODO: upload and download data from the texture

private:
	void createImage();
	void createView();
	void uploadData(void* data_ptr);
	void copyBufferToImage(Ref<Buffer> buffer) const;
	void destroyResources();
};

TO_STRING_DEC(Sampler::Filter);
TO_STRING_DEC(Sampler::Address);
TO_STRING_DEC(Texture::Format);
ENUM_OPERATOR(Texture::Usage);
TO_STRING_DEC(Texture::Usage);
TO_STRING_DEC(Texture::Layout);

}
