#include "render_pass.h"

#include <array>
#include <stdexcept>

#include "graphics_environment.h"
#include "swapchain.h"
#include "texture.h"

using namespace HopEngine;
using namespace std;

RenderPass::RenderPass(Ref<Swapchain> _swapchain, RenderOutput config)
{
    output_config = config;
    swapchain = _swapchain;

    createRenderPass(swapchain->getFormat(), VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, false);

    createResources(swapchain);

    DBG_INFO(string("created render pass with colour buffer, ") + (config.has_depth_attachment ? "depth buffer, " : "") + "and " + to_string(config.additional_attachments) + " data attachments");
}

RenderPass::RenderPass(uint32_t width, uint32_t height, RenderOutput config)
{
    output_config = config;

    createRenderPass(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true);
    createResources(VK_FORMAT_R8G8B8A8_SRGB, width, height);
}

RenderPass::~RenderPass()
{
    DBG_INFO("destroying render pass " + PTR(this));
    destroyResources();
    vkDestroyRenderPass(RenderServer::get()->getDevice(), render_pass, nullptr);
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

void RenderPass::resize(uint32_t width, uint32_t height)
{
    destroyResources();
    if (swapchain.isValid())
        createResources(swapchain);
    else
        createResources(VK_FORMAT_R8G8B8A8_SRGB, width, height);
}

Ref<Texture> RenderPass::getImage(size_t attachment)
{
    if (attachment < additional_textures.size())
        return additional_textures[attachment];
    else if (attachment == additional_textures.size())
        return depth_texture;
    else
        return nullptr;
}

void RenderPass::createRenderPass(VkFormat main_colour_format, VkImageLayout final_main_colour_layout, bool make_readable)
{
    vector<VkAttachmentDescription> attachments;
    VkAttachmentDescription colour_attachment{ };
    colour_attachment.format = main_colour_format;
    colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment.finalLayout = final_main_colour_layout;
    attachments.push_back(colour_attachment);

    for (size_t i = 0; i < output_config.additional_attachments; ++i)
    {
        VkAttachmentDescription attachment{ };
        attachment.format = Texture::data_format;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = make_readable ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments.push_back(attachment);
    }

    if (output_config.has_depth_attachment)
    {
        VkAttachmentDescription depth_attachment{ };
        depth_attachment.format = Texture::depth_format;
        depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout = make_readable ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
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

    if (make_readable)
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

    if (vkCreateRenderPass(RenderServer::get()->getDevice(), &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
        DBG_FAULT("vkCreateRenderPass failed");
}

void RenderPass::destroyResources()
{
    for (VkFramebuffer framebuffer : framebuffers)
        vkDestroyFramebuffer(RenderServer::get()->getDevice(), framebuffer, nullptr);
    depth_texture = nullptr;
    additional_textures.clear();
}

void RenderPass::createResources(Ref<Swapchain> swapchain)
{
    // create texture buffers to back everything
    auto texture_extent = swapchain->getExtent();
    extent = texture_extent;
    if (output_config.has_depth_attachment)
        depth_texture = new Texture(texture_extent.width, texture_extent.height, Texture::depth_format);
    for (size_t i = 0; i < output_config.additional_attachments; ++i)
        additional_textures.push_back(new Texture(texture_extent.width, texture_extent.height, Texture::data_format));

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

        if (vkCreateFramebuffer(RenderServer::get()->getDevice(), &framebuffer_create_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
            DBG_FAULT("vkCreateFramebuffer failed");
    }
}

void RenderPass::createResources(VkFormat main_colour_format, uint32_t width, uint32_t height)
{
    extent = VkExtent2D{ width, height };
    // create texture buffers to back everything
    additional_textures.push_back(new Texture(width, height, main_colour_format, nullptr,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
    if (output_config.has_depth_attachment)
        depth_texture = new Texture(width, height, Texture::depth_format, nullptr,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    for (size_t i = 0; i < output_config.additional_attachments; ++i)
        additional_textures.push_back(new Texture(width, height, Texture::data_format, nullptr,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));

    // create framebuffers to actually render into
    framebuffers.resize(1);
    vector<VkImageView> image_attachments;
    for (size_t i = 0; i < additional_textures.size(); ++i)
        image_attachments.push_back(additional_textures[i]->getView());
    if (output_config.has_depth_attachment)
        image_attachments.push_back(depth_texture->getView());

    VkFramebufferCreateInfo framebuffer_create_info{ };
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.renderPass = render_pass;
    framebuffer_create_info.attachmentCount = static_cast<uint32_t>(image_attachments.size());
    framebuffer_create_info.pAttachments = image_attachments.data();
    framebuffer_create_info.width = width;
    framebuffer_create_info.height = height;
    framebuffer_create_info.layers = 1;

    if (vkCreateFramebuffer(RenderServer::get()->getDevice(), &framebuffer_create_info, nullptr, &framebuffers[0]) != VK_SUCCESS)
        DBG_FAULT("vkCreateFramebuffer failed");
}
