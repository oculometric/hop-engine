#pragma once

#include <string>
#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Buffer;

class Image
{
private:
	size_t width;
	size_t height;
	VkImageLayout current_layout;
	VkFormat format;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Image);

	// TODO: empty image constructor
	Image(std::string file);
	~Image();

	void transitionLayout(VkImageLayout new_layout);
	void copyBufferToImage(Ref<Buffer> buffer);
	VkImageView getView();

private:
	void createImage();
	void loadFromMemory(void* data);
};

}
