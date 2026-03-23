#pragma once

#include <optional>
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <set>

#include "common.h"
#include "vulkan_typedefs.h"
#include "frame_stats.h"

// if uncommented, specifies that the vulkan debug and validation systems should
// be enabled. should be disabled in release builds since the target machine is 
// unlikely to have the vulkan SDK installed
//#define VK_DEBUG

struct GLFWwindow;

namespace HopEngine
{

/**
 * @brief encapsulates all the behaviour of actually initialising and running the
 * rendering environment.
 */
class RenderServer final
{
public:
	struct SceneRender final
	{
		Ref<Scene> scene;
		glm::vec2 start_uv;
		glm::vec2 size_uv;
	};
	
	struct QueueFamilies final
	{
		std::optional<uint32_t> graphics_family;
		std::optional<uint32_t> present_family;
	};

private:
	// main window that the render server will create a surface for
	GLFWwindow* window;
	glm::u32vec2 window_size = { 1024, 1024 };
    glm::u32vec2 size_before_fullscreen = { 1024, 1024 };

	// surface for rendering into the attached window
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	// handle for the vulkan API instance itself
	VkInstance instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
	// the physical device selected, which the logical device is derived from
	VkPhysicalDevice physical_device = VK_NULL_HANDLE;
	// logical vulkan device handle. everything goes through here
	VkDevice device = VK_NULL_HANDLE;
	VkQueue graphics_queue = VK_NULL_HANDLE;
	VkQueue present_queue = VK_NULL_HANDLE;
	VkCommandPool command_pool = VK_NULL_HANDLE;
	std::vector<Ref<DrawCommandBuffer>> command_buffers;

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
	Ref<Texture> default_3d_image;
	Ref<Sampler> default_sampler;	// default sampler used when none is specified
	Ref<Material> default_material;	// default material used when none is specified

	Ref<Mesh> skybox_cube;			// mesh used to render skyboxes
	Ref<Mesh> quad;					// full screen quad mesh
    Ref<Mesh> tri;
    
    Ref<Material> debug_text_material;
    Ref<Mesh> debug_text_mesh;
    Ref<Font> debug_text_font;
	
	Ref<UniformBlock> final_pass_uniforms;
    Ref<Material> spinner_material;
    Ref<UniformBlock> spinner_uniforms;
	std::vector<SceneRender> scenes;

	bool fullscreen = false;
	bool wants_fullscreen_update = false;
	bool vsync = true;
	bool wants_vsync_update = false;
    bool overlay_logs = false;

    std::vector<std::function<void()>> free_list;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(RenderServer);
	
	static void init();
	static void destroy();

	static VkDevice getDevice() { return getInstance()->device; }
	static void waitIdle();
	static VkPhysicalDevice getPhysicalDevice() { return getInstance()->physical_device; }
    static VkSurfaceKHR getSurface() { return getInstance()->surface; }
	static QueueFamilies getQueueFamilies(VkPhysicalDevice device); // TODO: make this cached/internal
	static VkQueue getGraphicsQueue() { return getInstance()->graphics_queue; }
	static VkQueue getPresentQueue() { return getInstance()->present_queue; }
	static VkCommandPool getCommandPool() { return getInstance()->command_pool; }
	static VkDescriptorPool getDescriptorPool() { return getInstance()->descriptor_pool; }
	static Ref<UniformBlock> createSceneUniforms();
	static Ref<UniformBlock> createObjectUniforms();
	static VkPipelineLayout createPipelineLayout(VkDescriptorSetLayout set_2);

    static void free(std::function<void()> destructor);
    static void free(VkBuffer& resource);
    static void free(VkDescriptorSetLayout& resource);
    static void free(VkDescriptorSet& resource);
    static void free(VkDeviceMemory& resource);
    static void free(VkImage& resource);
    static void free(VkImageView& resource);
    static void free(VkPipelineLayout& resource);
    static void free(VkPipeline& resource);
    static void free(VkShaderModule& resource);
    static void free(VkSampler& resource);
    static void free(VkFramebuffer& resource);
    static void free(VkCommandBuffer& resource);
    static void free(VkQueryPool& resource);
    static void free(VkRenderPass& resource);
	
	static WeakRef<RenderPass> getMainRenderPass() { return getInstance()->offscreen_pass; }
	static WeakRef<RenderPass> getFinalRenderPass() { return getInstance()->final_render_pass; }
	static WeakRef<Texture> getDefaultTexture() { return getInstance()->default_image; }
	static WeakRef<Texture> getDefault3DTexture() { return getInstance()->default_3d_image; }
	static WeakRef<Sampler> getDefaultSampler() { return getInstance()->default_sampler; }
	static WeakRef<Mesh> getSkyboxCube() { return getInstance()->skybox_cube; }
	static WeakRef<Mesh> getQuad() { return getInstance()->quad; }
	
	static GLFWwindow* getWindow() { return getInstance()->window; }
	static glm::vec2 getFramebufferSize();
	static bool getWindowShouldClose();
	static void setTitle(const std::string& title);
	static void setVisible(bool visible);
	static void setIcon(const std::string& path);
    static void setBorderless(bool borderless);

	static void setVsyncEnabled(bool enabled);
	static bool getVsyncEnabled();
	static void setFullscreenEnabled(bool enabled);
	static bool getFullscreenEnabled();
    static void setOverlayLogs(bool enabled) { getInstance()->overlay_logs = enabled; }
    static bool getOverlayLogs() { return getInstance()->overlay_logs; }

	static void setSingleScene(const Ref<Scene>& scene);
	static void setMultiScene(const std::vector<SceneRender>& multi_scenes);
	
	static FrameStats draw() { return getInstance()->drawFrame(); }
	
private:
	RenderServer();
	~RenderServer();

	static RenderServer* getInstance();
	
	void createWindow();
	void createVulkan();
	void initImGui();
	bool resize(bool force_resize = false);
    void updateTextMesh();
    void tryFreeResources(bool force = false);
	FrameStats drawFrame();
	void destroyImGui();
	void destroyVulkan();
	void destroyWindow();

	void recordRenderCommands(uint32_t image_index, FrameStats& stats);
};

}
