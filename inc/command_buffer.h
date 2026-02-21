#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <vector>
#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

/**
 * @brief represents a transient GPU command buffer to allow immediate command execution
 */
class TransientCommandBuffer : public Destructible
{
private:
	VkCommandBuffer buffer = VK_NULL_HANDLE;	// GPU command buffer handle
	bool already_submitted = false;				// whether the command buffer has been submitted

public:
	DELETE_NOT_ALL_CONSTRUCTORS(TransientCommandBuffer);
	TransientCommandBuffer();
	~TransientCommandBuffer() override;
	
	VkCommandBuffer getBuffer() const { return buffer; }
	/**
	 * @brief causes the command buffer to be submitted to the graphics queue, and waits
	 * for completion before returning.
	 */
	void submit();
};

typedef void* GPUHandle;
struct FrameStats;

class DrawCommandBuffer : public Destructible
{
private:
	VkCommandBuffer buffer = VK_NULL_HANDLE;
	/* hawk tuah - els */
	// the query pool allows us to pull useful frame stats (like timings) from the GPU
	VkQueryPool query_pool = VK_NULL_HANDLE;
	uint32_t query_offset = 0;
	bool begun = false;
	bool submitted = false;
	uint32_t image_index = 0;
	FrameStats* stats = nullptr;
	
	GPUHandle current_render_pass = nullptr;
	GPUHandle current_descriptor_sets[3] = { nullptr };
	GPUHandle current_pipeline_layout = nullptr;
	GPUHandle current_pipeline = nullptr;
	GPUHandle current_vertex_buffer = nullptr;
	GPUHandle current_index_buffer = nullptr;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(DrawCommandBuffer);
	DrawCommandBuffer();
	~DrawCommandBuffer() override;
	
	void begin(uint32_t index, FrameStats* frame_stats);
	
	uint32_t getImageIndex() const { return image_index; }
	VkCommandBuffer getCommandBuffer() const { return buffer; }
	
	// these get called by different assets (render pass, material, shader, object, etc)
	void startRenderPassInternal(GPUHandle render_pass, GPUHandle framebuffer, glm::u32vec2 extent, std::vector<VkClearValue> clear_values, glm::vec3 clear_colour);
	void bindPipelineInternal(GPUHandle pipeline);
	void bindPipelineLayoutInternal(GPUHandle pipeline_layout);
	void bindDescriptorSetInternal(size_t set, GPUHandle descriptor_set);
	void bindVertexBuffer(GPUHandle vertex_buffer);
	void bindIndexBuffer(GPUHandle index_buffer);
	void setScissorViewport(glm::vec2 offset, glm::vec2 size, glm::u32vec2 framebuffer_extent);
	void drawMeshInternal(size_t indices);
	void drawImGui();
	void extractTiming();
	
	void end();
	
private:
	void writeTimestamp(bool bottom_of_pipe);
};

}
