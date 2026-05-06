/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"
#include "framebuffer.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

namespace HopEngine
{

/**
 * @brief represents a transient GPU command buffer to allow immediate command execution, such as
 * performing buffer/image copies and image transitions outside of the rendering pipeline.
 */
class TransientCommandBuffer final : public Destructible
{
private:
    GPUHandle buffer       = nullptr; // GPU command buffer handle
    bool already_submitted = false;   // whether the command buffer has been submitted

public:
    DELETE_NOT_ALL_CONSTRUCTORS(TransientCommandBuffer);
    TransientCommandBuffer();
    ~TransientCommandBuffer() override;

    GPUHandle getHandle() const { return buffer; }
    /**
     * @brief causes the command buffer to be submitted to the graphics queue, and waits
     * for completion before returning.
     */
    void submit();
};

/**
 * @brief abstracts a GPU graphics command buffer, managing current internal state to reduce unecessary
 * rebinds/duplicate commands and simplify some operations.
 */
class DrawCommandBuffer final : public Destructible
{
private:
    GPUHandle buffer      = nullptr; // GPU command buffer handle
    GPUHandle query_pool  = nullptr; // GPU query pool handle, stores stats/timings from the GPU
    uint32_t query_offset = 0;       // index of the next query to be submitted to the pool
    bool begun            = false;   // whether the command buffer has been started yet
    bool submitted        = false;   // whether the command buffer has already been submitted
    uint32_t image_index  = 0;       // image index for which the command buffer exists
    FrameStats* stats     = nullptr; // pointer to stats struct which will be populated by various calls

    // various currently-active states, to eliminate duplication of GPU commands where possible

    GPUHandle current_render_pass         = nullptr;
    glm::u32vec2 current_framebuffer_size = { 0, 0 };
    GPUHandle current_descriptor_sets[3]  = { nullptr };
    GPUHandle current_pipeline_layout     = nullptr;
    GPUHandle current_pipeline            = nullptr;
    GPUHandle current_vertex_buffer       = nullptr;
    GPUHandle current_index_buffer        = nullptr;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(DrawCommandBuffer);
    DrawCommandBuffer();
    ~DrawCommandBuffer() override;

    /**
     * @brief reset and begin the command buffer. returns early if the command buffer is already being
     * recorded. must be called before command recording can begin, and may not be called again until
     * after `end` has been called.
     * @param index image index which the command buffer is being recorded for. determines which
     * descriptor set instance is used by various objects when submitting commands.
     * @param frame_stats pointer to a `FrameStats` struct which will be updated with statistics such as
     * number of meshes and lights rendered, as well as statistics gathered from the GPU.
     */
    void begin(uint32_t index, FrameStats* frame_stats);

    uint32_t getImageIndex() const { return image_index; }
    GPUHandle getCommandBuffer() const { return buffer; }

    /**
     * @brief internal function to issue a command to start a GPU render pass. should be called when a
     * new render pass is started.
     * @param render_pass GPU handle for the render pass (e.g. a `VkRenderPass`).
     * @param framebuffer GPU handle for the framebuffer to be used (e.g. `VkFramebuffer`).
     * @param extent size of the framebuffer in pixels.
     * @param clear_values array of values to be used for clearing the various attachments to the render
     * pass.
     * accordingly.
     */
    void startRenderPassInternal(GPUHandle render_pass, GPUHandle framebuffer, glm::u32vec2 extent,
        const Framebuffer::Clear& clear_values);
    /**
     * @brief internal function to issue a command to bind a pipeline. should be called when the active
     * material and/or shader changes.
     * @param pipeline GPU handle for the pipeline object (e.g. a `VkPipeline`).
     */
    void bindPipelineInternal(GPUHandle pipeline);
    /**
     * @brief internal function to set the currently in-use pipeline layout. should be called when the
     * active shader changes.
     * @param pipeline_layout GPU handle for the pipeline layout (e.g. a `VkPipelineLayout`).
     */
    void bindPipelineLayoutInternal(GPUHandle pipeline_layout);
    /**
     * @brief internal function to issue a command to bind a descriptor set. should be called when the
     * active object, material, or camera/view changes.
     * @param set index of the descriptor set begin bound to in the pipeline layout.
     * @param descriptor_set GPU handle for the actual descriptor set (e.g. a `VkDescriptorSet`).
     */
    void bindDescriptorSetInternal(size_t set, GPUHandle descriptor_set);
    /**
     * @brief internal function to issue a command to bind a vertex buffer. should be called when the
     * active mesh changes.
     * @param vertex_buffer GPU handle for the buffer containing vertex data (e.g. a `VkBuffer`).
     */
    void bindVertexBuffer(GPUHandle vertex_buffer);
    /**
     * @brief internal function to issue a command to bind an index buffer. should be called when the
     * active mesh changes.
     * @param index_buffer GPU handle for the buffer containing index data (e.g. a `VkBuffer`).
     */
    void bindIndexBuffer(GPUHandle index_buffer);
    /**
     * @brief internal function to issue a command to update the scissor and viewport together, allowing
     * only a portion of the screen (specified in 0-1 UV coordinates from top-left) to be drawn to.
     * @param offset offset of the start of the viewport in 0-1 UV coordinates.
     * @param size size of the viewport in 0-1 UV coordinates.
     */
    void setScissorViewport(glm::vec2 offset, glm::vec2 size) const;
    /**
     * @brief internal function to issue a command to draw the currently bound vertex and index buffers.
     * @param indices size of the index buffer to be drawn (should already be bound).
     */
    void drawMeshInternal(size_t indices) const;
    /**
     * @brief issues necessary commands to draw ImGui render info into the currently bound render pass.
     */
    void drawImGui() const;
    /**
     * @brief collects timing information from the GPU query pool, stored into the `FrameStats` struct
     * pointer given when `begin` was called.
     */
    void extractTiming() const;

    /**
     * @brief terminates the command buffer, resets internal state, and prepares for submission.
     */
    void end();

private:
    /**
     * @brief writes a new timestamp for the current pipeline state into the query pool.
     * @param bottom_of_pipe `true` if the timestamp should be recorded with the
     * `VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT` stage flag; otherwise the
     * `VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT` stage flag is used.
     */
    void writeTimestamp(bool bottom_of_pipe);
};

} // namespace HopEngine
