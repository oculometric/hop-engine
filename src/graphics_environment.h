#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "window.h"

namespace HopEngine
{

class Swapchain;
class RenderPass;
class Pipeline;
class Shader;

class GraphicsEnvironment
{
public:
	struct QueueFamilies
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;
	};

private:
	const std::vector<const char*> required_validation_layers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

	const std::vector<const char*> required_extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

private:
	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphics_queue = VK_NULL_HANDLE;
	VkQueue present_queue = VK_NULL_HANDLE;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	Swapchain* swapchain = nullptr;
	RenderPass* render_pass = nullptr;
	Shader* shader = nullptr;
	Pipeline* pipeline = nullptr;

public:
	GraphicsEnvironment(Window* window);

	QueueFamilies getQueueFamilies(VkPhysicalDevice device);
	inline VkPhysicalDevice getPhysicalDevice() { return physical_device; }
	inline VkDevice getDevice() { return device; }
	static GraphicsEnvironment* get();

	~GraphicsEnvironment();

private:
	void createInstance();
	void createDevice();
	
};

}
