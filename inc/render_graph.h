#pragma once

#include "common.h"
#include "material.h"
#include "swapchain.h"
#include "texture.h"

#include <glm/vec2.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace HopEngine
{

/**
 * @brief provides support for complex, multi-step image processing workflows with a serialisable interface.
 * allows rendering multiple cameras within a scene, in addition to mulitple shader-based post-processing
 * steps which can be chained together.
 *
 * - simplifies the process of executing multiple render passes
 *
 * - manages resolution of framebuffers, including fractional resolutions and custom extents
 *
 * - handles binding output attachments to post-process materials as input textures, including sampler
 * configuration
 */
class RenderGraph final : public Destructible
{
public:
    /**
     * @brief outlines the binding to an output attachment of an existing render pass, including information
     * for how it should be sampled in the material to which it is bound.
     */
    struct AttachmentBinding final
    {
        size_t step_index             = 0; // target step (render pass) to bind to
        size_t output_index           = 0; // attachment texture index within the target step
        Sampler::Filter filter_mode   = Sampler::FILTER_LINEAR;      // texture filtering mode
        Sampler::Address address_mode = Sampler::ADDRESS_CLAMP_EDGE; // texture addressing mode

        AttachmentBinding() = default;
        AttachmentBinding(const size_t step, const size_t output) : step_index(step), output_index(output)
        {
        }
        AttachmentBinding& filter(const Sampler::Filter value)
        {
            filter_mode = value;
            return *this;
        }
        AttachmentBinding& address(const Sampler::Address value)
        {
            address_mode = value;
            return *this;
        }
    };

    /**
     * @brief describes a step in the render chain. may contribute either as a camera step (rendering
     * objects in the scene) or a post-process step (rendering a full-screen quad, with attachment
     * bindings). should not be initialised manually, instead use the `RenderGraph::Builder` and its
     * `add...` functions.
     */
    struct Step final
    {
        bool is_camera = true; // whether the step is a camera or post-process step
        // if `is_camera`, reflects the camera index in the scene to which this step should bind
        size_t camera_slot = 0;
        // if `!is_camera`, holds the material used for post-processing
        Ref<Material> material;
        // if `!is_camera`, holds a mapping of material texture binding indices to output attachments from
        // previous `Step`s
        std::map<uint32_t, AttachmentBinding> texture_bindings;

        // scales the resolution of the overall render graph, and thus the extent of the output textures,
        // allowing for smaller (e.g. half-resolution) steps. if this value is less than or equal to zero,
        // `custom_extent` is used instead
        float resolution_scale = 1.0f;
        // provides custom values for the resolution of the step's outputs. if either component is zero, the
        // extent of the overall render graph is used for that component instead
        glm::u32vec2 custom_extent{ 0, 0 };
        // render pass (including framebuffers) into which this step will be rendered
        Ref<RenderPass> render_pass;
        // if `!is_camera`, holds the scene (set 0) uniforms for rendering
        Ref<UniformBlock> scene_uniforms;

        std::string name; // text identifier for the step, making retrieval easier
        // if `true`, this step will not be executed, and attachment bindings targeting it will be
        // redirected to the previous step
        bool skipped = false;

        ~Step();
    };

    /**
     * @brief builder class for the render graph which handles constructing and chaining together render
     * steps.
     */
    struct Builder final
    {
        // list of render steps in the order they will be rendered
        std::vector<Step> execution_steps;
        // filtering mode for when the final output attachment is being drawn to the swapchain, useful for
        // pixel-art post processing
        Sampler::Filter screen_filtering = Sampler::FILTER_LINEAR;

        /**
         * @brief builer function which adds a camera render step. default render pass config (3 extra +
         * depth) is used, along with default scaling.
         * @param slot which camera slot in the scene should be used to render this camera step.
         * @returns self-reference for chaining calls.
         */
        Builder& addCamera(size_t slot);
        Builder& addCamera(size_t slot, const RenderPass::Config& render_pass_config,
            float size_factor = 1.0f, glm::u32vec2 custom_extent = { 128, 128 });
        Builder& addCamera(size_t slot, float size_factor, glm::u32vec2 custom_extent = { 128, 128 });
        Builder& addPostProcess(const Ref<Shader>& shader,
            const std::map<uint32_t, AttachmentBinding>& texture_bindings);
        Builder& addPostProcess(const Ref<Shader>& shader,
            const std::map<uint32_t, AttachmentBinding>& texture_bindings,
            const RenderPass::Config& render_pass_config, float size_factor = 1.0f,
            glm::u32vec2 custom_extent = { 128, 128 });
        Builder& addPostProcess(const Ref<Shader>& shader,
            const std::map<uint32_t, AttachmentBinding>& texture_bindings, float size_factor,
            glm::u32vec2 custom_extent = { 128, 128 });
        Builder& filtering(const Sampler::Filter value)
        {
            screen_filtering = value;
            return *this;
        }
    };

public:
    int output_step  = -1;
    int output_image = 0;

private:
    std::string origin;
    std::vector<Step> execution_steps;
    glm::u32vec2 expected_extent = { 0, 0 };
    Ref<Material> passthrough;
    WeakRef<Texture> passthrough_texture;

public:
    DELETE_CONSTRUCTORS(RenderGraph);
    RenderGraph(const Builder& config);
    ~RenderGraph() override;

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }
    WeakRef<Material> getMaterialForStep(size_t step);
    WeakRef<Material> getMaterialForStep(const std::string& name);
    WeakRef<Texture> getFinalImage() const;
    bool getSkipStep(size_t step) const;
    bool getSkipStep(const std::string& name) const;
    void setSkipStep(size_t step, bool skip);
    void setSkipStep(const std::string& name, bool skip);

    void resizeBuffers(glm::u32vec2 new_extent);
    void draw(WeakRef<DrawCommandBuffer> command_buffer, const std::vector<DrawCommand>& draw_commands,
        const std::map<size_t, std::pair<WeakRef<UniformBlock>, glm::vec4>>& cameras);
    void bindOutputMaterial(WeakRef<DrawCommandBuffer> command_buffer);

    std::map<size_t, glm::u32vec2> getCameraSlots();

    void drawImGuiDebug();
    static Ref<RenderGraph> deserialise(const std::string& name);

private:
    size_t findStep(const std::string& name) const;
    void rebuildBindings();
    static void recordCameraStep(WeakRef<DrawCommandBuffer> command_buffer,
        const WeakRef<UniformBlock>& camera, glm::vec4 clear_colour, const WeakRef<RenderPass>& pass,
        const std::multiset<DrawCommand, DrawCommand>& commands);
    static void recordPostProcessStep(WeakRef<DrawCommandBuffer> command_buffer,
        const WeakRef<Material>& material, const WeakRef<UniformBlock>& scene_descriptor_set);
};

} // namespace HopEngine
