#include "framebuffer.h"

#include "command_buffer.h"
#include "material.h"
#include "render_server.h"
#include "vulkan_helpers.h"

#include <vulkan/vulkan.hpp>

using namespace HopEngine;

Framebuffer::Framebuffer(glm::u32vec2 resolution, const Config& attachments)
{
    config = attachments;
    extent = resolution;
    createResources();
}

Framebuffer::Framebuffer(WeakRef<Swapchain> _swapchain, const Config& attachments)
{
    swapchain = _swapchain;
    config    = attachments;
    extent    = swapchain->getExtent();
    createResources();
}

Framebuffer::~Framebuffer() { destroyResources(); }

Ref<Texture> Framebuffer::getImage(size_t attachment) const
{
    if (attachment == 0) return framebuffers.begin()->first;
    else if (attachment - 1 < additional_textures.size())
        return additional_textures[attachment - 1];
    else if (attachment - 1 == additional_textures.size())
        return depth_texture;
    return nullptr;
}

Ref<Framebuffer> Framebuffer::duplicate() const { return new Framebuffer(getExtent(), getConfig()); }

void Framebuffer::resize(glm::u32vec2 new_extent)
{
    destroyResources();
    extent = new_extent;
    createResources();
}

bool Framebuffer::isCompatible(const WeakRef<RenderPass>& other) const
{
    return (other->getConfig().has_depth_attachment == getConfig().has_depth_attachment) &&
           (other->getConfig().additional_attachments == getConfig().additional_attachments) &&
           (other->getConfig().main_colour_format == getConfig().main_colour_format);
}

bool Framebuffer::isCompatible(const WeakRef<Material>& other) const
{
    return (other->getRenderPassConfig().has_depth_attachment == getConfig().has_depth_attachment) &&
           (other->getRenderPassConfig().additional_attachments == getConfig().additional_attachments) &&
           (other->getRenderPassConfig().main_colour_format == getConfig().main_colour_format);
}

void Framebuffer::bind(WeakRef<DrawCommandBuffer> command_buffer, Framebuffer::Clear clear_values)
{
    clear_values.additionals.resize(getConfig().additional_attachments, { 0, 0, 0, 1 });
    clear_values.depth_present = getConfig().has_depth_attachment;
    command_buffer->startRenderPassInternal(
        static_cast<VkRenderPass>(RenderServer::getRenderPass(getConfig())),
        getFramebuffer(command_buffer->getImageIndex()), getExtent(), clear_values);
}

void Framebuffer::createResources()
{
    // create texture buffers to back everything
    for (size_t i = 0; i < config.additional_attachments; ++i)
        additional_textures.push_back(new Texture({ extent.x, extent.y, 1 }, Texture::FORMAT_FLOAT_16X4));
    if (config.has_depth_attachment)
        depth_texture = new Texture({ extent.x, extent.y, 1 }, Texture::FORMAT_DEPTH);

    // create framebuffers to actually render into
    framebuffers.resize(swapchain ? swapchain->getImageCount() : 1);
    for (size_t i = 0; i < framebuffers.size(); ++i)
    {
        std::vector<VkImageView> image_attachments;

        Ref<Texture> main_tex;
        if (swapchain)
        {
            image_attachments.push_back(static_cast<VkImageView>(swapchain->getImageView(i)));
        }
        else
        {
            main_tex = new Texture({ extent.x, extent.y, 1 }, config.main_colour_format);
            image_attachments.push_back(static_cast<VkImageView>(main_tex->getView()));
        }
        for (const auto& image : additional_textures)
            image_attachments.push_back(static_cast<VkImageView>(image->getView()));
        if (depth_texture) image_attachments.push_back(static_cast<VkImageView>(depth_texture->getView()));

        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType      = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.renderPass = static_cast<VkRenderPass>(RenderServer::getRenderPass(config));
        framebuffer_create_info.attachmentCount = static_cast<uint32_t>(image_attachments.size());
        framebuffer_create_info.pAttachments    = image_attachments.data();
        framebuffer_create_info.width           = extent.x;
        framebuffer_create_info.height          = extent.y;
        framebuffer_create_info.layers          = 1;

        VkFramebuffer fb;
        CHECK_RESULT(vkCreateFramebuffer,
            (static_cast<VkDevice>(RenderServer::getDevice()), &framebuffer_create_info, nullptr, &fb),
            FAULT,
            ;);

        framebuffers[i] = { main_tex, fb };
    }
}

void Framebuffer::destroyResources()
{
    for (auto [tex, framebuffer] : framebuffers)
        QUEUE_FREE(framebuffer, VkFramebuffer, vkDestroyFramebuffer);
    framebuffers.clear();
    additional_textures.clear();
    depth_texture = nullptr;
}

GPUHandle Framebuffer::getFramebuffer(uint32_t index) const
{ return framebuffers[index % framebuffers.size()].second; }
