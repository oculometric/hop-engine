#include "command_buffer.h"

#include "engine.h"
#include "render_server.h"
#include "vulkan_helpers.h"

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#include <vulkan/vulkan.hpp>

using namespace HopEngine;

TransientCommandBuffer::TransientCommandBuffer()
{
    // allocate a vulkan command buffer
    VkCommandBufferAllocateInfo allocate_info{};
    allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandPool        = static_cast<VkCommandPool>(RenderServer::getCommandPool());
    allocate_info.commandBufferCount = 1;

    CHECK_RESULT(vkAllocateCommandBuffers,
        (static_cast<VkDevice>(RenderServer::getDevice()), &allocate_info,
            reinterpret_cast<VkCommandBuffer*>(&buffer)),
        FAULT,
        ;);

    // automatically start the command buffer so the user can just start issuing co
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    CHECK_RESULT(vkBeginCommandBuffer, (static_cast<VkCommandBuffer>(buffer), &begin_info), FAULT, ;);
}

TransientCommandBuffer::~TransientCommandBuffer()
{
    if (!already_submitted)
        DBG_WARNING("command buffer " + PTR(this) + " being destructed without being submitted");
    DBG_VERBOSE("destroying command buffer " + PTR(this));
    vkFreeCommandBuffers(static_cast<VkDevice>(RenderServer::getDevice()),
        static_cast<VkCommandPool>(RenderServer::getCommandPool()), 1,
        reinterpret_cast<VkCommandBuffer*>(&buffer));
}

void TransientCommandBuffer::submit()
{
    // don't submit if the command buffer has already been submitted
    if (already_submitted)
    {
        DBG_WARNING(
            "attempt to submit command buffer " + PTR(this) + ", but it has already been submitted");
        return;
    }
    already_submitted = true;

    DBG_VERBOSE("submitting transient command buffer " + PTR(this));
    CHECK_RESULT(vkEndCommandBuffer, (static_cast<VkCommandBuffer>(buffer)), ERROR, return;);

    VkSubmitInfo submit_info{};
    submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers    = reinterpret_cast<VkCommandBuffer*>(&buffer);

    // submit and wait for it to be executed
    CHECK_RESULT(vkQueueSubmit,
        (static_cast<VkQueue>(RenderServer::getGraphicsQueue()), 1, &submit_info, VK_NULL_HANDLE), ERROR,
        return;);
    CHECK_RESULT(vkQueueWaitIdle, (static_cast<VkQueue>(RenderServer::getGraphicsQueue())), FAULT, ;);
}

DrawCommandBuffer::DrawCommandBuffer()
{
    VkCommandBufferAllocateInfo buffer_allocate_info{};
    buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    buffer_allocate_info.commandPool        = static_cast<VkCommandPool>(RenderServer::getCommandPool());
    buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    buffer_allocate_info.commandBufferCount = 1;

    CHECK_RESULT(vkAllocateCommandBuffers,
        (static_cast<VkDevice>(RenderServer::getDevice()), &buffer_allocate_info,
            reinterpret_cast<VkCommandBuffer*>(&buffer)),
        FAULT,
        ;);

    VkQueryPoolCreateInfo query_pool_create_info{};
    query_pool_create_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_create_info.queryCount = 512;
    query_pool_create_info.queryType  = VK_QUERY_TYPE_TIMESTAMP;
    CHECK_RESULT(vkCreateQueryPool,
        (static_cast<VkDevice>(RenderServer::getDevice()), &query_pool_create_info, nullptr,
            reinterpret_cast<VkQueryPool*>(&query_pool)),
        FAULT,
        ;);
}

DrawCommandBuffer::~DrawCommandBuffer()
{
    DBG_VERBOSE("destroying command buffer " + PTR(this));
    auto temp_buffer = buffer;
    RenderServer::queueFree(
        [temp_buffer]()
        {
            vkFreeCommandBuffers(static_cast<VkDevice>(static_cast<VkDevice>(RenderServer::getDevice())),
                static_cast<VkCommandPool>(RenderServer::getCommandPool()), 1,
                reinterpret_cast<const VkCommandBuffer*>(&temp_buffer));
        });
    buffer = VK_NULL_HANDLE;
    QUEUE_FREE(query_pool, VkQueryPool, vkDestroyQueryPool);
}

void DrawCommandBuffer::begin(uint32_t index, FrameStats* frame_stats)
{
    if (begun)
    {
        DBG_ERROR("attempt to begin a command buffer which has already been started!");
        return;
    }
    stats       = frame_stats;
    image_index = index;
    CHECK_RESULT(vkResetCommandBuffer, (static_cast<VkCommandBuffer>(buffer), 0), FAULT, ;);
    submitted                  = false;
    current_render_pass        = nullptr;
    current_descriptor_sets[0] = nullptr;
    current_descriptor_sets[1] = nullptr;
    current_descriptor_sets[2] = nullptr;
    current_pipeline_layout    = nullptr;
    current_pipeline           = nullptr;
    current_vertex_buffer      = nullptr;
    current_index_buffer       = nullptr;

    VkCommandBufferBeginInfo cmd_buffer_begin_info{};
    cmd_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CHECK_RESULT(
        vkBeginCommandBuffer, (static_cast<VkCommandBuffer>(buffer), &cmd_buffer_begin_info), FAULT, ;);

    query_offset = 0;
    vkCmdResetQueryPool(static_cast<VkCommandBuffer>(buffer), static_cast<VkQueryPool>(query_pool), 0, 512);
    writeTimestamp(false);
    begun = true;
}

void DrawCommandBuffer::startRenderPassInternal(GPUHandle render_pass, GPUHandle framebuffer,
    glm::u32vec2 extent, const Framebuffer::Clear& clear_values)
{
    if (!begun)
    {
        DBG_ERROR("attempt to start a render pass in a command buffer which has not been started!");
        return;
    }

    if (current_render_pass)
    {
        writeTimestamp(true);
        vkCmdEndRenderPass(static_cast<VkCommandBuffer>(buffer));
    }

    current_descriptor_sets[0] = nullptr;
    current_descriptor_sets[1] = nullptr;
    current_descriptor_sets[2] = nullptr;
    current_pipeline_layout    = RenderServer::getDefaultPipelineLayout();
    current_pipeline           = nullptr;
    current_vertex_buffer      = nullptr;
    current_index_buffer       = nullptr;

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass        = static_cast<VkRenderPass>(render_pass);
    render_pass_begin_info.framebuffer       = static_cast<VkFramebuffer>(framebuffer);
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = { extent.x, extent.y };
    std::vector<VkClearValue> actual_clears;
    VkClearColorValue main_colour_clear;
    main_colour_clear.float32[0] = clear_values.transparent ? 0.0f : clear_values.colour.r;
    main_colour_clear.float32[1] = clear_values.transparent ? 0.0f : clear_values.colour.g;
    main_colour_clear.float32[2] = clear_values.transparent ? 0.0f : clear_values.colour.b;
    main_colour_clear.float32[3] = clear_values.transparent ? 0.0f : 1.0f;
    VkClearValue temp;
    temp.color = main_colour_clear;
    actual_clears.push_back(temp);
    for (glm::vec4 value : clear_values.additionals)
    {
        VkClearColorValue additional_clear;
        additional_clear.float32[0] = value.r;
        additional_clear.float32[1] = value.g;
        additional_clear.float32[2] = value.b;
        additional_clear.float32[3] = value.a;
        temp                        = VkClearValue{};
        temp.color                  = additional_clear;
        actual_clears.push_back(temp);
    }
    if (clear_values.depth_present)
    {
        VkClearDepthStencilValue depth_clear;
        depth_clear.depth   = clear_values.depth;
        depth_clear.stencil = 0;
        temp                = VkClearValue{};
        temp.depthStencil   = depth_clear;
        actual_clears.push_back(temp);
    }
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(actual_clears.size());
    render_pass_begin_info.pClearValues    = actual_clears.data();

    vkCmdBeginRenderPass(static_cast<VkCommandBuffer>(buffer), &render_pass_begin_info,
        VK_SUBPASS_CONTENTS_INLINE);
    writeTimestamp(false);
    stats->passes++;
    current_render_pass      = render_pass;
    current_framebuffer_size = extent;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = { extent.x, extent.y };
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(scissor.extent.width);
    viewport.height   = static_cast<float>(scissor.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(static_cast<VkCommandBuffer>(buffer), 0, 1, &viewport);
    vkCmdSetScissor(static_cast<VkCommandBuffer>(buffer), 0, 1, &scissor);
}

void DrawCommandBuffer::bindPipelineInternal(GPUHandle pipeline)
{
    if (!begun)
    {
        DBG_ERROR("attempt to bind a pipeline in a command buffer which has not been started!");
        return;
    }
    if (!current_render_pass)
    {
        DBG_ERROR("attempt to bind a pipeline in a command buffer where no render pass is in progress!");
        return;
    }

    if (pipeline == current_pipeline) return;

    vkCmdBindPipeline(static_cast<VkCommandBuffer>(buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
        static_cast<VkPipeline>(pipeline));
    stats->pipeline_rebinds++;
    current_pipeline = pipeline;
}

void DrawCommandBuffer::bindPipelineLayoutInternal(GPUHandle pipeline_layout)
{
    if (!begun)
    {
        DBG_ERROR("attempt to bind a pipeline layout in a command buffer which has not been started!");
        return;
    }
    if (!current_render_pass)
    {
        DBG_ERROR(
            "attempt to bind a pipeline layout in a command buffer where no render pass is in progress!");
        return;
    }

    current_pipeline_layout = pipeline_layout;
}

void DrawCommandBuffer::bindDescriptorSetInternal(size_t set, GPUHandle descriptor_set)
{
    if (!begun)
    {
        DBG_ERROR("attempt to bind a descriptor set in a command buffer which has not been started!");
        return;
    }
    if (!current_render_pass)
    {
        DBG_ERROR(
            "attempt to bind a descriptor set in a command buffer where no render pass is in progress!");
        return;
    }
    if (!current_pipeline_layout)
    {
        DBG_ERROR(
            "attempt to bind a descriptor set in a command buffer where no pipeline layout is bound!");
        return;
    }
    if (set > 2)
    {
        DBG_ERROR(
            "attempt to bind a descriptor set for set " + std::to_string(set) + ", which is not allowed");
        return;
    }

    if (current_descriptor_sets[set] == descriptor_set) return;

    current_descriptor_sets[set] = descriptor_set;
    VkDescriptorSet binding_set  = static_cast<VkDescriptorSet>(descriptor_set);
    vkCmdBindDescriptorSets(static_cast<VkCommandBuffer>(buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
        static_cast<VkPipelineLayout>(current_pipeline_layout), static_cast<uint32_t>(set), 1, &binding_set,
        0, nullptr);
}

void DrawCommandBuffer::bindVertexBuffer(GPUHandle vertex_buffer)
{
    if (!begun)
    {
        DBG_ERROR("attempt to bind a vertex buffer in a command buffer which has not been started!");
        return;
    }
    if (!current_render_pass)
    {
        DBG_ERROR(
            "attempt to bind a vertex buffer in a command buffer where no render pass is in progress!");
        return;
    }
    if (!current_pipeline)
    {
        DBG_ERROR("attempt to bind a vertex buffer in a command buffer where no pipeline is bound!");
        return;
    }

    if (current_vertex_buffer == vertex_buffer) return;

    const VkBuffer vertex_buffers[]  = { static_cast<VkBuffer>(vertex_buffer) };
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(static_cast<VkCommandBuffer>(buffer), 0, 1, vertex_buffers, offsets);
    current_vertex_buffer = vertex_buffer;
}

void DrawCommandBuffer::bindIndexBuffer(GPUHandle index_buffer)
{
    if (!begun)
    {
        DBG_ERROR("attempt to bind an index buffer in a command buffer which has not been started!");
        return;
    }
    if (!current_render_pass)
    {
        DBG_ERROR(
            "attempt to bind an index buffer in a command buffer where no render pass is in progress!");
        return;
    }
    if (!current_pipeline)
    {
        DBG_ERROR("attempt to bind an index buffer in a command buffer where no pipeline is bound!");
        return;
    }

    if (current_index_buffer == index_buffer) return;

    vkCmdBindIndexBuffer(static_cast<VkCommandBuffer>(buffer), static_cast<VkBuffer>(index_buffer), 0,
        VK_INDEX_TYPE_UINT16);
    current_index_buffer = index_buffer;
}

void DrawCommandBuffer::setScissorViewport(glm::vec2 offset, glm::vec2 size) const
{
    glm::vec2 extent = current_framebuffer_size;
    VkRect2D scissor{};
    scissor.offset = { static_cast<int32_t>(offset.x * extent.x),
        static_cast<int32_t>(offset.y * extent.y) };
    scissor.extent = { static_cast<uint32_t>(size.x * extent.x), static_cast<uint32_t>(size.y * extent.y) };
    VkViewport viewport{};
    viewport.x        = offset.x * extent.x;
    viewport.y        = offset.y * extent.y;
    viewport.width    = size.x * extent.x;
    viewport.height   = size.y * extent.y;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(static_cast<VkCommandBuffer>(buffer), 0, 1, &viewport);
    vkCmdSetScissor(static_cast<VkCommandBuffer>(buffer), 0, 1, &scissor);
}

void DrawCommandBuffer::drawMeshInternal(size_t indices) const
{
    if (!begun)
    {
        DBG_ERROR("attempt to issue a draw command in a command buffer which has not been started!");
        return;
    }
    if (!current_render_pass)
    {
        DBG_ERROR(
            "attempt to issue a draw command in a command buffer where no render pass is in progress!");
        return;
    }
    if (!current_pipeline)
    {
        DBG_ERROR("attempt to issue a draw command in a command buffer where no pipeline is bound!");
        return;
    }
    if (!current_index_buffer || !current_vertex_buffer)
    {
        DBG_ERROR(
            "attempt to issue a draw command in a command buffer where the vertex/index buffers are not bound!");
        return;
    }

    vkCmdDrawIndexed(static_cast<VkCommandBuffer>(buffer), static_cast<uint32_t>(indices), 1, 0, 0, 0);
    stats->draw_calls++;
    stats->triangles += indices / 3;
}

void DrawCommandBuffer::drawImGui() const
{
    if (!begun)
    {
        DBG_ERROR("attempt to draw ImGui in a command buffer which has not been started!");
        return;
    }
    if (!current_render_pass)
    {
        DBG_ERROR("attempt to draw ImGui in a command buffer where no render pass is in progress!");
        return;
    }

    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data) ImGui_ImplVulkan_RenderDrawData(draw_data, static_cast<VkCommandBuffer>(buffer));
}

void DrawCommandBuffer::extractTiming() const
{
    std::vector<uint32_t> results_buf;
    results_buf.resize(query_offset, 0);
    CHECK_RESULT(vkGetQueryPoolResults,
        (static_cast<VkDevice>(RenderServer::getDevice()), static_cast<VkQueryPool>(query_pool), 0,
            query_offset, results_buf.size() * sizeof(uint32_t), results_buf.data(), 4,
            VK_QUERY_RESULT_WAIT_BIT),
        ERROR,
        ;);

    const float render_time = static_cast<float>(results_buf[results_buf.size() - 1] - results_buf[0]) /
                              (1000.0f * 1000.0f * 100.0f);
    stats->render_time      = render_time;
    for (size_t offset = 1; offset < results_buf.size() - 1; offset += 2)
    {
        float pass_time = static_cast<float>(results_buf[offset + 1] - results_buf[offset]) /
                          (1000.0f * 1000.0f * 100.0f);
        stats->pass_times.push_back(pass_time);
    }
}

void DrawCommandBuffer::end()
{
    if (!begun)
    {
        DBG_ERROR("attempt to end rendering in a command buffer which has not been started!");
        return;
    }
    if (current_render_pass)
    {
        writeTimestamp(true);
        vkCmdEndRenderPass(static_cast<VkCommandBuffer>(buffer));
    }
    writeTimestamp(true);
    CHECK_RESULT(vkEndCommandBuffer, (static_cast<VkCommandBuffer>(buffer)), FAULT, ;);
    submitted = true;
    begun     = false;
}

void DrawCommandBuffer::writeTimestamp(bool bottom_of_pipe)
{
    vkCmdWriteTimestamp(static_cast<VkCommandBuffer>(buffer),
        bottom_of_pipe ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        static_cast<VkQueryPool>(query_pool), query_offset++);
}
