#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Buffer;

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
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Texture);

	Texture(size_t width, size_t height, VkFormat format, void* data);
	Texture(std::string file);
	~Texture();

	void transitionLayout(VkImageLayout new_layout);
	void copyBufferToImage(Ref<Buffer> buffer);
	VkImageView getView();

private:
	void createImage();
	void loadFromMemory(void* data);
};

}
