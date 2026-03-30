#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "vulkan_typedefs.h"
#include "texture.h"

namespace HopEngine
{

class Swapchain final : public Destructible
{
public:
	struct SupportInfo final
	{
		std::vector<VkSurfaceCapabilitiesKHR> surface_capabilities;
		std::vector<VkSurfaceFormatKHR> surface_formats;
		// TODO: re-introduce checking for FIFO and IMMEDIATE present mode support
	};

private:
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	Texture::Format format;
	glm::u32vec2 extent;
    bool vsync_enabled = true;
	std::vector<VkImage> images;
	std::vector<VkImageView> image_views;
    std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;
    size_t frame_index = -1;

public:
	DELETE_CONSTRUCTORS(Swapchain);
	Swapchain(glm::u32vec2 new_extent);
	~Swapchain() override;
	
	static SupportInfo getSwapchainSupportInfo(VkPhysicalDevice device);
	static glm::u32vec2 computeActualExtent(glm::u32vec2 extent);
	static uint32_t computeImageCount();
	
	uint32_t getImageCount() const { return static_cast<uint32_t>(image_views.size()); }
	VkImageView getImageView(size_t i) const { return image_views[i]; }
	Texture::Format getFormat() const { return format; }
	glm::u32vec2 getExtent() const { return extent; }

    uint32_t acquireNextImage();
    bool submitCommands(WeakRef<DrawCommandBuffer> command_buffer, uint32_t image_index);
	void resize(glm::u32vec2 new_extent);
	void setVsync(bool enabled);
	bool getVsync();

private:
    void createSwapchain();
	void createImageViews();
    void createSyncObjects();
	void destroyResources();
};

class RenderPass final : public Destructible
{
public:
	struct Config final
	{
		size_t additional_attachments = 0;
		bool has_depth_attachment = true;
		Texture::Format main_colour_format = Texture::FORMAT_FLOAT_16X4;
	};

private:
	VkRenderPass render_pass = VK_NULL_HANDLE;
	Config output_config;
	std::vector<VkFramebuffer> framebuffers;
	std::vector<Ref<Texture>> textures;
	glm::u32vec2 extent;
	WeakRef<Swapchain> swapchain;

public:
	DELETE_CONSTRUCTORS(RenderPass);
	RenderPass(const WeakRef<Swapchain>& _swapchain, const Config& config);
	RenderPass(glm::u32vec2 image_extent, const Config& config);
	~RenderPass() override;
	
	VkRenderPass getRenderPass() const { return render_pass; }
	Config getOutputConfig() const { return output_config; }
	Ref<Texture> getImage(size_t attachment) const;
	std::vector<VkClearValue> getClearValues() const;
	glm::u32vec2 getExtent() const { return extent; }
	bool isCompatible(const WeakRef<RenderPass>& other) const;
	Ref<RenderPass> duplicate() const;
	void resize(glm::u32vec2 new_extent = { 0, 0 });
	void begin(WeakRef<DrawCommandBuffer> command_buffer, glm::vec3 clear_colour, bool transparent = false);
	
private:
	void createRenderPass();
	void createResources();
	void destroyResources();
};

}
