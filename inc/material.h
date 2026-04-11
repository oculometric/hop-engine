#pragma once

#include "common.h"

#include <glm/glm.hpp>
#include <map>

namespace HopEngine
{

class Shader final : public Destructible
{
public:
    enum Stage
    {
        STAGE_VERTEX,
        STAGE_FRAGMENT
    };

    enum DescriptorBindingType
    {
        UNIFORM,
        TEXTURE
    };

    struct UniformVariable final
    {
        std::string name;
        size_t size   = 0;
        size_t offset = 0;
    };

    struct DescriptorBinding final
    {
        uint32_t binding           = 0;
        DescriptorBindingType type = UNIFORM;
        size_t buffer_size         = 0;
        std::string name;
        std::vector<UniformVariable> variables;
        bool texture_is_3d = false;
    };

    struct Layout final
    {
        GPUHandle layout = nullptr;
        std::vector<DescriptorBinding> bindings;
        uint32_t set_index = 2;
    };

private:
    std::string origin;
    GPUHandle vert_module           = nullptr;
    GPUHandle frag_module           = nullptr;
    GPUHandle pipeline_layout       = nullptr;
    GPUHandle descriptor_set_layout = nullptr;
    std::vector<DescriptorBinding> bindings;

public:
    DELETE_CONSTRUCTORS(Shader);
    Shader(const std::string& base_path);
    ~Shader() override;

    static GPUHandle createDescriptorSetLayout(std::vector<DescriptorBinding> bindings);

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }
    GPUHandle getPipelineLayout() const { return pipeline_layout; }
    Layout getShaderLayout() const { return { descriptor_set_layout, bindings }; }
    void bind(WeakRef<DrawCommandBuffer> command_buffer);

    std::vector<std::pair<Stage, GPUHandle>> getShaderStages() const;
    bool reloadShader();

private:
    static std::vector<DescriptorBinding> mergeBindings(const std::vector<DescriptorBinding>& list_a,
        const std::vector<DescriptorBinding>& list_b);
    static std::vector<DescriptorBinding> getReflectedBindings(const std::vector<uint32_t>& blob);
    static GPUHandle createShaderModule(const std::vector<uint32_t>& blob);
    static void fixIncludes(std::string& source_code, const std::string& path_prefix, bool res_relative);
    static void preprocess(const std::string& source_code, std::string& vertex_shader_code,
        std::string& fragment_shader_code, const std::string& path);
    static std::string preprocessVertex(const std::string& common_code, const std::string& path);
    static std::string preprocessFragment(const std::string& common_code, const std::string& path);
    static void removeFunction(std::string& code, const std::string& signature);
    static void destroyAllPragmas(std::string& code);
    static bool compileShaders(const std::string& path, std::vector<uint32_t>& vert_blob,
        std::vector<uint32_t>& frag_blob);
    static bool compile(const std::string& code, Stage stage, std::vector<uint32_t>& blob, const std::string& path);

    void destroyResources();
};

class Pipeline final : public Destructible
{
public:
    /**
     * @brief enumerates mesh face culling mode
     */
    enum CullMode
    {
        CULL_NONE  = 0,
        CULL_FRONT = 1,
        CULL_BACK  = 2,
        CULL_BOTH  = 3
    };

    /**
     * @brief enumerates polygon drawing mode
     */
    enum PolygonMode
    {
        POLYGON_FILL,
        POLYGON_LINE,
        POLYGON_POINT
    };

    /**
     * @brief enumerates comparison operations
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
    GPUHandle pipeline = nullptr;
    Builder pipeline_config;

public:
    DELETE_CONSTRUCTORS(Pipeline);
    Pipeline(const Ref<Shader>& shader, const Builder& config, const Ref<RenderPass>& render_pass);
    ~Pipeline() override;

    void bind(WeakRef<DrawCommandBuffer> command_buffer);
    Builder getConfig() const { return pipeline_config; }
};

TO_STRING_DECL(Pipeline::CompareOp);
TO_STRING_DECL(Pipeline::PolygonMode);
TO_STRING_DECL(Pipeline::CullMode);

class UniformBlock final : public Destructible
{
private:
    // array of descriptor sets
    std::vector<GPUHandle> descriptor_sets;
    // array of buffers containing uniform variables, one per descriptor set
    std::vector<Ref<Buffer>> uniform_buffers;
    // mapping between descriptor index and the texture binding
    std::map<uint32_t, std::tuple<Ref<Texture>, Ref<Sampler>>> textures_in_use;
    // CPU-accessible block of data which the program can write to
    std::vector<uint8_t> live_uniform_buffer;
    size_t size;           // size of the uniform buffer
    Shader::Layout layout; // information about the size and offset of uniform variables
    uint32_t set_index;
    bool rebind_needed = true;

public:
    DELETE_CONSTRUCTORS(UniformBlock);
    /**
     * @brief creates a uniform block from a corresponding shader layout.
     * @param layout_info layout information listing the descriptor bindings.
     */
    UniformBlock(const Shader::Layout& layout_info);
    ~UniformBlock() override;

    void bind(WeakRef<DrawCommandBuffer> command_buffer);
    void* getBuffer() { return live_uniform_buffer.data(); }
    size_t getSize() const { return size; }
    /**
     * @brief update the bound texture (image view) for a given binding index.
     * @param binding descriptor binding index, matching to that specified in the shader.
     * @param image texture to bind. if this image is already bound, nothing will change.
     * @param use_stencil whether the stencil view aspect should be used.
     */
    void setTexture(uint32_t binding, Ref<Texture> image);
    /**
     * @brief update the bound sampler for a given binding index.
     * @param binding descriptor binding index, matching to that specified in the shader.
     * @param sampler sampler to bind. if this sampler is already bound, nothing will change.
     */
    void setSampler(uint32_t binding, Ref<Sampler> sampler);
    void setTextureSampler(uint32_t binding, Ref<Texture> texture, Ref<Sampler> sampler);

    void drawImGuiDebug(const std::map<std::string, uint32_t>& texture_name_to_binding);

private:
    /**
     * @brief issues descriptor set write commands to bind the uniform buffers and
     * textures to the appropriate places.
     */
    void applyDescriptorBindings();
};

class Material final : public Destructible
{
private:
    std::string origin;
    Ref<Shader> shader;
    Ref<Pipeline> pipeline;
    Ref<Pipeline> debug_pipeline;
    Ref<UniformBlock> uniforms;
    Ref<RenderPass> render_pass;
    std::map<std::string, uint32_t> texture_name_to_binding;
    std::map<std::string, Shader::UniformVariable> variable_name_to_binding;

public:
    DELETE_CONSTRUCTORS(Material);
    Material(Ref<Shader> _shader, const Pipeline::Builder& config = Pipeline::Builder(),
        WeakRef<RenderPass> _render_pass = nullptr);
    ~Material() override;

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }
    Ref<Shader> getShader() const;
    Ref<RenderPass> getRenderPass() const;
    Ref<Material> duplicate() const;

    void bind(WeakRef<DrawCommandBuffer> command_buffer, bool wireframe_allowed = true);

    void setTexture(uint32_t binding, Ref<Texture> texture);
    void setSampler(uint32_t binding, Ref<Sampler> sampler);
    void setTexture(const std::string& name, Ref<Texture> texture);
    void setSampler(const std::string& name, Ref<Sampler> sampler);
    void setTextureSampler(uint32_t binding, Ref<Texture> texture, Ref<Sampler> sampler);
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

    void setBoolUniform(const std::string& name, bool value) { setUniform(name, &value, sizeof(uint32_t)); }

    void setMat2Uniform(const std::string& name, glm::mat2 value)
    { setUniform(name, &value, sizeof(value)); }
    void setMat3Uniform(const std::string& name, glm::mat3 value)
    { setUniform(name, &value, sizeof(value)); }
    void setMat4Uniform(const std::string& name, glm::mat4 value)
    { setUniform(name, &value, sizeof(value)); }

    void setUniform(const std::string& name, const void* data, size_t size);

    static Ref<Material> deserialise(const std::string& name);

    void drawImGuiDebug();
};

/**
 * @brief describes a 3D scene light. see the \code LightComponent\endcode class.
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
    glm::mat4 model_to_world;
    int id;
};

/**
 * @brief structure which mirrors the standard scene uniform
 * buffer (i.e. descriptor set 0).
 */
struct SceneUniforms final
{
    glm::mat4 world_to_view;
    glm::mat4 view_to_clip;
    glm::mat4 clip_to_view;
    glm::mat4 view_to_world;
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
