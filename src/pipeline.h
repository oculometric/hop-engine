#pragma once

#include <vulkan/vulkan.h>

#include "common.h"

namespace HopEngine
{

class Shader;
class RenderPass;

class Pipeline
{
private:
	VkPipeline pipeline = VK_NULL_HANDLE;

public:
	DELETE_CONSTRUCTORS(Pipeline);

	Pipeline(Ref<Shader> shader, VkCullModeFlags culling_mode, VkPolygonMode polygon_mode, Ref<RenderPass> render_pass);
	~Pipeline();

	inline VkPipeline getPipeline() { return pipeline; }
};

}
