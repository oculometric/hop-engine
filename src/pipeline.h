#pragma once

#include <vulkan/vulkan.hpp>

#include "common.h"

namespace HopEngine
{

class Pipeline
{
private:
	VkPipeline pipeline = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Pipeline);

	Pipeline(Ref<Shader> shader, VkCullModeFlags culling_mode, VkPolygonMode polygon_mode, 
		VkBool32 depth_write_enable, VkBool32 depth_test_enable, VkCompareOp depth_compare_op, Ref<RenderPass> render_pass);
	~Pipeline();

	inline VkPipeline getPipeline() { return pipeline; }
};

}
