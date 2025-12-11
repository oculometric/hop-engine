#include "command_buffer.h"

#include "graphics_environment.h"

using namespace HopEngine;
using namespace std;

CommandBuffer::CommandBuffer()
{
    VkCommandBufferAllocateInfo allocate_info{ };
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool = RenderServer::get()->getCommandPool();
    allocate_info.commandBufferCount = 1;

    vkAllocateCommandBuffers(RenderServer::getDevice(), &allocate_info, &buffer);

    VkCommandBufferBeginInfo begin_info{ };
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(buffer, &begin_info);
    DBG_BABBLE("started transient command buffer " + PTR(this));
}

CommandBuffer::~CommandBuffer()
{
    submit();
    DBG_BABBLE("destroying command buffer " + PTR(this));
    vkFreeCommandBuffers(RenderServer::getDevice(), RenderServer::get()->getCommandPool(), 1, &buffer);
}

void CommandBuffer::submit()
{
    if (already_submitted)
        return;
    already_submitted = true;

    DBG_BABBLE("submitting transient command buffer " + PTR(this));
    vkEndCommandBuffer(buffer);

    VkSubmitInfo submit_info{ };
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;

    vkQueueSubmit(RenderServer::get()->getGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(RenderServer::get()->getGraphicsQueue());
}
