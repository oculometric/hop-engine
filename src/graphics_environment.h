#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/vec2.hpp>

#include "common.h"

namespace HopEngine
{

class RenderServer
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
#if !defined(NDEBUG)
		"VK_LAYER_KHRONOS_validation"
#endif
	};

	const std::vector<const char*> required_instance_extensions =
	{
#if !defined(NDEBUG)
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
	};

	const std::vector<const char*> required_extensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	int MAX_FRAMES_IN_FLIGHT = 2;

private:
	Ref<Window> window;

	VkInstance instance = VK_NULL_HANDLE;
#if !defined(NDEBUG)
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
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

	Ref<Swapchain> swapchain;
    Ref<RenderPass> offscreen_pass;
	Ref<RenderPass> render_pass;

	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	VkDescriptorSetLayout scene_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout object_descriptor_set_layout = VK_NULL_HANDLE;

	Ref<Texture> default_image;
	Ref<Sampler> default_sampler;
	Ref<Mesh> quad;
	Ref<Material> post_process;

public:
	static void init(Ref<Window> main_window);
	static void destroy();

	static void waitIdle();
	static Ref<RenderPass> getMainRenderPass();
	static QueueFamilies getQueueFamilies(VkPhysicalDevice device);
	static VkPhysicalDevice getPhysicalDevice();
	static VkDevice getDevice();
	static VkDescriptorSetLayout getSceneDescriptorSetLayout();
	static VkDescriptorSetLayout getObjectDescriptorSetLayout();
	static size_t getFramesInFlight();
	static VkDescriptorPool getDescriptorPool();
	static VkCommandPool getCommandPool();
	static VkQueue getGraphicsQueue();
	static std::pair<Ref<Texture>, Ref<Sampler>> getDefaultTextureSampler();
	static glm::vec2 getFramebufferSize();

	static void draw(float delta_time);
	static void resize();

private:
	RenderServer(Ref<Window> main_window);
	~RenderServer();

	void createInstance();
	void createDevice();
	void createDescriptorPoolAndSets();
	void createCommandPool();
	void createSyncObjects();
	void initImGui();

	void drawFrame(float delta_time);
	void resizeSwapchain();

	void recordRenderCommands(VkCommandBuffer command_buffer, uint32_t image_index);
};

}
