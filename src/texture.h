#pragma once

#include <string>
#include <vulkan/vulkan.hpp>
#include <glm/vec2.hpp>

#include "common.h"

namespace HopEngine
{

class Texture
{
public:
	static constexpr VkFormat depth_format = VK_FORMAT_D32_SFLOAT;
	static constexpr VkFormat data_format = VK_FORMAT_R16G16B16A16_SFLOAT;

private:
	size_t width;
	size_t height;
	VkImageLayout current_layout;
	VkFormat format;
	VkImageUsageFlags usage;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Texture);

	Texture(size_t width, size_t height, VkFormat format, void* data = nullptr, VkImageUsageFlags usage = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM);
	Texture(std::string file, VkImageUsageFlags usage = VK_IMAGE_USAGE_FLAG_BITS_MAX_ENUM);
	~Texture();

	void transitionLayout(VkImageLayout new_layout);
	void copyBufferToImage(Ref<Buffer> buffer);
	VkImageView getView();
	inline glm::ivec2 getSize() { return { width, height }; }

private:
	void createImage();
	void loadFromMemory(void* data);
};

}
