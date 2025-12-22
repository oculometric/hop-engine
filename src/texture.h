#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include <glm/vec2.hpp>

#include "common.h"

namespace HopEngine
{
	
struct TextureBuilder
{
	void* data_ptr = nullptr;
	VkImageUsageFlags usage_flags = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM;
	VkExtent2D layer_arrangement = { 1, 1 };
	
	inline TextureBuilder data(void* value) { data_ptr = value; return *this; }
	inline TextureBuilder usage(VkImageUsageFlags value) { usage_flags = value; return *this; }
	inline TextureBuilder layers(VkExtent2D arrangement) { layer_arrangement = arrangement; return *this; }
};
	
class Texture : public Destructible
{
public:
	static constexpr VkFormat depth_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
	static constexpr VkFormat data_format = VK_FORMAT_R16G16B16A16_SFLOAT;

private:
	VkExtent3D extent;
	VkImageLayout current_layout;
	VkFormat format;
	VkImageUsageFlags usage;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkImageView stencil_view = VK_NULL_HANDLE;
	std::string origin;

public:
	DELETE_CONSTRUCTORS(Texture);

	Texture(size_t width, size_t height, VkFormat format, TextureBuilder builder = TextureBuilder());
	Texture(std::string file, TextureBuilder builder = TextureBuilder());
	~Texture() override;

	void transitionLayout(VkImageLayout new_layout);
	void copyBufferToImage(Ref<Buffer> buffer);
	VkImageView getView(bool stencil = false);
	inline glm::ivec2 getSize() const { return { extent.width, extent.height }; }
	inline VkFormat getFormat() const { return format; }
	inline std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }

private:
	void createImage();
	void loadFromMemory(void* data, VkExtent2D layers);
};

}
