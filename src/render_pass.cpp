#include "render_pass.h"

#include <stdexcept>

#include "graphics_environment.h"
#include "swapchain.h"

using namespace HopEngine;
using namespace std;

RenderPass::RenderPass(Swapchain* swapchain)
{
    VkAttachmentDescription colour_attachment{ };
    colour_attachment.format = swapchain->getFormat();
    colour_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colour_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colour_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colour_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colour_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colour_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colour_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colour_attachment_ref{ };
    colour_attachment_ref.attachment = 0;
    colour_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{ };
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = &colour_attachment_ref;
    subpass.colorAttachmentCount = 1;

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.attachmentCount = 1;
    render_pass_create_info.pAttachments = &colour_attachment;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass;

    if (vkCreateRenderPass(GraphicsEnvironment::get()->getDevice(), &render_pass_create_info, nullptr, &render_pass) != VK_SUCCESS)
        throw runtime_error("vkCreateRenderPass failed");
}

RenderPass::~RenderPass()
{
    vkDestroyRenderPass(GraphicsEnvironment::get()->getDevice(), render_pass, nullptr);
}
