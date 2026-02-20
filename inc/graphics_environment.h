#pragma once

#include <optional>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <set>

#include "common.h"
#include "vulkan_typedefs.h"
#include "engine.h"

// if uncommented, specifies that the vulkan debug and validation systems should
// be enabled. should be disabled in release builds since the target machine is 
// unlikely to have the vulkan SDK installed
//#define VK_DEBUG

namespace HopEngine
{

struct MultiSceneRenderSpec
{
	Ref<Scene> scene;
	glm::vec2 start_uv;
	glm::vec2 size_uv;
};

/**
 * @brief encapsulates all the behaviour of actually initialising and running the
 * rendering environment.
 */
class RenderServer
{
public:
	struct QueueFamilies
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;
	};

private:
	// number of concurrently processed/queued frames. may be adjusted
	// at runtime.
	int MAX_FRAMES_IN_FLIGHT = 2;
	// main window that the render server will create a surface for
	Ref<Window> window;

	// handle for the vulkan API instance itself
	VkInstance instance = VK_NULL_HANDLE;
#if defined(VK_DEBUG)
	VkDebugUtilsMessengerEXT debug_messenger;
#endif
	// the physical device selected, which the logical device is derived from
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	// logical vulkan device handle. everything goes through here
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphics_queue = VK_NULL_HANDLE;
	VkQueue present_queue = VK_NULL_HANDLE;
	VkCommandPool command_pool = VK_NULL_HANDLE;
	std::vector<Ref<DrawCommandBuffer>> command_buffers;
	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
	// surface for rendering into the attached window
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	Ref<Swapchain> swapchain;
	// standard render pass used by all scene camera render passes
    Ref<RenderPass> offscreen_pass;
	// final render pass for actually putting stuff on the window surface
	Ref<RenderPass> final_render_pass;
	
	// pool from which descriptors and descriptor sets are allocated by the rest of
	// the engine
	VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
	// universally used descriptor set layout for set 0, containing scene information
	VkDescriptorSetLayout scene_descriptor_set_layout = VK_NULL_HANDLE;
	// universally used descriptor set layout for set 1, containing object information
	VkDescriptorSetLayout object_descriptor_set_layout = VK_NULL_HANDLE;

	Ref<Texture> default_image;		// default image used when none is specified
	Ref<Sampler> default_sampler;	// default sampler used when none is specified
	Ref<Material> default_material;	// default material used when none is specified

	Ref<Mesh> skybox_cube;			// mesh used to render skyboxes
	Ref<Mesh> quad;					// full screen quad mesh
	
	std::vector<std::pair<MultiSceneRenderSpec, Ref<UniformBlock>>> scenes;

public:
	DELETE_CONSTRUCTORS(RenderServer);
	
	static void init(const Ref<Window>& main_window);
	static void destroy();

	static size_t getFramesInFlight();
	static VkDevice getDevice();
	static VkPhysicalDevice getPhysicalDevice();
	static QueueFamilies getQueueFamilies(VkPhysicalDevice device);
	static VkQueue getGraphicsQueue();
	static VkCommandPool getCommandPool();
	static VkDescriptorPool getDescriptorPool();
	static glm::vec2 getFramebufferSize();
	
	static Ref<RenderPass> getMainRenderPass();
	static Ref<RenderPass> getFinalRenderPass();
	static VkDescriptorSetLayout getSceneDescriptorSetLayout();
	static VkDescriptorSetLayout getObjectDescriptorSetLayout();
	static std::pair<Ref<Texture>, Ref<Sampler>> getDefaultTextureSampler();
	static Ref<Material> getDefaultMaterial();
	static Ref<Mesh> getSkyboxCube();
	static Ref<Mesh> getQuad();
	static void waitIdle();
	static FrameStats draw();
	static void resize();
	
	static void setSingleScene(const Ref<Scene>& scene);
	static void setMultiScene(const std::vector<MultiSceneRenderSpec>& multi_scenes);

private:
	RenderServer(const Ref<Window>& main_window);
	~RenderServer();
	
	void createInstance();
	void createDevice();
	void createDescriptorPoolAndSets();
	void createCommandPool();
	void createSyncObjects();
	void initImGui();

	FrameStats drawFrame();
	void resizeSwapchain();

	void recordRenderCommands(uint32_t image_index, FrameStats& stats);
	void updateUniforms(uint32_t image_index, float time_since_start, FrameStats& stats);
};

}
