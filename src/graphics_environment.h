#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Window;
class Swapchain;
class RenderPass;
class Pipeline;
class Shader;
class Mesh;

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

	int MAX_FRAMES_IN_FLIGHT = 2;

private:
	Ref<Window> window = nullptr;

	VkInstance instance = VK_NULL_HANDLE;
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphics_queue = VK_NULL_HANDLE;
	VkQueue present_queue = VK_NULL_HANDLE;
	VkCommandPool command_pool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer> command_buffers;
	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	Ref<Swapchain> swapchain = nullptr;
	std::vector<VkFramebuffer> framebuffers;
	Ref<RenderPass> render_pass = nullptr;
	Ref<Shader> shader = nullptr;
	Ref<Pipeline> pipeline = nullptr;
	Ref<Mesh> mesh = nullptr;

public:
	GraphicsEnvironment(Ref<Window> main_window);
	~GraphicsEnvironment();

	QueueFamilies getQueueFamilies(VkPhysicalDevice device);
	inline VkPhysicalDevice getPhysicalDevice() { return physical_device; }
	inline VkDevice getDevice() { return device; }
	static GraphicsEnvironment* get();
	void drawFrame();

private:
	void createInstance();
	void createDevice();
	void createResources();
	void createCommandPool();
	void createSyncObjects();

	void recordRenderCommands(VkCommandBuffer command_buffer, uint32_t image_index);
};

}
