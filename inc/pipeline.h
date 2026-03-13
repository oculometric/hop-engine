#pragma once

#include "common.h"
#include "vulkan_typedefs.h"

namespace HopEngine
{

class Pipeline final : public Destructible
{
public:
	/**
	 * @brief enumerates mesh face culling mode
	 */
	enum CullMode
	{
		CULL_NONE = 0,
		CULL_FRONT = 1,
		CULL_BACK = 2,
		CULL_BOTH = 3
	};

	/**
	 * @brief enumerates polygon drawing mode
	 */
	enum PolygonMode
	{
		POLYGON_FILL,
		POLYGON_LINE,
		POLYGON_POINT
	};

	/**
	 * @brief enumerates comparison operations
	 */
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

	struct Builder
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

		Builder& cullMode(const CullMode value) { culling_mode = value; return *this; }
		Builder& polygonMode(const PolygonMode value) { polygon_mode = value; return *this; }
		Builder& depthWrite(const bool value) { depth_write_enable = value; return *this; }
		Builder& depthTest(const bool value) { depth_test_enable = value; return *this; }
		Builder& depthOp(const CompareOp value) { depth_compare_op = value; return *this; }
		Builder& stencil() { stencil_enable = true; return *this; }
		Builder& stencilCompare(const CompareOp value, const uint32_t compare_value, const uint32_t compare_mask = 0xFFFFFFFF)
		{ stencil_enable = true; stencil_compare_op = value; stencil_compare_value = compare_value; stencil_compare_mask = compare_mask;  return *this; }
		Builder& stencilWrite(const uint32_t value) { stencil_enable = true; stencil_write = value; return *this; }
	};

private:
	VkPipeline pipeline = VK_NULL_HANDLE;
	Builder pipeline_config;

public:
	DELETE_CONSTRUCTORS(Pipeline);
	Pipeline(const Ref<Shader>& shader, const Builder& config, const Ref<RenderPass>& render_pass);
	~Pipeline() override;
	
	void bind(WeakRef<DrawCommandBuffer> command_buffer);
	Builder getConfig() const { return pipeline_config; }
};

TO_STRING_DEC(Pipeline::CompareOp);
TO_STRING_DEC(Pipeline::PolygonMode);
TO_STRING_DEC(Pipeline::CullMode);

}
