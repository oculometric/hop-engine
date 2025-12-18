#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <set>

#include "common.h"
#include "engine.h"

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
	Ref<RenderPass> final_render_pass;

	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	VkDescriptorSetLayout scene_descriptor_set_layout = VK_NULL_HANDLE;
	VkDescriptorSetLayout object_descriptor_set_layout = VK_NULL_HANDLE;

	Ref<Texture> default_image;
	Ref<Sampler> default_sampler;
	Ref<Material> default_material;

	Ref<Material> gizmo_material;
	Ref<Mesh> axes_gizmo;
	Ref<Mesh> rotations_gizmo;
	Ref<Mesh> scale_gizmo;
	Ref<Mesh> skybox_cube;
	Ref<Material> skybox_material;
	WeakRef<Texture> current_skybox;
	Ref<Mesh> quad;
	Ref<Material> passthrough;
	WeakRef<Texture> passthrough_texture;

public:
	static void init(Ref<Window> main_window);
	static void destroy();

	static void waitIdle();
	static VkDevice getDevice();
	static VkPhysicalDevice getPhysicalDevice();
	static Ref<RenderPass> getMainRenderPass();
	static glm::vec2 getFramebufferSize();
	static size_t getFramesInFlight();
	static QueueFamilies getQueueFamilies(VkPhysicalDevice device);
	static VkQueue getGraphicsQueue();
	static VkCommandPool getCommandPool();
	static VkDescriptorPool getDescriptorPool();
	static VkDescriptorSetLayout getSceneDescriptorSetLayout();
	static VkDescriptorSetLayout getObjectDescriptorSetLayout();
	static std::pair<Ref<Texture>, Ref<Sampler>> getDefaultTextureSampler();
	static Ref<Material> getGizmoMaterial();
	static Ref<Mesh> getGizmoMesh(int type);
	static Ref<Material> getDefaultMaterial();
	static Ref<Mesh> getQuad();
	static Ref<Mesh> getSkyboxCube();
	static Ref<Material> getSkyboxMaterial();

	static FrameStats draw();
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

	FrameStats drawFrame();
	void resizeSwapchain();

	void recordRenderCommands(VkCommandBuffer command_buffer, uint32_t image_index, FrameStats& stats);
};

}
