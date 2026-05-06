#include "framebuffer.h"
#include "render_server.h"
#include "vulkan_helpers.h"

#include <array>
#include <vulkan/vulkan.hpp>

using namespace HopEngine;

RenderPass::RenderPass(const Framebuffer::Config& conf)
{
    config = conf;
    createRenderPass();

    DBG_VERBOSE(std::string("created render pass with colour buffer, ") +
                (config.has_depth_attachment ? "depth buffer, " : "") + "and " +
                std::to_string(config.additional_attachments) + " data attachments");
}

RenderPass::~RenderPass()
{
    DBG_VERBOSE("destroying render pass " + PTR(this));
    QUEUE_FREE(render_pass, VkRenderPass, vkDestroyRenderPass);
}

Ref<Framebuffer> RenderPass::createFramebuffer(glm::u32vec2 extent) const
{ return new Framebuffer(extent, getConfig()); }

void RenderPass::createRenderPass()
{
    const Texture::Format main_colour_format = config.main_colour_format;
    const Texture::Layout final_main_colour_layout =
        config.presentable_layout ? Texture::LAYOUT_PRESENT_SRC : Texture::LAYOUT_SHADER_READ;
    const bool readable_output = !config.presentable_layout;

    std::vector<VkAttachmentDescription> attachments;
    VkAttachmentDescription colour_attachment{};
    colour_attachment.format         = toVulkanFormat(main_colour_format);
    colour_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment.finalLayout    = toVulkanLayout(final_main_colour_layout);
    attachments.push_back(colour_attachment);

    for (size_t i = 0; i < config.additional_attachments; ++i)
    {
        VkAttachmentDescription attachment{};
        attachment.format         = toVulkanFormat(Texture::FORMAT_FLOAT_16X4);
        attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout    = readable_output ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                    : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
    }

    if (config.has_depth_attachment)
    {
        VkAttachmentDescription depth_attachment{};
        depth_attachment.format         = toVulkanFormat(Texture::FORMAT_DEPTH);
        depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout    = readable_output ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                                          : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        attachments.push_back(depth_attachment);
    }

    std::vector<VkAttachmentReference> attachment_refs;
    VkAttachmentReference colour_attachment_ref{};
    colour_attachment_ref.attachment = 0;
    colour_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_refs.push_back(colour_attachment_ref);

    for (size_t i = 0; i < config.additional_attachments; ++i)
    {
        VkAttachmentReference attachment_ref{};
        attachment_ref.attachment = static_cast<uint32_t>(i + 1);
        attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_refs.push_back(attachment_ref);
    }

    VkAttachmentReference depth_attachment_ref{};
    if (config.has_depth_attachment)
    {
        depth_attachment_ref.attachment = static_cast<uint32_t>(config.additional_attachments + 1);
        depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(attachment_refs.size());
    subpass.pColorAttachments    = attachment_refs.data();
    if (config.has_depth_attachment) subpass.pDepthStencilAttachment = &depth_attachment_ref;

    std::vector<VkSubpassDependency> dependencies;
    VkSubpassDependency dependency{};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (config.has_depth_attachment)
    {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    dependencies.push_back(dependency);

    if (readable_output)
    {
        VkSubpassDependency dependency2{};
        dependency2.srcSubpass      = 0;
        dependency2.dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependency2.srcStageMask    = dependency.dstStageMask;
        dependency2.srcAccessMask   = dependency.dstAccessMask;
        dependency2.dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency2.dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies.push_back(dependency2);
    }

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments    = attachments.data();
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass;
    render_pass_create_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_create_info.pDependencies   = dependencies.data();

    CHECK_RESULT(vkCreateRenderPass,
        (static_cast<VkDevice>(RenderServer::getDevice()), &render_pass_create_info, nullptr,
            reinterpret_cast<VkRenderPass*>(&render_pass)),
        FAULT,
        ;);
}
