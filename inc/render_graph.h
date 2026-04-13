#pragma once

#include "common.h"
#include "material.h"
#include "swapchain.h"
#include "texture.h"

#include <glm/vec2.hpp>
#include <map>
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
         * depth) is used, along with default resolution.
         * @param slot which camera slot in the scene should be used to render this camera step.
         * @returns self-reference for chaining calls.
         */
        Builder& addCamera(size_t slot);
        /**
         * @brief builer function which adds a camera render step. allows you to specify the configuration
         * of the target render pass and resolution of the framebuffers.
         * @param slot which camera slot in the scene should be used to render this camera step.
         * @param render_pass_config description of the render pass configuration.
         * @param size_factor scale factor applied to default render graph resolution for output
         * attachments. if set to zero, this value is ignored and `custom_extent` is used instead.
         * @param custom_extent custom resolution to use for output attachments, in pixels. if either of the
         * coordinates is zero, the default resolution for the render graph is used for that coordinate
         * instead.
         * @returns self-reference for chaining calls.
         */
        Builder& addCamera(size_t slot, const RenderPass::Config& render_pass_config,
            float size_factor = 1.0f, glm::u32vec2 custom_extent = { 128, 128 });
        /**
         * @brief builer function which adds a camera render step. allows you to specify the resolution of
         * the framebuffers.
         * @param slot which camera slot in the scene should be used to render this camera step.
         * @param size_factor scale factor applied to default render graph resolution for output
         * attachments. if set to zero, this value is ignored and `custom_extent` is used instead.
         * @param custom_extent custom resolution to use for output attachments, in pixels. if either of the
         * coordinates is zero, the default resolution for the render graph is used for that coordinate
         * instead.
         * @returns self-reference for chaining calls.
         */
        Builder& addCamera(size_t slot, float size_factor, glm::u32vec2 custom_extent = { 128, 128 });
        /**
         * @brief builer function which adds a post-process render step. default render pass config (3 extra
         * + depth) is used, along with default resolution.
         * @param shader shader to use for creating a material to be applied to the fullscreen quad.
         * @param texture_bindings list of bindings between material textures and output attachments from
         * other render steps.
         * @returns self-reference for chaining calls.
         */
        Builder& addPostProcess(const Ref<Shader>& shader,
            const std::map<uint32_t, AttachmentBinding>& texture_bindings);
        /**
         * @brief builer function which adds a post-process render step. allows you to specify the
         * render pass configuration and the resolution of the framebuffers.
         * @param shader shader to use for creating a material to be applied to the fullscreen quad.
         * @param texture_bindings list of bindings between material textures and output attachments from
         * other render steps.
         * @param render_pass_config description of the render pass configuration.
         * @param size_factor scale factor applied to default render graph resolution for output
         * attachments. if set to zero, this value is ignored and `custom_extent` is used instead.
         * @param custom_extent custom resolution to use for output attachments, in pixels. if either of the
         * coordinates is zero, the default resolution for the render graph is used for that coordinate
         * instead.
         * @returns self-reference for chaining calls.
         */
        Builder& addPostProcess(const Ref<Shader>& shader,
            const std::map<uint32_t, AttachmentBinding>& texture_bindings,
            const RenderPass::Config& render_pass_config, float size_factor = 1.0f,
            glm::u32vec2 custom_extent = { 128, 128 });
        /**
         * @brief builer function which adds a post-process render step. allows you to specify the
         * resolution of the framebuffers.
         * @param shader shader to use for creating a material to be applied to the fullscreen quad.
         * @param texture_bindings list of bindings between material textures and output attachments from
         * other render steps.
         * @param size_factor scale factor applied to default render graph resolution for output
         * attachments. if set to zero, this value is ignored and `custom_extent` is used instead.
         * @param custom_extent custom resolution to use for output attachments, in pixels. if either of the
         * coordinates is zero, the default resolution for the render graph is used for that coordinate
         * instead.
         * @returns self-reference for chaining calls.
         */
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
    int output_step  = -1; // index of the step from which to pick the final image drawn to the screen
    int output_image = 0;  // attachment index representing the final image drawn to the screen

private:
    std::string origin; // if not empty, contains the path from which this render graph was deserialised
    std::vector<Step> execution_steps;       // list of render steps in the order they will be rendered
    glm::u32vec2 expected_extent = { 0, 0 }; // default extent of the render graph images
    Ref<Material> passthrough; // material for rendering the final output attachment to the screen
    // final output texture which will be exposed outside the render graph, and theoretically drawn to the
    // screen
    WeakRef<Texture> passthrough_texture;

public:
    DELETE_CONSTRUCTORS(RenderGraph);
    /**
     * @brief constructs a new render graph based on a specified configuration. use the `add...` functions
     * on `Builder` to configure the render graph.
     * @param config render graph configuration.
     */
    RenderGraph(const Builder& config);
    ~RenderGraph() override;

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }
    /**
     * @brief retrieves the material being used for a particular post-processing render step.
     * @param step index of the step from which to retrieve the material. must be the index of a
     * post-processing step.
     * @returns material used by the render step, or `nullptr` if `step` does not exist or is not a
     * post-processing step.
     */
    WeakRef<Material> getMaterialForStep(size_t step);
    /**
     * @brief retrieves the material being used for a particular post-processing render step.
     * @param name name of the step from which to retrieve the material. must be the name of a
     * post-processing step.
     * @returns material used by the render step, or `nullptr` if `name` does not match any render step or
     * is not a post-processing step.
     */
    WeakRef<Material> getMaterialForStep(const std::string& name);
    /**
     * @brief retrieves the final output attachment from the render graph, defined by the `output_step` and
     * `output_image` fields. if `output_step` is set to -1, the last step in the graph is used. if
     * `output_step` is greater than or equal to the number of render steps, a modulus is applied.
     * @returns reference to the texture backing the selected render pass attachment, or `nullptr` if there
     * were no render steps in the render graph or the `output_image` value was out of bounds for the
     * attachments to the given render step.
     */
    WeakRef<Texture> getFinalImage() const;
    /**
     * @brief checks if a particular step is set to be skipped during rendering.
     * @param step index of the step to query.
     * @returns `true` if the step is set to be skipped, or `false` if not. also returns `false` if the
     * `step` was out of bounds for the list of render steps.
     */
    bool getSkipStep(size_t step) const;
    /**
     * @brief checks if a particular step is set to be skipped during rendering.
     * @param name name of the step to query.
     * @returns `true` if the step is set to be skipped, or `false` if not. also returns `false` if the
     * `name` did not match anything in the list of render steps.
     */
    bool getSkipStep(const std::string& name) const;
    /**
     * @brief sets whether a particular step will be skipped during rendering. does nothing if the
     * `step` is out of bounds for the list of render steps.
     * @param step index of the step to modify.
     * @param skip `true` if the step should be skipped during rendering, or `false` if it should be
     * executed as normal.
     */
    void setSkipStep(size_t step, bool skip);
    /**
     * @brief sets whether a particular step will be skipped during rendering. does nothing if the
     * `name` does not match anything in the list of render steps.
     * @param name name of the step to modify.
     * @param skip `true` if the step should be skipped during rendering, or `false` if it should be
     * executed as normal.
     */
    void setSkipStep(const std::string& name, bool skip);

    /**
     * @brief updates the extents of the framebuffers of all steps based on a specified overall render graph
     * extent. this is the extent which defines the default resolution, which may be overriden by individual
     * render steps. if `new_extent` is the same as the current overall extent, this function returns
     * immediately.
     * @param new_extent new expected default extent for the render graph.
     */
    void resizeBuffers(glm::u32vec2 new_extent);

    /**
     * @brief executes the entire render graph's rendering sequence, including a series of scene draw
     * commands, into a command buffer. each draw command will be executed in any relevant camera render
     * step (according to the `camera_mask` field in the `DrawCommand` struct). if the material of the draw
     * command is incompatible with the render pass configuration in any given render step, the draw command
     * is skipped for that step. render steps are executed in the order in which they are defined, while
     * draw commands are ordered according to their priority followed by their specific characteristics (see
     * `DrawCommand::operator()`).
     * @param command_buffer draw command buffer into which commands will be issued.
     * @param draw_commands series of draw commands which will be executed in camera steps.
     * @param cameras mapping from camera slot indices to camera descriptions in the form of uniform buffers
     * and clear colours for each camera. the caller should provide a camera description for each expected
     * camera slot index in the render graph; if a camera step is encountered without a matching entry in
     * this map, that step will be skipped during rendering.
     */
    void draw(WeakRef<DrawCommandBuffer> command_buffer, const std::vector<DrawCommand>& draw_commands,
        const std::map<size_t, std::pair<WeakRef<UniformBlock>, glm::vec4>>& cameras);
    /**
     * @brief binds the output passthrough material which draws the output image (can be retrieved with
     * `getFinalImage`). allows the result of the render graph to then be drawn to the screen (or another
     * framebuffer).
     * @param command_buffer draw command buffer into which the command will be issued.
     */
    void bindOutputMaterial(WeakRef<DrawCommandBuffer> command_buffer);

    /**
     * @brief queries the desired camera slots and their intended extents. used to calculate camera uniforms
     * (in particular, the view-to-clip matrices).
     * @returns mapping from camera slot index to the intended resolution of the camera's framebuffer.
     */
    std::map<size_t, glm::u32vec2> getCameraSlots();

    void drawImGuiDebug();
    /**
     * @brief constructs a render graph from a text-based serialised representation.
     * @param name path to the target file from which to read the text representation.
     * @returns render graph constructed based on serialised representation, or `nullptr` if an error
     * occurred during deserialisation.
     */
    static Ref<RenderGraph> deserialiseFile(const std::string& name);
    /**
     * @brief constructs a render graph from a text-based serialised representation.
     * @param token_str text representation to decode.
     * @param origin original path from which the render graph was loaded, may be empty.
     * @returns render graph constructed based on serialised representation, or `nullptr` if an error
     * occurred during deserialisation.
     */
    static Ref<RenderGraph> deserialise(const std::string& token_str, const std::string& origin = "");

private:
    /**
     * @brief retrieves a render step by name.
     * @param name name of the step to search for.
     * @returns index of the matching step, or 0 if no matching step was found.
     */
    size_t findStep(const std::string& name) const;
    /**
     * @brief binds material texture slots to the output attachments described by their
     * `AttachmentBinding`s; necessary when the render graph is modified.
     */
    void rebuildBindings();
    /**
     * @brief records draw commands for a camera render step.
     * @param command_buffer draw command buffer into which commands will be issued.
     * @param camera uniform block to be used in the scene (set 0) slot.
     * @param clear_colour clear colour for the main colour attachment.
     * @param pass render pass to execute commands within.
     * @param commands sorted series of draw commands to be executed.
     */
    static void recordCameraStep(WeakRef<DrawCommandBuffer> command_buffer,
        const WeakRef<UniformBlock>& camera, glm::vec4 clear_colour, const WeakRef<RenderPass>& pass,
        const std::vector<DrawCommand>& commands);
    /**
     * @brief records draw commands for a post-process render step.
     * @param command_buffer draw command buffer into which commands will be issued.
     * @param material material which will be rendered on a full-screen quad, into its render pass.
     * @param scene_descriptor_set uniform block to be used in the scene (set 0) slot.
     */
    static void recordPostProcessStep(WeakRef<DrawCommandBuffer> command_buffer,
        const WeakRef<Material>& material, const WeakRef<UniformBlock>& scene_descriptor_set);
};

} // namespace HopEngine
