#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <vulkan/vulkan.hpp>

namespace HopEngine
{

struct SwapchainSupportInfo
{
    VkSurfaceCapabilitiesKHR surface_capabilities{ };
    std::vector<VkSurfaceFormatKHR> surface_formats;
    std::vector<VkPresentModeKHR> present_modes;
};

SwapchainSupportInfo getSwapchainSupportInfo(VkPhysicalDevice device, VkSurfaceKHR surface);
VkSurfaceFormatKHR getIdealSurfaceFormat(const SwapchainSupportInfo& info);
VkPresentModeKHR getIdealPresentMode(const SwapchainSupportInfo& info);
glm::u32vec2 getIdealExtent(const SwapchainSupportInfo& info, uint32_t window_width, uint32_t window_height);

}