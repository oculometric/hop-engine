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
#include "material.h"
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
    struct TextureInput final
    {
        friend class RenderGraph;

    private:
        size_t step_index             = 0; // target step (render pass) to bind to
        size_t output_index           = 0; // attachment texture index within the target step
        Sampler::Filter filter_mode   = Sampler::FILTER_LINEAR;      // texture filtering mode
        Sampler::Address address_mode = Sampler::ADDRESS_CLAMP_EDGE; // texture addressing mode

    public:
        TextureInput() = default;
        TextureInput(size_t step, size_t output) : step_index(step), output_index(output) {}
        TextureInput& filter(Sampler::Filter value)
        {
            filter_mode = value;
            return *this;
        }
        TextureInput& address(Sampler::Address value)
        {
            address_mode = value;
            return *this;
        }
    };

    /**
     * @brief builder class for the render graph which handles constructing and chaining together render
     * steps.
     */
    struct Builder final : public Destructible
    {
        friend class RenderGraph;

    private:
        /**
         * @brief builder class for constructing render graph steps.
         */
        struct StepDescription final : public Destructible
        {
            ~StepDescription() override = default;

            std::string name;
            bool is_camera                         = true;
            float resolution_scale                 = 1.0f;
            glm::u32vec2 custom_extent             = { 128, 128 };
            Framebuffer::Config framebuffer_config = Framebuffer::getDefaultConfig();
            size_t camera_slot                     = 0;
            Ref<Shader> shader;
            std::map<std::string, TextureInput> texture_bindings;
        };

    private:
        // list of render steps in the order they will be rendered
        std::vector<StepDescription> execution_steps;
        // filtering mode for when the final output attachment is being drawn to the swapchain, useful for
        // pixel-art post processing
        Sampler::Filter screen_filtering = Sampler::FILTER_LINEAR;

    public:
        ~Builder() override = default;

        /**
         * @brief builder function which adds a camera render step. subsequent calls to
         * `configureFramebuffer` and `setResolution` can be used to further configure this render step.
         * @param name text identifier for the render step to be queried later.
         * @param slot which camera slot in the scene should be used to render this camera step.
         * @returns self-reference for chaining calls.
         */
        Builder& addCameraStep(const std::string& name, size_t slot);
        /**
         * @brief builder function which adds a post-process render step. subsequent calls to
         * `configureFramebuffer`, `setResolution`, and `bindTexture` can be used to further configure this
         * render step.
         * @param name text identifier for the render step to be queried later.
         * @param shader shader to generate a material for rendering.
         * @returns self-reference for chaining calls.
         */
        Builder& addPostprocessStep(const std::string& name, Ref<Shader> shader);
        /**
         * @brief builder function which updates the framebuffer attachment configuration of the most
         * recently added render step. only valid after a call to `addCameraStep` or `addPostprocessStep`.
         * @param config configuration parameters to be used for creating the framebuffer (and the material,
         * if this is a post-processing step) for the render step.
         * @returns self-reference for chaining calls.
         */
        Builder& configureFramebuffer(const Framebuffer::Config& config);
        /**
         * @brief builder function which updates the framebuffer resolution of the most recently added
         * render step. only valid after a call to `addCameraStep` or `addPostprocessStep`.
         * @param scale scale factor used to compute the framebuffer resolution, by multiplying with default
         * render graph resolution. may not be less than or equal to zero.
         * @returns self-reference for chaining calls.
         */
        Builder& setResolution(float scale);
        /**
         * @brief builder function which updates the framebuffer resolution of the most recently added
         * render step. only valid after a call to `addCameraStep` or `addPostprocessStep`.
         * @param extent custom resolution to use for the framebuffer, in pixels. if either of the
         * coordinates is zero, the default resolution for the render graph is used for that coordinate
         * instead.
         * @returns self-reference for chaining calls.
         */
        Builder& setResolution(glm::u32vec2 extent);
        /**
         * @brief builder function which adds a texture binding to the most recently added render step. used
         * to link the outputs of previous render steps as inputs of this step. only valid after a call to
         * `addPostprocessStep`.
         * @param texture_uniform name of the texture uniform variable to which the texture will be bound.
         * @param binding struct describing which render step and output attachment should be used as the
         * target texture. also allows specifying texture sampling parameters.
         * @returns self-reference for chaining calls.
         */
        Builder& bindTexture(const std::string& texture_uniform, const TextureInput& binding);
        /**
         * @brief builder function which configures the filtering of the final render graph output when
         * drawn to the screen.
         * @param value filtering mode used when outputting the render graph's final image.
         * @returns self-reference for chaining calls.
         */
        Builder& filtering(Sampler::Filter value);
    };

private:
    /**
     * @brief describes a step in the render chain. may contribute either as a camera step (rendering
     * objects in the scene) or a post-process step (rendering a full-screen quad, with attachment
     * bindings). should not be initialised manually, instead use the `RenderGraph::Builder` and its
     * `add...` functions.
     */
    struct Step final : public Destructible
    {
        ~Step() override = default;

        bool is_camera = true; // whether the step is a camera or post-process step
        // if `is_camera`, reflects the camera index in the scene to which this step should bind
        size_t camera_slot = 0;
        // if `!is_camera`, holds the material used for post-processing
        Ref<Material> material;
        // if `!is_camera`, holds a mapping of material texture binding names to output attachments from
        // previous `Step`s
        std::map<std::string, TextureInput> texture_bindings;

        // scales the resolution of the overall render graph, and thus the extent of the output textures,
        // allowing for smaller (e.g. half-resolution) steps. if this value is less than or equal to zero,
        // `custom_extent` is used instead
        float resolution_scale = 1.0f;
        // provides custom values for the resolution of the step's outputs. if either component is zero, the
        // extent of the overall render graph is used for that component instead
        glm::u32vec2 custom_extent{ 0, 0 };
        // framebuffer into which this step will be rendered
        Ref<Framebuffer> framebuffer;
        // if `!is_camera`, holds the scene (set 0) uniforms for rendering
        Ref<UniformBlock> scene_uniforms;

        std::string name; // text identifier for the step, making retrieval easier
        // if `true`, this step will not be executed, and attachment bindings targeting it will be
        // redirected to the previous step
        bool skipped = false;
    };

public:
    int output_step  = -1; // index of the step from which to pick the final image drawn to the screen
    int output_image = 0;  // attachment index representing the final image drawn to the screen

private:
    std::string origin; // if not empty, contains the path from which this render graph was deserialised
    std::vector<Step> execution_steps;       // list of render steps in the order they will be rendered
    glm::u32vec2 expected_extent = { 0, 0 }; // default extent of the render graph images
    Ref<Material> passthrough; // material for rendering the final output attachment to the screen
    // final output texture which will be exposed outside the render graph as the final product
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
    WeakRef<Material> getMaterialForStep(size_t step) const;
    /**
     * @brief retrieves the material being used for a particular post-processing render step.
     * @param name name of the step from which to retrieve the material. must be the name of a
     * post-processing step.
     * @returns material used by the render step, or `nullptr` if `name` does not match any render step or
     * is not a post-processing step.
     */
    WeakRef<Material> getMaterialForStep(const std::string& name) const;
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
     * @example doc/RENDER_GRAPH.md
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
     * `TextureInput`s; necessary when the render graph is modified.
     */
    void rebuildBindings();
    /**
     * @brief records draw commands for a camera render step.
     * @param command_buffer draw command buffer into which commands will be issued.
     * @param pass framebuffer to render within.
     * @param camera camera to be used for rendering (providing scene descriptor set).
     * @param clear_colour clear colour for the main colour attachment.
     * @param commands sorted series of draw commands to be executed.
     */
    static void recordCameraStep(WeakRef<DrawCommandBuffer> command_buffer, WeakRef<Framebuffer> pass,
        WeakRef<UniformBlock> camera, glm::vec4 clear_colour, const std::vector<DrawCommand>& commands);
    /**
     * @brief records draw commands for a post-process render step.
     * @param command_buffer draw command buffer into which commands will be issued.
     * @param pass framebuffer to render within.
     * @param scene_descriptor_set uniform block to be used in the scene (set 0) slot.
     * @param material material which will be rendered on a full-screen quad, into its render pass.
     */
    static void recordPostProcessStep(WeakRef<DrawCommandBuffer> command_buffer, WeakRef<Framebuffer> pass,
        WeakRef<UniformBlock> scene_descriptor_set, WeakRef<Material> material);
};

} // namespace HopEngine
