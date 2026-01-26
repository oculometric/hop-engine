#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

class Swapchain : public Destructible
{
private:
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	VkFormat format;
	glm::u32vec2 extent;
	std::vector<VkImageView> image_views;
	VkSurfaceKHR surface;
	uint32_t queue_families[2];
	std::vector<VkSwapchainCreateInfoKHR> create_info; // just... don't worry about it

public:
	DELETE_CONSTRUCTORS(Swapchain);

	VkFormat getFormat() const { return format; }
	glm::u32vec2 getExtent() const { return extent; }
	uint32_t getImageCount() const { return static_cast<uint32_t>(image_views.size()); }
	VkImageView getImage(size_t i) const { return image_views[i]; }
	VkSwapchainKHR getSwapchain() const { return swapchain; }
	void resize(uint32_t width, uint32_t height);

	Swapchain(uint32_t width, uint32_t height, VkSurfaceKHR surface);
	~Swapchain() override;

private:
	void createImageViews();
	void destroyResources();
};

}
