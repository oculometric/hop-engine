#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

struct SwapchainSupportInfo
{
	VkSurfaceCapabilitiesKHR surface_capabilities{ };
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class Swapchain
{
private:
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	VkFormat format;
	VkExtent2D extent;
	std::vector<VkImageView> image_views;

public:
	DELETE_CONSTRUCTORS(Swapchain);

	Swapchain(uint32_t width, uint32_t height, VkSurfaceKHR surface);
	~Swapchain();

	inline VkFormat getFormat() { return format; }
	inline VkExtent2D getExtent() { return extent; }
	inline uint32_t getImageCount() { return static_cast<uint32_t>(image_views.size()); }
	inline VkImageView getImage(uint32_t i) { return image_views[i]; }
	inline VkSwapchainKHR getSwapchain() { return swapchain; }

	static SwapchainSupportInfo getSupportInfo(VkPhysicalDevice device, VkSurfaceKHR surface);
	static VkSurfaceFormatKHR getIdealSurfaceFormat(const SwapchainSupportInfo& info);
	static VkPresentModeKHR getIdealPresentMode(const SwapchainSupportInfo& info);
	static VkExtent2D getIdealExtent(const SwapchainSupportInfo& info, uint32_t window_width, uint32_t window_height);
};

}
