#pragma once

#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

enum ImageUsage
{
	IMAGE_USAGE_DEFAULT = 0,
	IMAGE_USAGE_TRANSFER_SRC = 1,
	IMAGE_USAGE_TRANSFER_DST = 2,
	IMAGE_USAGE_SAMPLED = 4,
	IMAGE_USAGE_COLOR_ATTACHMENT = 8,
	IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT = 16,
};
ENUM_OPERATOR(ImageUsage)
TO_STRING_DEC(ImageUsage);

enum ImageFormat
{
	FORMAT_R8G8B8A8_SRGB,
	FORMAT_D32_SFLOAT_S8_UINT,
	FORMAT_R16G16B16A16_SFLOAT,
	FORMAT_B8G8R8A8_SRGB,
};
TO_STRING_DEC(ImageFormat);

enum ImageLayout
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
TO_STRING_DEC(ImageLayout);

struct TextureBuilder
{
	void* data_ptr = nullptr;
	ImageUsage usage_flags = IMAGE_USAGE_DEFAULT;
	glm::u32vec2 layer_arrangement = { 1, 1 };
	
	TextureBuilder& data(void* value) { data_ptr = value; return *this; }
	TextureBuilder& usage(const ImageUsage value) { usage_flags = value; return *this; }
	TextureBuilder& layers(const glm::u32vec2 arrangement) { layer_arrangement = arrangement; return *this; }
};

class Texture : public Destructible
{
private:
	std::string origin;
	glm::u32vec3 extent;
	ImageLayout current_layout;
	ImageFormat format;
	ImageUsage usage;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkImageView stencil_view = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Texture);
	Texture(size_t _width, size_t _height, ImageFormat _format, const TextureBuilder& builder = TextureBuilder());
	Texture(const std::string& file, const TextureBuilder& builder = TextureBuilder());
	~Texture() override;
	
	static ImageFormat getDepthFormat();
	static ImageFormat getDataFormat();
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	glm::ivec2 getSize() const { return { extent.x, extent.y }; }
	ImageFormat getFormat() const { return format; }
	VkImageView getView(bool stencil = false);
	void transitionLayout(ImageLayout new_layout);
	void copyBufferToImage(Ref<Buffer> buffer) const;

private:
	void createImage();
	void loadFromMemory(void* data, glm::u32vec2 layers);
};

}
