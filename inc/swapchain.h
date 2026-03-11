#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "vulkan_typedefs.h"
#include "texture.h"

namespace HopEngine
{

class Swapchain : public Destructible
{
public:
	struct SupportInfo
	{
		std::vector<VkSurfaceCapabilitiesKHR> surface_capabilities;
		std::vector<VkSurfaceFormatKHR> surface_formats;
		// TODO: re-introduce checking for FIFO and IMMEDIATE present mode support
	};

private:
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	Texture::Format format;
	glm::u32vec2 extent;
	std::vector<VkImageView> image_views;
	VkSurfaceKHR surface;
	uint32_t queue_families[2];
	VkSwapchainCreateInfoKHR* create_info;

public:
	DELETE_CONSTRUCTORS(Swapchain);
	Swapchain(uint32_t width, uint32_t height, VkSurfaceKHR _surface);
	~Swapchain() override;
	
	static SupportInfo getSwapchainSupportInfo(VkPhysicalDevice device, VkSurfaceKHR surface);
	static glm::u32vec2 computeExtent(uint32_t window_width, uint32_t window_height);
	static uint32_t computeImageCount();
	
	VkSwapchainKHR getHandle() const { return swapchain; }
	uint32_t getImageCount() const { return static_cast<uint32_t>(image_views.size()); }
	VkImageView getImage(size_t i) const { return image_views[i]; }
	Texture::Format getFormat() const { return format; }
	glm::u32vec2 getExtent() const { return extent; }

	void resize(uint32_t width, uint32_t height);
	void setVsync(bool enabled);
	bool getVsync();

private:
	void createImageViews();
	void destroyResources() const;
};

}
