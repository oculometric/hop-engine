#include "command_buffer.h"
#include "material.h"
#include "render_server.h"
#include "swapchain.h"
#include "vulkan_helpers.h"

#include <array>
#include <vulkan/vulkan.hpp>

using namespace HopEngine;

TO_STRING_IMPL(Pipeline::CullMode, 4, VARGS("NONE", "FRONT", "BACK", "BOTH"));

TO_STRING_IMPL(Pipeline::PolygonMode, 3, VARGS("FILL", "LINE", "BACK"));

TO_STRING_IMPL(Pipeline::CompareOp, 8,
    VARGS("NEVER", "LESS", "EQUAL", "LESS_EQUAL", "GREATER", "NOT_EQUAL", "GREATER_EQUAL", "ALWAYS"));

Pipeline::Pipeline(const Ref<Shader>& shader, const Builder& config, const Ref<RenderPass>& render_pass)
{
    pipeline_config                              = config;
    std::array<VkDynamicState, 2> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
    dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_state_create_info.pDynamicStates    = dynamic_states.data();

    VkPipelineVertexInputStateCreateInfo vertex_input_create_info{};
    vertex_input_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    auto binding_description       = getVertexBindingDescription();
    vertex_input_create_info.vertexBindingDescriptionCount = 1;
    vertex_input_create_info.pVertexBindingDescriptions    = &binding_description;
    auto attribute_descriptions                            = getVertexAttributeDescriptions();
    vertex_input_create_info.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(attribute_descriptions.size());
    vertex_input_create_info.pVertexAttributeDescriptions = attribute_descriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info{};
    input_assembly_create_info.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_create_info.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode             = static_cast<VkPolygonMode>(config.polygon_mode);
    rasterizer.lineWidth               = 1.0f;
    rasterizer.cullMode                = config.culling_mode;
    rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depth{};
    depth.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth.depthCompareOp        = static_cast<VkCompareOp>(config.depth_compare_op);
    depth.depthWriteEnable      = config.depth_write_enable;
    depth.depthTestEnable       = config.depth_test_enable;
    depth.depthBoundsTestEnable = VK_FALSE;
    depth.stencilTestEnable     = config.stencil_enable;
    if (config.stencil_enable)
    {
        VkStencilOpState front_and_back{};
        front_and_back.failOp      = VK_STENCIL_OP_KEEP;
        front_and_back.passOp      = VK_STENCIL_OP_REPLACE;
        front_and_back.depthFailOp = VK_STENCIL_OP_KEEP;
        front_and_back.compareOp   = static_cast<VkCompareOp>(config.stencil_compare_op);
        front_and_back.reference   = config.stencil_compare_value;
        front_and_back.compareMask = config.stencil_compare_mask;
        front_and_back.writeMask   = config.stencil_write;
        if (!(config.culling_mode & CULL_FRONT)) depth.front = front_and_back;
        if (!(config.culling_mode & CULL_BACK)) depth.back = front_and_back;
    }

    std::vector<VkPipelineColorBlendAttachmentState> colour_attachment_blends;
    VkPipelineColorBlendAttachmentState colour_blend_attachment{};
    colour_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colour_blend_attachment.blendEnable         = VK_TRUE;
    colour_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colour_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colour_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
    colour_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colour_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colour_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;
    colour_attachment_blends.push_back(colour_blend_attachment);

    auto [additional_attachments, has_depth_attachment, main_colour_format] =
        render_pass->getOutputConfig();
    for (size_t i = 0; i < additional_attachments; ++i)
    {
        VkPipelineColorBlendAttachmentState blend_attachment{};
        blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        blend_attachment.blendEnable    = VK_FALSE;
        colour_attachment_blends.push_back(blend_attachment);
    }

    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable   = VK_FALSE;
    color_blending.attachmentCount = static_cast<uint32_t>(colour_attachment_blends.size());
    color_blending.pAttachments    = colour_attachment_blends.data();

    auto shader_stages = shader->getShaderStages();
    std::vector<VkPipelineShaderStageCreateInfo> stage_create_infos;
    for (const auto& stage : shader_stages)
    {
        VkPipelineShaderStageCreateInfo info{};
        info.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.module = static_cast<VkShaderModule>(stage.second);
        info.pName  = "main";
        switch (stage.first)
        {
        case Shader::STAGE_VERTEX:   info.stage = VK_SHADER_STAGE_VERTEX_BIT; break;
        case Shader::STAGE_FRAGMENT: info.stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
        }
        stage_create_infos.push_back(info);
    }

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount          = static_cast<uint32_t>(stage_create_infos.size());
    pipeline_create_info.pStages             = stage_create_infos.data();
    pipeline_create_info.pVertexInputState   = &vertex_input_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
    pipeline_create_info.pViewportState      = &viewport_state;
    pipeline_create_info.pRasterizationState = &rasterizer;
    pipeline_create_info.pMultisampleState   = &multisampling;
    pipeline_create_info.pDepthStencilState  = &depth;
    pipeline_create_info.pColorBlendState    = &color_blending;
    pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
    pipeline_create_info.layout              = static_cast<VkPipelineLayout>(shader->getPipelineLayout());
    pipeline_create_info.renderPass          = static_cast<VkRenderPass>(render_pass->getRenderPass());
    pipeline_create_info.subpass             = 0;

    CHECK_RESULT(vkCreateGraphicsPipelines,
        (static_cast<VkDevice>(RenderServer::getDevice()), VK_NULL_HANDLE, 1, &pipeline_create_info,
            nullptr, reinterpret_cast<VkPipeline*>(&pipeline)),
        FAULT,
        ;);

    DBG_VERBOSE("created pipeline for shader " + PTR(shader.get()));
}

Pipeline::~Pipeline()
{
    DBG_VERBOSE("destroying pipeline " + PTR(this));
    QUEUE_FREE(pipeline, VkPipeline, vkDestroyPipeline);
}

void Pipeline::bind(WeakRef<DrawCommandBuffer> command_buffer)
{ command_buffer->bindPipelineInternal(pipeline); }
