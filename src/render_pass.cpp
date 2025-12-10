#include "render_pass.h"

#include <array>
#include <stdexcept>

#include "graphics_environment.h"
#include "swapchain.h"
#include "texture.h"

using namespace HopEngine;
using namespace std;

RenderPass::RenderPass(Ref<Swapchain> swapchain, RenderOutput config)
{
    output_config = config;

    vector<VkAttachmentDescription> attachments;
    VkAttachmentDescription colour_attachment{ };
    colour_attachment.format = swapchain->getFormat();
    colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments.push_back(colour_attachment);

    for (size_t i = 0; i < config.additional_attachments; ++i)
    {
        VkAttachmentDescription attachment{ };
        attachment.format = Texture::data_format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
    }

    if (config.has_depth_attachment)
    {
        VkAttachmentDescription depth_attachment{ };
        depth_attachment.format = Texture::depth_format;
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachments.push_back(depth_attachment);
    }

    vector<VkAttachmentReference> attachment_refs;
    VkAttachmentReference colour_attachment_ref{ };
    colour_attachment_ref.attachment = 0;
    colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_refs.push_back(colour_attachment_ref);

    for (size_t i = 0; i < config.additional_attachments; ++i)
    {
        VkAttachmentReference attachment_ref{ };
        attachment_ref.attachment = static_cast<uint32_t>(i + 1);
        attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment_refs.push_back(attachment_ref);
    }

    VkAttachmentReference depth_attachment_ref{ };
    if (config.has_depth_attachment)
    {
        depth_attachment_ref.attachment = static_cast<uint32_t>(config.additional_attachments + 1);
        depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass{ };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(attachment_refs.size());
    subpass.pColorAttachments = attachment_refs.data();
    if (config.has_depth_attachment)
    {
        subpass.pDepthStencilAttachment = &depth_attachment_ref;
    }

    VkSubpassDependency dependency{ };
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (config.has_depth_attachment)
    {
        dependency.srcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    VkRenderPassCreateInfo render_pass_create_info{ };
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    render_pass_create_info.pAttachments = attachments.data();
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;
    render_pass_create_info.dependencyCount = 1;
    render_pass_create_info.pDependencies = &dependency;

    if (vkCreateRenderPass(GraphicsEnvironment::get()->getDevice(), &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
        DBG_FAULT("vkCreateRenderPass failed");

    createResources(swapchain);

    DBG_INFO(string("created render pass with colour buffer, ") + (config.has_depth_attachment ? "depth buffer, " : "") + "and " + to_string(config.additional_attachments) + " data attachments");
}

RenderPass::~RenderPass()
{
    DBG_INFO("destroying render pass " + PTR(this));
    destroyResources();
    vkDestroyRenderPass(GraphicsEnvironment::get()->getDevice(), render_pass, nullptr);
}

vector<VkClearValue> RenderPass::getClearValues()
{
    vector<VkClearValue> values = { { VkClearColorValue{{1.0f, 0.0f, 1.0f, 1.0f}} } };
    for (size_t i = 0; i < output_config.additional_attachments; ++i)
        values.push_back({ VkClearColorValue{{0.0f, 0.0f, 0.0f, 1.0f}} });
    if (output_config.has_depth_attachment)
        values.push_back({ 1.0f, 0 });

    return values;
}

void RenderPass::resize(Ref<Swapchain> swapchain)
{
    destroyResources();

    createResources(swapchain);
}

void RenderPass::destroyResources()
{
    for (VkFramebuffer framebuffer : framebuffers)
        vkDestroyFramebuffer(GraphicsEnvironment::get()->getDevice(), framebuffer, nullptr);
    depth_texture = nullptr;
    additional_textures.clear();
}

void RenderPass::createResources(Ref<Swapchain> swapchain)
{
    // create texture buffers to back everything
    auto texture_extent = swapchain->getExtent();
    if (output_config.has_depth_attachment)
        depth_texture = new Texture(texture_extent.width, texture_extent.height, Texture::depth_format, nullptr);
    for (size_t i = 0; i < output_config.additional_attachments; ++i)
        additional_textures.push_back(new Texture(texture_extent.width, texture_extent.height, Texture::data_format, nullptr));

    // create framebuffers to actually render into
    framebuffers.resize(swapchain->getImageCount());
    for (size_t i = 0; i < framebuffers.size(); ++i)
    {
        vector<VkImageView> image_attachments = { swapchain->getImage(i) };
        for (size_t i = 0; i < output_config.additional_attachments; ++i)
            image_attachments.push_back(additional_textures[i]->getView());
        if (output_config.has_depth_attachment)
            image_attachments.push_back(depth_texture->getView());

        VkFramebufferCreateInfo framebuffer_create_info{ };
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(image_attachments.size());
        framebuffer_create_info.pAttachments = image_attachments.data();
        framebuffer_create_info.width = swapchain->getExtent().width;
        framebuffer_create_info.height = swapchain->getExtent().height;
        framebuffer_create_info.layers = 1;

        if (vkCreateFramebuffer(GraphicsEnvironment::get()->getDevice(), &framebuffer_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
            DBG_FAULT("vkCreateFramebuffer failed");
    }
}
