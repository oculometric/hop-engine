#include "command_buffer.h"

#include <vulkan/vulkan.hpp>

#include "graphics_environment.h"

using namespace HopEngine;
using namespace std;

CommandBuffer::CommandBuffer()
{
    // allocate a vulkan command buffer
    VkCommandBufferAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = RenderServer::getCommandPool();
    allocate_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(RenderServer::getDevice(), &allocate_info, &buffer);

    // automatically start the command buffer so the user can just start issuing co
    VkCommandBufferBeginInfo begin_info{ };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(buffer, &begin_info);
    DBG_BABBLE("started transient command buffer " + PTR(this));
}

CommandBuffer::~CommandBuffer()
{
    if (!already_submitted)
        DBG_WARNING("command buffer " + PTR(this) + " being destructed without being submitted");
    DBG_BABBLE("destroying command buffer " + PTR(this));
    vkFreeCommandBuffers(RenderServer::getDevice(), RenderServer::getCommandPool(), 1, &buffer);
}

void CommandBuffer::submit()
{
    // don't submit if the command buffer has already been submitted
    if (already_submitted)
    {
        DBG_WARNING("attempt to submit command buffer " + PTR(this) + ", but it has already been submitted");
        return;
    }
    already_submitted = true;

    DBG_BABBLE("submitting transient command buffer " + PTR(this));
    vkEndCommandBuffer(buffer);

    VkSubmitInfo submit_info{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;

    // submit and wait for it to be executed
    vkQueueSubmit(RenderServer::getGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(RenderServer::getGraphicsQueue());
}
