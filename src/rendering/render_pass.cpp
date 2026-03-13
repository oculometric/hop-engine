#include "render_pass.h"

#include <array>
#include <vulkan/vulkan.hpp>

#include "command_buffer.h"
#include "render_server.h"
#include "swapchain.h"
#include "texture.h"
#include "vulkan_converters.h"

using namespace HopEngine;
using namespace std;

RenderPass::RenderPass(const Ref<Swapchain>& _swapchain, const Config& config)
{
    output_config = config;
    swapchain = _swapchain;
    extent = swapchain->getExtent();

    createRenderPass();
    createResources();

    DBG_VERBOSE(string("created render pass with colour buffer, ") + (config.has_depth_attachment ? "depth buffer, " : "") + "and " + ::to_string(config.additional_attachments) + " data attachments");
}

RenderPass::RenderPass(const glm::u32vec2 image_extent, const Config& config)
{
    output_config = config;
    extent = image_extent;

    createRenderPass();
    createResources();
}

RenderPass::~RenderPass()
{
    DBG_VERBOSE("destroying render pass " + PTR(this));
    RenderServer::waitIdle();
    destroyResources();
    vkDestroyRenderPass(RenderServer::getDevice(), render_pass, nullptr);
}

Ref<Texture> RenderPass::getImage(const size_t attachment) const
{
    if (attachment < textures.size())
        return textures[attachment];
    return nullptr;
}

vector<VkClearValue> RenderPass::getClearValues() const
{
    vector<VkClearValue> values = { { VkClearColorValue{{1.0f, 0.0f, 1.0f, 1.0f}} } };
    for (size_t i = 0; i < output_config.additional_attachments; ++i)
        values.push_back({ VkClearColorValue{{0.0f, 0.0f, 0.0f, 0.0f}} });
    if (output_config.has_depth_attachment)
    {
        VkClearValue clear_value;
        clear_value.depthStencil.depth = 1.0f;
        clear_value.depthStencil.stencil = 0;
        values.push_back(clear_value);
    }

    return values;
}

bool RenderPass::isCompatible(const WeakRef<RenderPass>& other) const
{
    if (other->output_config.has_depth_attachment != output_config.has_depth_attachment)
        return false;
    if (other->output_config.additional_attachments != output_config.additional_attachments)
        return false;
    return true;
}

Ref<RenderPass> RenderPass::duplicate() const
{
    if (swapchain)
    {
        DBG_ERROR("cannot duplicate render pass which draws to the swapchain");
        return nullptr;
    }
    return new RenderPass{ extent, output_config };
}

void RenderPass::resize(const glm::u32vec2 new_extent)
{
    RenderServer::waitIdle();
    if (swapchain)
        extent = swapchain->getExtent();
    else
        extent = new_extent;
    destroyResources();
    createResources();
}

void RenderPass::begin(WeakRef<DrawCommandBuffer> command_buffer, glm::vec3 clear_colour)
{
    command_buffer->startRenderPassInternal(render_pass, framebuffers[command_buffer->getImageIndex() % framebuffers.size()], extent, getClearValues(), clear_colour);
}

void RenderPass::createRenderPass()
{
    const Texture::Format main_colour_format = swapchain ? swapchain->getFormat() : Texture::FORMAT_SRGB_8X4;
    const Texture::Layout final_main_colour_layout = swapchain ? Texture::LAYOUT_PRESENT_SRC : Texture::LAYOUT_SHADER_READ_ONLY;
    const bool readable_output = swapchain ? false : true;

    vector<VkAttachmentDescription> attachments;
    VkAttachmentDescription colour_attachment{ };
    colour_attachment.format = toVulkanFormat(main_colour_format);
    colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment.finalLayout = toVulkanLayout(final_main_colour_layout);
    attachments.push_back(colour_attachment);

    for (size_t i = 0; i < output_config.additional_attachments; ++i)
    {
        VkAttachmentDescription attachment{ };
        attachment.format = toVulkanFormat(Texture::FORMAT_FLOAT_16X4);
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = readable_output ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
    }

    if (output_config.has_depth_attachment)
    {
        VkAttachmentDescription depth_attachment{ };
        depth_attachment.format = toVulkanFormat(Texture::FORMAT_DEPTH);
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = readable_output ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        attachments.push_back(depth_attachment);
    }

    vector<VkAttachmentReference> attachment_refs;
    VkAttachmentReference colour_attachment_ref{ };
    colour_attachment_ref.attachment = 0;
    colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_refs.push_back(colour_attachment_ref);

    for (size_t i = 0; i < output_config.additional_attachments; ++i)
    {
        VkAttachmentReference attachment_ref{ };
        attachment_ref.attachment = static_cast<uint32_t>(i + 1);
        attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_refs.push_back(attachment_ref);
    }

    VkAttachmentReference depth_attachment_ref{ };
    if (output_config.has_depth_attachment)
    {
        depth_attachment_ref.attachment = static_cast<uint32_t>(output_config.additional_attachments + 1);
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass{ };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(attachment_refs.size());
    subpass.pColorAttachments = attachment_refs.data();
    if (output_config.has_depth_attachment)
        subpass.pDepthStencilAttachment = &depth_attachment_ref;

    vector<VkSubpassDependency> dependencies;
    VkSubpassDependency dependency{ };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (output_config.has_depth_attachment)
    {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    dependencies.push_back(dependency);

    if (readable_output)
    {
        VkSubpassDependency dependency2{ };
        dependency2.srcSubpass = 0;
        dependency2.dstSubpass = VK_SUBPASS_EXTERNAL;
        dependency2.srcStageMask = dependency.dstStageMask;
        dependency2.srcAccessMask = dependency.dstAccessMask;
        dependency2.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        dependency2.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies.push_back(dependency2);
    }

    VkRenderPassCreateInfo render_pass_create_info{ };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments = attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
    render_pass_create_info.pDependencies = dependencies.data();

    if (vkCreateRenderPass(RenderServer::getDevice(), &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
        DBG_FAULT("vkCreateRenderPass failed");
}

void RenderPass::createResources()
{
    // create texture buffers to back everything
    if (!swapchain)
        textures.push_back(new Texture({ extent.x, extent.y, 1 }, Texture::FORMAT_SRGB_8X4));
    for (size_t i = 0; i < output_config.additional_attachments; ++i)
        textures.push_back(new Texture({ extent.x, extent.y, 1 }, Texture::FORMAT_FLOAT_16X4));
    if (output_config.has_depth_attachment)
        textures.push_back(new Texture({ extent.x, extent.y, 1 }, Texture::FORMAT_DEPTH));

    // create framebuffers to actually render into
    if (swapchain)
        framebuffers.resize(swapchain->getImageCount());
    else
        framebuffers.resize(1);
    for (size_t i = 0; i < framebuffers.size(); ++i)
    {
        vector<VkImageView> image_attachments;
        if (swapchain)
            image_attachments.push_back(swapchain->getImage(i));
        for (const auto& image : textures)
            image_attachments.push_back(image->getView());

        VkFramebufferCreateInfo framebuffer_create_info{ };
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(image_attachments.size());
        framebuffer_create_info.pAttachments = image_attachments.data();
        framebuffer_create_info.width = extent.x;
        framebuffer_create_info.height = extent.y;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(RenderServer::getDevice(), &framebuffer_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
            DBG_FAULT("vkCreateFramebuffer failed");
    }
}

void RenderPass::destroyResources()
{
    RenderServer::waitIdle();
    for (const VkFramebuffer framebuffer : framebuffers)
        vkDestroyFramebuffer(RenderServer::getDevice(), framebuffer, nullptr);
    textures.clear();
}
