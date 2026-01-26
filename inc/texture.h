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

struct TextureBuilder
{
	void* data_ptr = nullptr;
	ImageUsage usage_flags = IMAGE_USAGE_DEFAULT;
	glm::u32vec2 layer_arrangement = { 1, 1 };
	
	TextureBuilder data(void* value) { data_ptr = value; return *this; }
	TextureBuilder usage(ImageUsage value) { usage_flags = value; return *this; }
	TextureBuilder layers(glm::u32vec2 arrangement) { layer_arrangement = arrangement; return *this; }
};

class Texture : public Destructible
{
private:
	glm::u32vec3 extent;
	VkImageLayout current_layout;
	VkFormat format;
	ImageUsage usage;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkImageView stencil_view = VK_NULL_HANDLE;
	std::string origin;

public:
	DELETE_CONSTRUCTORS(Texture);

	void transitionLayout(VkImageLayout new_layout);
	void copyBufferToImage(Ref<Buffer> buffer);
	VkImageView getView(bool stencil = false);
	glm::ivec2 getSize() const { return { extent.x, extent.y }; }
	VkFormat getFormat() const { return format; }
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }

	Texture(size_t width, size_t height, VkFormat format, TextureBuilder builder = TextureBuilder());
	Texture(const std::string& file, TextureBuilder builder = TextureBuilder());
	~Texture() override;
	
	static VkFormat getDepthFormat();
	static VkFormat getDataFormat();

private:
	void createImage();
	void loadFromMemory(void* data, glm::u32vec2 layers);
};

}
