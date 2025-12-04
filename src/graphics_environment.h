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
class Material;
class Buffer;

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

	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	VkDescriptorSetLayout scene_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout object_descriptor_set_layout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> scene_descriptor_sets;
	std::vector<Ref<Buffer>> scene_uniform_buffers;

	std::vector<VkDescriptorSet> test_object_descriptor_sets;
	std::vector<Ref<Buffer>> test_object_uniform_buffers;

	Ref<Shader> shader = nullptr;
	Ref<Material> material = nullptr;
	Ref<Mesh> mesh = nullptr;

public:
	GraphicsEnvironment(Ref<Window> main_window);
	~GraphicsEnvironment();

	inline Ref<RenderPass> getRenderPass() { return render_pass; }
	QueueFamilies getQueueFamilies(VkPhysicalDevice device);
	inline VkPhysicalDevice getPhysicalDevice() { return physical_device; }
	inline VkDevice getDevice() { return device; }
	inline VkDescriptorSetLayout getSceneDescriptorSetLayout() { return scene_descriptor_set_layout; }
	inline VkDescriptorSetLayout getObjectDescriptorSetLayout() { return object_descriptor_set_layout; }
	static GraphicsEnvironment* get();
	void drawFrame();
	void createUniformsAndDescriptorSets(VkDescriptorSetLayout layout, VkDeviceSize buffer_size, std::vector<VkDescriptorSet>& descriptor_sets, std::vector<Ref<Buffer>>& uniform_buffers);
	void freeDescriptorSets(std::vector<VkDescriptorSet>& descriptor_sets);

private:
	void createInstance();
	void createDevice();
	void createResources();
	void createCommandPool();
	void createSyncObjects();

	void recordRenderCommands(VkCommandBuffer command_buffer, uint32_t image_index);
};

}
