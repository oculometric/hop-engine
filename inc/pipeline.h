#pragma once

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

enum CullMode
{
	CULL_NONE = 0,
	CULL_FRONT = 1,
	CULL_BACK = 2,
	CULL_BOTH = 3
};
TO_STRING_DEC(CullMode);

enum PolygonMode
{
	POLYGON_FILL,
	POLYGON_LINE,
	POLYGON_POINT
};
TO_STRING_DEC(PolygonMode);

enum CompareOp
{
	COMPARE_NEVER = 0,
	COMPARE_LESS = 1,
	COMPARE_EQUAL = 2,
	COMPARE_LESS_OR_EQUAL = 3,
	COMPARE_GREATER = 4,
	COMPARE_NOT_EQUAL = 5,
	COMPARE_GREATER_OR_EQUAL = 6,
	COMPARE_ALWAYS = 7,
};
TO_STRING_DEC(CompareOp);

struct PipelineBuilder
{
	CullMode culling_mode = CULL_BACK;
	PolygonMode polygon_mode = POLYGON_FILL;
	bool depth_write_enable = true;
	bool depth_test_enable = true;
	CompareOp depth_compare_op = COMPARE_LESS;
	bool stencil_enable = false;
	CompareOp stencil_compare_op = COMPARE_ALWAYS;
	uint32_t stencil_compare_value = 0xFFFFFFFF;
	uint32_t stencil_compare_mask = 0xFFFFFFFF;
	uint32_t stencil_write = 0;

	PipelineBuilder cullMode(const CullMode value) { culling_mode = value; return *this; }
	PipelineBuilder polygonMode(const PolygonMode value) { polygon_mode = value; return *this; }
	PipelineBuilder depthWrite(const bool value) { depth_write_enable = value; return *this; }
	PipelineBuilder depthTest(const bool value) { depth_test_enable = value; return *this; }
	PipelineBuilder depthOp(const CompareOp value) { depth_compare_op = value; return *this; }
	PipelineBuilder stencil() { stencil_enable = true; return *this; }
	PipelineBuilder stencilCompare(const CompareOp value, const uint32_t compare_value, const uint32_t compare_mask = 0xFFFFFFFF)
	{ stencil_enable = true; stencil_compare_op = value; stencil_compare_value = compare_value; stencil_compare_mask = compare_mask;  return *this; }
	PipelineBuilder stencilWrite(const uint32_t value) { stencil_enable = true; stencil_write = value; return *this; }
};

class Pipeline : public Destructible
{
private:
	VkPipeline pipeline = VK_NULL_HANDLE;
	PipelineBuilder pipeline_config;

public:
	DELETE_CONSTRUCTORS(Pipeline);
	Pipeline(const Ref<Shader>& shader, const PipelineBuilder& config, const Ref<RenderPass>& render_pass);
	~Pipeline() override;
	
	VkPipeline getPipeline() const { return pipeline; }
	PipelineBuilder getConfig() const { return pipeline_config; }
};

}
