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

#include <glm/glm.hpp>
#include <map>

namespace HopEngine
{

/**
 * @brief encapsulates a set of shader programs which will be used to render a surface by a material.
 * essentially a 'material template', where materials apply specific values to shader uniforms, and
 * determine how the shader will be rendered (i.e. the pipeline characteristics).
 */
class Shader final : public Destructible
{
public:
    /**
     * @brief enumerates supported shader stages.
     */
    enum Stage
    {
        STAGE_VERTEX,
        STAGE_FRAGMENT
    };

    /**
     * @brief enumerates supported descriptor binding types. descriptor bindings can either be a block of
     * uniform variables (individual uniforms are not supported in Vulkan), or a combined texture-sampler.
     */
    enum DescriptorType
    {
        UNIFORM,
        TEXTURE
    };

    /**
     * @brief describes the layout information for a uniform variable within a block, allowing it to be
     * accessed in a uniform buffer by name later. this information is retrieved from shader reflection
     * information.
     */
    struct UniformVariable final
    {
        std::string name;  // name of the variable provided in shader code
        size_t size   = 0; // size of the variable in bytes
        size_t offset = 0; // offset into the uniform buffer in bytes
    };

    /**
     * @brief describes a descriptor binding, which may be either a uniform block or a combined
     * texture-sampler. contains important shader reflection information.
     */
    struct Descriptor final
    {
        uint32_t binding    = 0;       // layout binding index within descriptor set 2
        DescriptorType type = UNIFORM; // binding type, uniform block or texture-sampler
        size_t buffer_size  = 0;       // total buffer size, if this is a uniform block binding
        std::string name;              // name of the descriptor, only used for textures
        // list of contained uniform variables, if this is a uniform block binding
        std::vector<UniformVariable> variables;
        // if this is a texture binding, determines whether a 2D or 3D image view is expected
        bool texture_is_3d = false;
    };

    /**
     * @brief describes the layout of a descriptor set with metadata. most of the time you should only be
     * dealing with set 2 layouts, as set 0 and set 1 are handled internally.
     */
    struct Layout final
    {
        GPUHandle layout = nullptr;       // VkDescriptorSetLayout GPU handle
        std::vector<Descriptor> bindings; // list of bindings within the DSL
        uint32_t set_index = 2;           // index to which the DSL is meant to be bound
    };

private:
    std::string origin; // if not empty, contains the path from which this shader was compiled
    GPUHandle vert_module           = nullptr; // vertex shader GPU module
    GPUHandle frag_module           = nullptr; // fragment shader GPU module
    GPUHandle pipeline_layout       = nullptr; // pipeline layout linking together descriptor set layouts
    GPUHandle descriptor_set_layout = nullptr; // shader-specific descriptor set 2 layout
    std::vector<Descriptor> bindings;          // reflected descriptor bindings for set 2
    uint64_t hash       = 0;                   // hash of the shader modules which are compiled
    bool load_succeeded = false;               // if `false` the shader failed to load/compile

public:
    DELETE_CONSTRUCTORS(Shader);
    /**
     * @brief loads a text shader from the specified path and compiles it.
     * @param base_path path to the shader to load.
     */
    Shader(const std::string& base_path);
    ~Shader() override;

    /**
     * @brief constructs a descriptor set layout based on an array of bindings.
     * @param bindings list of descriptor bindings present in this descriptor set. see `Shader::Descriptor`.
     * @returns GPU handle for the VkDescriptorSetLayout.
     */
    static GPUHandle createDescriptorSetLayout(std::vector<Descriptor> bindings);
    /**
     * @brief recompiles the shader from the original file. if the shader does not compile successfully,
     * nothing is changed. if the shader is different than the last time it was loaded, descriptor set
     * layout, pipeline layout, shader stages, and the overall shader layout will be destroyed, and users
     * should update their derived resources accordingly. if the shader is in fact recompiled, the hash is
     * updated to reflect the new shader compilation, and the user can use this to detect successful
     * reloads.
     */
    void reload();

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }
    uint64_t getHash() const { return hash; }
    GPUHandle getPipelineLayout() const { return pipeline_layout; }
    bool didCompileSuccessfully() const { return load_succeeded; }
    Layout getShaderLayout() const { return { descriptor_set_layout, bindings }; }
    /**
     * @brief constructs a list describing the currently present stages within the shader. usually just
     * vertex and fragment. used to construct a pipeline.
     * @returns list of shader/pipeline stages, described as pairing of shader stage ID and VkShaderModule.
     */
    std::vector<std::pair<Stage, GPUHandle>> getShaderStages() const;

    /**
     * @brief makes the pipeline layout for this shader the currently active layout in a render command
     * buffer, allowing descriptor sets to be bound.
     * @param command_buffer draw command buffer into which the binding will be written.
     */
    void bind(WeakRef<DrawCommandBuffer> command_buffer);

private:
    /**
     * @brief uses SPIRV to reflect the uniform bindings for a given blob of compiled shader code. do we
     * need this nowadays? uh not really i guess.
     * // TODO: eliminate this and the binding merge function, use data from shader parsing instead
     * @param blob compiled shader binary.
     * @returns array of descriptor bindings found, ignoring DSL0 and DSL1 (scene and object uniforms).
     * empty if an error occurred or there were no bindings.
     */
    static std::vector<Descriptor> getReflectedBindings(const std::vector<uint32_t>& blob);
    /**
     * @brief merges together and deduplicates the uniform bindings from vertex and fragment code. performs
     * error checking.
     * @param list_a list of descriptor bindings reflected from the vertex shader.
     * @param list_b list of descriptor bindings reflected from the fragment shader.
     * @returns array of deduplicated descriptor bindings. empty if an error occurred or there were no
     * bindings.
     */
    static std::vector<Descriptor> mergeBindings(const std::vector<Descriptor>& list_a,
        const std::vector<Descriptor>& list_b);
    /**
     * @brief reads the text from the specified path and compiles it into vertex and fragment SPIRV
     * shader modules.
     * @param path path to load the shader from.
     * @param vert_blob output location for the compiled vertex module.
     * @param vert_blob output location for the compiled fragment module.
     * @returns `true` if parsing and compilation were successful, otherwise `false`.
     */
    static bool compileShaders(const std::string& path, std::vector<uint32_t>& vert_blob,
        std::vector<uint32_t>& frag_blob);
    /**
     * @brief compiles the given shader code using glslang, for the specified shader stage. the shader code
     * should already have been preprocessed at this point.
     * @param code text shader code to be compiled.
     * @param stage shader stage to compile for.
     * @param blob output lcation for the compiled shader module.
     * @param path path from which the shader was loaded, for debug messages.
     * @returns `true` if compilation was successful, otherwise `false`.
     */
    static bool compile(const std::string& code, Stage stage, std::vector<uint32_t>& blob,
        const std::string& path);
    /**
     * @brief constructs a GPU shader for the specified shader module.
     * @param blob compiled shader module.
     * @returns VkShaderModule GPU handle.
     */
    static GPUHandle createShaderModule(const std::vector<uint32_t>& blob);
    /**
     * @brief computes a hash based on the compiled shader code provided.
     * @param blob1 vertex shader blob.
     * @param blob2 fragment shader blob.
     * @returns 64-bit hash of the shader.
     */
    static uint64_t computeHash(const std::vector<uint32_t>& blob1, const std::vector<uint32_t>& blob2);

    /**
     * @brief destroys all internal resources (shader modules, pipeline layout, descriptor set layout).
     */
    void destroyResources();
};

/**
 * @brief describes a rendering pipeline, combining shader a shader with rasteriser controls such as culling
 * and polygon mode.
 */
class Pipeline final : public Destructible
{
public:
    /**
     * @brief enumerates mesh face culling mode, can be used as bitflags.
     */
    enum CullMode
    {
        CULL_NONE  = 0b00,
        CULL_FRONT = 0b01,
        CULL_BACK  = 0b10,
        CULL_BOTH  = 0b11
    };

    /**
     * @brief enumerates polygon drawing mode.
     */
    enum PolygonMode
    {
        POLYGON_FILL,
        POLYGON_LINE,
        POLYGON_POINT
    };

    /**
     * @brief enumerates comparison operations, used for depth and stencil comparisons.
     */
    enum CompareOp
    {
        COMPARE_NEVER            = 0,
        COMPARE_LESS             = 1,
        COMPARE_EQUAL            = 2,
        COMPARE_LESS_OR_EQUAL    = 3,
        COMPARE_GREATER          = 4,
        COMPARE_NOT_EQUAL        = 5,
        COMPARE_GREATER_OR_EQUAL = 6,
        COMPARE_ALWAYS           = 7,
    };

    /**
     * @brief pipeline parameter builder.
     */
    struct Builder
    {
        CullMode culling_mode          = CULL_BACK;
        PolygonMode polygon_mode       = POLYGON_FILL;
        bool depth_write_enable        = true;
        bool depth_test_enable         = true;
        CompareOp depth_compare_op     = COMPARE_LESS;
        bool stencil_enable            = false;
        CompareOp stencil_compare_op   = COMPARE_ALWAYS;
        uint32_t stencil_compare_value = 0xFFFFFFFF;
        uint32_t stencil_compare_mask  = 0xFFFFFFFF;
        uint32_t stencil_write         = 0;

        Builder& cullMode(const CullMode value)
        {
            culling_mode = value;
            return *this;
        }
        Builder& polygonMode(const PolygonMode value)
        {
            polygon_mode = value;
            return *this;
        }
        Builder& depthWrite(const bool value)
        {
            depth_write_enable = value;
            return *this;
        }
        Builder& depthTest(const bool value)
        {
            depth_test_enable = value;
            return *this;
        }
        Builder& depthOp(const CompareOp value)
        {
            depth_compare_op = value;
            return *this;
        }
        Builder& stencil()
        {
            stencil_enable = true;
            return *this;
        }
        Builder& stencilCompare(const CompareOp value, const uint32_t compare_value,
            const uint32_t compare_mask = 0xFFFFFFFF)
        {
            stencil_enable        = true;
            stencil_compare_op    = value;
            stencil_compare_value = compare_value;
            stencil_compare_mask  = compare_mask;
            return *this;
        }
        Builder& stencilWrite(const uint32_t value)
        {
            stencil_enable = true;
            stencil_write  = value;
            return *this;
        }
    };

private:
    GPUHandle pipeline = nullptr; // pipeline GPU handle
    Builder pipeline_config;      // builder struct used to construct the pipeline

public:
    DELETE_CONSTRUCTORS(Pipeline);
    /**
     * @brief construct a new pipeline based on a shader, rasteriser config, and render pass. the shader
     * provides shader modules, while the render pass provides output attachments.
     * @param shader shader providing modules to bind to the pipeline.
     * @param config rasteriser configuration, see `PipelineBuilder` for details.
     * @param render_pass render pass config in which the pipeline will be drawn.
     */
    Pipeline(Ref<Shader> shader, const Builder& config, const Framebuffer::Config& render_pass);
    ~Pipeline() override;

    /**
     * @brief binds the pipeline ready for meshes to be rendered with it.
     * @param command_buffer draw command buffer into which rendering commands will be issued.
     */
    void bind(WeakRef<DrawCommandBuffer> command_buffer);
    Builder getConfig() const { return pipeline_config; }
};

TO_STRING_DECL(Pipeline::CompareOp);
TO_STRING_DECL(Pipeline::PolygonMode);
TO_STRING_DECL(Pipeline::CullMode);

/**
 * @brief describes a descriptor set, including uniform buffers and texture-sampler bindings, with internal
 * management of descriptor set instances. essentially implements the an array of `Shader::Binding`s,
 * allowing it to be written to indirectly.
 */
class UniformBlock final : public Destructible
{
private:
    // array of descriptor sets, one per frame-in-flight
    std::vector<GPUHandle> descriptor_sets;
    // array of buffers containing uniform variables, one per descriptor set
    std::vector<Ref<Buffer>> uniform_buffers;
    // mapping between descriptor index and the texture binding
    std::map<uint32_t, std::tuple<Ref<Texture>, Ref<Sampler>>> textures_in_use;
    // CPU-accessible block of data which the program can write to
    std::vector<uint8_t> live_uniform_buffer;
    size_t size;               // size of the uniform buffer
    Shader::Layout layout;     // information about the size and offset of uniform variables
    uint32_t set_index;        // index of the descriptor set layout on which this uniform block is based
    bool rebind_needed = true; // if `true`, texture/sampler bindings have changed and need to be rebound

public:
    DELETE_CONSTRUCTORS(UniformBlock);
    /**
     * @brief creates a uniform block from a corresponding shader layout.
     * @param layout_info layout information listing the descriptor bindings.
     */
    UniformBlock(const Shader::Layout& layout_info);
    ~UniformBlock() override;

    /**
     * @brief binds the uniform block (i.e. its internally managed descriptor set) ready for shaders to be
     * executed using it. updates descriptor set bindings if needed.
     * @param command_buffer draw command buffer into which rendering commands will be issued.
     */
    void bind(WeakRef<DrawCommandBuffer> command_buffer);
    /**
     * @brief gives access to the uniform backing buffer. unless you know the layout, down to alignment, you
     * shouldn't use this (use the material's wrappers instead).
     * @returns pointer to the uniform backing buffer byte array.
     */
    void* getBuffer() { return live_uniform_buffer.data(); }
    /**
     * @brief queries the size of the uniform backing buffer.
     * @returns size of the uniform backing buffer array in bytes.
     */
    size_t getSize() const { return size; }
    /**
     * @brief update the bound texture (image view) for a given binding index.
     * @param binding descriptor binding index, matching to that specified in the shader.
     * @param texture texture to bind. if this texture is already bound, nothing will change.
     */
    void setTexture(uint32_t binding, Ref<Texture> texture);
    /**
     * @brief update the bound sampler for a given binding index.
     * @param binding descriptor binding index, matching to that specified in the shader.
     * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
     */
    void setSampler(uint32_t binding, Ref<Sampler> sampler);
    /**
     * @brief update the bound texture and sampler for a given binding index. useful when you know you need
     * to modify both at once.
     * @param binding descriptor binding index, matching to that specified in the shader.
     * @param texture texture to bind. if this texture is already bound, nothing will change.
     * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
     */
    void setTextureSampler(uint32_t binding, Ref<Texture> texture, Ref<Sampler> sampler);

    void drawImGuiDebug(const std::map<std::string, uint32_t>& texture_name_to_binding);

private:
    /**
     * @brief issues descriptor set write commands to bind the uniform buffers and textures to the
     * appropriate places. only called when needed.
     */
    void applyDescriptorBindings();
};

/**
 * @brief encapsulates a surface material used for rendering meshes. combines shader, pipeline, and uniform
 * buffer/texture bindings. allows textures and uniforms to be written to by name, as well as by binding
 * index.
 */
class Material final : public Destructible
{
private:
    std::string origin;              // if not empty, contains the path from which this shader was compiled
    Ref<Shader> shader;              // shader used by the material for rendering
    uint64_t last_known_shader_hash; // last known hash of the shader, used to check if a reload is needed
    Ref<Pipeline> pipeline;          // pipeline used by the material for rendering
    Ref<Pipeline> debug_pipeline;    // debug pipeline used when force wireframe mode is enabled
    Ref<UniformBlock> uniforms;      // uniform block providing uniform buffers and texture/sampler bindings
    Framebuffer::Config render_config; // render pass config with which the material is compatible
    // mapping from texture uniform name to its shader binding index, retrieved from shader reflection
    std::map<std::string, uint32_t> texture_name_to_binding;
    // mapping from uniform variable name to its backing buffer offset, retrieved from shader reflection
    std::map<std::string, Shader::UniformVariable> variable_name_to_binding;
    // list of previously applied material textures
    std::map<std::string, Ref<Texture>> material_textures;
    // list of previously applied material samplers
    std::map<std::string, Ref<Sampler>> material_samplers;
    // list of previously applied material uniforms
    std::map<std::string, std::vector<uint8_t>> material_parameters;

public:
    DELETE_CONSTRUCTORS(Material);
    /**
     * @brief constructs a new material based on a shader, pipeline config, and render pass. the pipeline
     * config and render pass are optional.
     * @param _shader shader to base the material on. an appropriate uniform buffer will be allocated.
     * @param config if provided, configures the pipeline (and thus rasteriser) used for the material.
     * @param _render_pass specifies the render pass configuration. the material can only be rendered in
     * this render pass, or a compatible one.
     */
    Material(Ref<Shader> _shader, const Pipeline::Builder& config, const Framebuffer::Config& _render_pass);
    Material(Ref<Shader> _shader, const Pipeline::Builder& config = Pipeline::Builder());
    ~Material() override;

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }
    Ref<Shader> getShader() const;
    Framebuffer::Config getRenderPassConfig() const { return render_config; }
    Pipeline::Builder getPipelineConfig() const { return pipeline->getConfig(); }
    /**
     * @brief creates a copy of the material using the same shader and render pass, but with unique pipeline
     * and uniform buffer resources, which can be modified independently.
     */
    Ref<Material> duplicate() const;

    /**
     * @brief binds the material ready for a mesh to be drawn using it. updates uniform buffer if needed.
     * @param command_buffer draw command buffer into which rendering commands will be issued.
     * @param wireframe_allowed if `true`, the `Engine::isWireframeMode` result determines whether the debug
     * pipeline will be rendered, otherwise only the default pipeline is used.
     */
    void bind(WeakRef<DrawCommandBuffer> command_buffer, bool wireframe_allowed = true);

    /**
     * @brief update the bound texture for a given binding index.
     * @param binding descriptor binding index, matching to that specified in the shader.
     * @param texture texture to bind. if this texture is already bound, nothing will change.
     */
    void setTexture(uint32_t binding, Ref<Texture> texture);
    /**
     * @brief update the bound sampler for a given binding index.
     * @param binding descriptor binding index, matching to that specified in the shader.
     * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
     */
    void setSampler(uint32_t binding, Ref<Sampler> sampler);
    /**
     * @brief update the bound texture for the uniform with a given name.
     * @param name uniform variable name, matching to that in the shader.
     * @param texture texture to bind. if this texture is already bound, nothing will change.
     */
    void setTexture(const std::string& name, Ref<Texture> texture);
    /**
     * @brief update the bound sampler for the uniform with a given name.
     * @param name uniform variable name, matching to that in the shader.
     * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
     */
    void setSampler(const std::string& name, Ref<Sampler> sampler);
    /**
     * @brief update the bound texture-sampler for a given binding index.
     * @param binding descriptor binding index, matching to that specified in the shader.
     * @param texture texture to bind. if this texture is already bound, nothing will change.
     * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
     */
    void setTextureSampler(uint32_t binding, Ref<Texture> texture, Ref<Sampler> sampler);
    /**
     * @brief update the bound texture-sampler uniform with a given name.
     * @param name uniform variable name, matching to that in the shader.
     * @param texture texture to bind. if this texture is already bound, nothing will change.
     * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
     */
    void setTextureSampler(const std::string& name, Ref<Texture> texture, Ref<Sampler> sampler);

    void setFloatUniform(const std::string& name, float value) { setUniform(name, &value, sizeof(value)); }
    void setVec2Uniform(const std::string& name, glm::vec2 value)
    { setUniform(name, &value, sizeof(value)); }
    void setVec3Uniform(const std::string& name, glm::vec3 value)
    { setUniform(name, &value, sizeof(value)); }
    void setVec4Uniform(const std::string& name, glm::vec4 value)
    { setUniform(name, &value, sizeof(value)); }

    void setIntUniform(const std::string& name, int value) { setUniform(name, &value, sizeof(value)); }
    void setIvec2Uniform(const std::string& name, glm::ivec2 value)
    { setUniform(name, &value, sizeof(value)); }
    void setIvec3Uniform(const std::string& name, glm::ivec3 value)
    { setUniform(name, &value, sizeof(value)); }
    void setIvec4Uniform(const std::string& name, glm::ivec4 value)
    { setUniform(name, &value, sizeof(value)); }

    void setUintUniform(const std::string& name, glm::uint value)
    { setUniform(name, &value, sizeof(value)); }

    void setBoolUniform(const std::string& name, bool value)
    {
        uint32_t tmp = value;
        setUniform(name, &tmp, sizeof(uint32_t));
    }

    void setMat2Uniform(const std::string& name, glm::mat2 value)
    { setUniform(name, &value, sizeof(value)); }
    void setMat3Uniform(const std::string& name, glm::mat3 value)
    { setUniform(name, &value, sizeof(value)); }
    void setMat4Uniform(const std::string& name, glm::mat4 value)
    { setUniform(name, &value, sizeof(value)); }

    /**
     * @brief updates a uniform variable with a particular name. allows specifying custom data. the offset
     * into the uniform backing buffer is determined via shader reflection. the size of the data is checked
     * also using reflection.
     * @param name uniform variable name, matching to that in the shader.
     * @param data pointer to the data to fill the variable with.
     * @param size expected size of the data (and uniform variable, if you're doing things right!).
     */
    void setUniform(const std::string& name, const void* data, size_t size);

    /**
     * @brief constructs a material from a text-based serialised representation.
     * @param name path to the target file from which to read the text representation.
     * @returns material constructed based on serialised representation, or `nullptr` if an error occurred
     * during deserialisation.
     */
    static Ref<Material> deserialiseFile(const std::string& name);
    /**
     * @brief constructs a material from a text-based serialised representation.
     * @example doc/MATERIAL.md
     * @param token_str text representation to decode.
     * @param origin original path from which the material was loaded, may be empty.
     * @returns material constructed based on serialised representation, or `nullptr` if an error occurred
     * during deserialisation.
     */
    static Ref<Material> deserialise(const std::string& token_str, const std::string& origin = "");

    void drawImGuiDebug();

private:
    /**
     * @brief actually initialises material resources, or replaces them.
     * @param config pipeline configuration used to create the pipelines.
     */
    void initaliseMaterial(const Pipeline::Builder& config);
};

/**
 * @brief describes a 3D scene light in shader terms. see the `LightComponent` class.
 */
struct LightParams final
{
    glm::vec4 position  = { 2, 0, 2, 0 };
    glm::vec4 direction = { -1, 0, -1, 0 };
    glm::vec4 colour    = { 1, 0, 0, 0 };
    float spot_angle    = 0.0f;
    int light_type      = 0;
    bool enabled        = false;
    float padding;
};

/**
 * @brief structure which mirrors the standard object uniform
 * buffer (i.e. descriptor set 1).
 */
struct ObjectUniforms final
{
    glm::mat4 model_to_world = glm::mat4(1);
    int id                   = 0;
};

/**
 * @brief structure which mirrors the standard scene uniform
 * buffer (i.e. descriptor set 0).
 */
struct SceneUniforms final
{
    glm::mat4 world_to_view  = glm::mat4(1);
    glm::mat4 view_to_clip   = glm::mat4(1);
    glm::mat4 clip_to_view   = glm::mat4(1);
    glm::mat4 view_to_world  = glm::mat4(1);
    glm::ivec2 viewport_size = { 0, 0 };
    glm::vec2 padding        = { 0, 0 };
    glm::vec3 eye_position   = { 0, 0, 0 };
    float time               = 0;
    glm::vec2 near_far       = { 0, 0 };
    glm::vec2 padding2       = { 0, 0 };
    LightParams lights[8];
    glm::vec4 ambient_light = { 0, 0.05f, 0.05f, 0 };
};

} // namespace HopEngine
