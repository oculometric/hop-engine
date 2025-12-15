#pragma once

#include <vulkan/vulkan.hpp>

#include "common.h"

namespace HopEngine
{

struct PipelineBuilder
{ // TODO: stencil support
	VkCullModeFlags culling_mode = VK_CULL_MODE_BACK_BIT;
	VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
	VkBool32 depth_write_enable = VK_TRUE;
	VkBool32 depth_test_enable = VK_TRUE;
	VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS;

	inline PipelineBuilder cullMode(VkCullModeFlags value) { culling_mode = value; return *this; }
	inline PipelineBuilder polygonMode(VkPolygonMode value) { polygon_mode = value; return *this; }
	inline PipelineBuilder depthWrite(VkBool32 value) { depth_write_enable = value; return *this; }
	inline PipelineBuilder depthTest(VkBool32 value) { depth_test_enable = value; return *this; }
	inline PipelineBuilder depthOp(VkCompareOp value) { depth_compare_op = value; return *this; }
};

class Pipeline
{
private:
	VkPipeline pipeline = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Pipeline);

	Pipeline(Ref<Shader> shader, PipelineBuilder config, Ref<RenderPass> render_pass);
	~Pipeline();

	inline VkPipeline getPipeline() const { return pipeline; }
};

}
