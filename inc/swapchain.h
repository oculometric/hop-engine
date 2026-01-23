#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

#include "common.h"

namespace HopEngine
{

struct SwapchainSupportInfo
{
	VkSurfaceCapabilitiesKHR surface_capabilities{ };
	std::vector<VkSurfaceFormatKHR> surface_formats;
	std::vector<VkPresentModeKHR> present_modes;
};

class Swapchain : public Destructible
{
private:
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector<VkImage> images;
	VkFormat format;
	VkExtent2D extent;
	std::vector<VkImageView> image_views;
	VkSurfaceKHR surface;
	uint32_t queue_families[2];
	VkSwapchainCreateInfoKHR create_info;

public:
	DELETE_CONSTRUCTORS(Swapchain);

	inline VkFormat getFormat() const { return format; }
	inline VkExtent2D getExtent() const { return extent; }
	inline uint32_t getImageCount() const { return static_cast<uint32_t>(image_views.size()); }
	inline VkImageView getImage(size_t i) const { return image_views[i]; }
	inline VkSwapchainKHR getSwapchain() const { return swapchain; }
	void resize(uint32_t width, uint32_t height);

	static SwapchainSupportInfo getSupportInfo(VkPhysicalDevice device, VkSurfaceKHR surface);
	static VkSurfaceFormatKHR getIdealSurfaceFormat(const SwapchainSupportInfo& info);
	static VkPresentModeKHR getIdealPresentMode(const SwapchainSupportInfo& info);
	static VkExtent2D getIdealExtent(const SwapchainSupportInfo& info, uint32_t window_width, uint32_t window_height);

	Swapchain(uint32_t width, uint32_t height, VkSurfaceKHR surface);
	~Swapchain() override;

private:
	void createImageViews();
	void destroyResources();
};

}
