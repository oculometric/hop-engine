#include "command_buffer.h"
#include "material.h"
#include "package.h"
#include "render_server.h"

#include <filesystem>

using namespace HopEngine;

Shader::Shader(const std::string& base_path)
{
    origin = base_path;
    std::vector<uint32_t> vert_blob;
    std::vector<uint32_t> frag_blob;
    if (!compileShaders(base_path, vert_blob, frag_blob))
    {
        DBG_ERROR(base_path + " shader compilation failed");
        if (!compileShaders("res://engine/shaders/default_shader.glsl", vert_blob, frag_blob))
            DBG_FAULT("failed to load default shader!");
    }

    vert_module = createShaderModule(vert_blob);
    frag_module = createShaderModule(frag_blob);

    const auto vert_bindings = getReflectedBindings(vert_blob);
    const auto frag_bindings = getReflectedBindings(frag_blob);

    bindings = mergeBindings(vert_bindings, frag_bindings);

    descriptor_set_layout = createDescriptorSetLayout(bindings);

    pipeline_layout = RenderServer::createPipelineLayout(descriptor_set_layout);

    DBG_VERBOSE("created shader from " + base_path);
}

Shader::~Shader()
{
    DBG_VERBOSE("destroyed shader '" + getOrigin() + '\'');

    destroyResources();
}

void Shader::bind(WeakRef<DrawCommandBuffer> command_buffer)
{ command_buffer->bindPipelineLayoutInternal(pipeline_layout); }

bool Shader::reloadShader()
{
    DBG_WARNING("shader reloading is not implemented. this function does nothing.");

    return true;
}

std::vector<Shader::DescriptorBinding> Shader::mergeBindings(const std::vector<DescriptorBinding>& list_a,
    const std::vector<DescriptorBinding>& list_b)
{
    std::multimap<uint32_t, DescriptorBinding> bindings;

    for (const auto& item : list_a) bindings.insert({ item.binding, item });
    for (const auto& item : list_b) bindings.insert({ item.binding, item });

    if (bindings.empty()) return {};
    if (bindings.size() == 1) return { bindings.begin()->second };

    std::vector<DescriptorBinding> resolved_bindings;

    auto binding_it = bindings.begin();
    while (binding_it != bindings.end())
    {
        DescriptorBinding last_binding = binding_it->second;
        resolved_bindings.push_back(last_binding);
        ++binding_it;
        if (binding_it == bindings.end()) return resolved_bindings;
        if (binding_it->first == last_binding.binding)
        {
            // uh oh! duplicate bindings! that's not good...
            if (binding_it->second.type == last_binding.type &&
                binding_it->second.buffer_size == last_binding.buffer_size)
                ++binding_it;
            else
            {
                DBG_ERROR("incompatible duplicate shader uniform/texture bindings found");
                ++binding_it;
            }
        }
    }

    return resolved_bindings;
}

void Shader::fixIncludes(std::string& source_code_text, const std::string& path_prefix,
    const bool res_relative)
{
    static const std::string include_search = "#include \"";
    size_t offset                           = source_code_text.find(include_search, 0);
    while (offset != std::string::npos)
    {
        const size_t start       = offset + include_search.size();
        const size_t end         = source_code_text.find('\"', start);
        std::string include_path = source_code_text.substr(start, end - start);
        if (include_path.find(' ') != std::string::npos)
        {
            DBG_ERROR("malformed include found!");
            return;
        }
        source_code_text.erase(offset, (end - offset) + 1);
        std::string real_path;
        if (include_path.starts_with("res://")) { real_path = include_path; }
        else
        {
            std::string target_path          = path_prefix + include_path;
            std::filesystem::path fixed_path = target_path;
            auto lex                         = fixed_path.lexically_normal();
            real_path                        = lex.string();
            for (char& value : real_path)
                if (value == '\\') value = '/';
            if (res_relative) real_path = "res://" + real_path;
        }
        auto include_data = Package::load(real_path);
        if (include_data.empty())
        {
            DBG_WARNING("included file " + real_path + " did not exist, or contained no data!");
        }
        std::string include_string(include_data.size(), ' ');
        memcpy(include_string.data(), include_data.data(), include_data.size());
        source_code_text.insert(source_code_text.begin() + static_cast<long long>(offset),
            include_data.begin(), include_data.end());

        offset = source_code_text.find(include_search, offset);
    }
}

static const std::string vertex_function_sig   = "void vertex()";
static const std::string fragment_function_sig = "void fragment()";

static const std::string version_str          = "#version 450\n\n";
static const std::string descriptor_set_0_str = R"V0G0N(struct Light
{
	vec4 position;
	vec4 direction;
	vec4 colour;
	float spot_angle;
	int type;
	bool enabled;
	float padding;
};

layout(set = 0, binding = 0) uniform SceneUniforms
{
	mat4 world_to_view;
	mat4 view_to_clip;
	mat4 clip_to_view;
	mat4 view_to_world;
	ivec2 viewport_size;
	vec3 eye_position;
	float time;
	vec2 near_far;
	Light lights[8];
	vec4 ambient_light;
} scene;

)V0G0N";

static const std::string omit_object_uniform_pragma = "#pragma OMIT_OBJECT_UNIFORMS";
static const std::string descriptor_set_1_str = R"V0G0N(layout(set = 1, binding = 0) uniform ObjectUniforms
{
	mat4 model_to_world;
	int id;
} object;

)V0G0N";

static const std::string vertex_fragment_varyings = R"VOGON(struct Frag
{
	vec4 position;
	vec4 colour;
	vec4 normal;
	vec4 tangent;
	vec3 uv;
};

)VOGON";

static const std::string vertex_input_default = R"VOGON(layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_colour;
layout(location = 2) in vec4 in_normal;
layout(location = 3) in vec4 in_tangent;
layout(location = 4) in vec3 in_uv;
)VOGON";

static const std::string vertex_outputs = "layout(location = 0) out Frag frag;\n";

static const std::string default_transform_pragma = "#pragma DEFAULT_TRANSFORM";
static const std::string default_vertex_transform =
    R"VOGON(frag.position = object.model_to_world * vec4(in_position.xyz, 1);
	frag.colour = in_colour;
	frag.normal = vec4(normalize((object.model_to_world * vec4(in_normal.xyz, 0)).xyz), 0);
	frag.tangent = vec4(normalize((object.model_to_world * vec4(in_tangent.xyz, 0)).xyz), 0);
	frag.uv = in_uv;
	gl_Position = scene.view_to_clip * scene.world_to_view * frag.position;)VOGON";
static const std::string canvas_transform_pragma = "#pragma CANVAS_TRANSFORM";
static const std::string canvas_vertex_transform = R"VOGON(frag.position = vec4(in_position.xyz, 1);
	frag.uv = in_uv;
	gl_Position = vec4(in_position.xyz, 1);)VOGON";

static const std::string fragment_inputs = "layout(location = 0) in Frag frag;\n";

static const std::string default_attachments = R"VOGON(layout(location = 0) out vec4 out_colour;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_params;
layout(location = 3) out vec4 out_custom;)VOGON";
static const std::string canvas_attachments  = R"VOGON(layout(location = 0) out vec4 out_colour;)VOGON";
static const std::string default_attachment_pragma = "#pragma DEFAULT_ATTACHMENTS";
static const std::string canvas_attachment_pragma  = "#pragma CANVAS_ATTACHMENTS";

void Shader::preprocess(const std::string& source_code, std::string& vertex_shader_code,
    std::string& fragment_shader_code, const std::string& path)
{
    std::string common_code = source_code;

    // remove comments
    size_t comment_pos = common_code.find("//");
    while (comment_pos != std::string::npos)
    {
        size_t comment_end = common_code.find('\n', comment_pos);
        common_code.erase(comment_pos, (comment_end - comment_pos) + 1);
        comment_pos = common_code.find("//", comment_pos);
    }

    comment_pos = common_code.find("/*");
    while (comment_pos != std::string::npos)
    {
        size_t comment_end = common_code.find("*/", comment_pos);
        common_code.erase(comment_pos, (comment_end - comment_pos) + 2);
        comment_pos = common_code.find("/*", comment_pos);
    }

    // top-of-file version and descriptor set preprocessing
    common_code.insert(0, version_str);
    common_code.insert(version_str.size(), descriptor_set_0_str);
    size_t omit_object_uniform_pragma_pos = common_code.find(omit_object_uniform_pragma);
    if (omit_object_uniform_pragma_pos != std::string::npos)
        common_code.erase(omit_object_uniform_pragma_pos, omit_object_uniform_pragma.size());
    else
        common_code.insert(version_str.size() + descriptor_set_0_str.size(), descriptor_set_1_str);

    // check if vertex and fragment funcs are present and have the correct signature
    size_t vert_func_pos = common_code.find(vertex_function_sig);
    if (vert_func_pos == std::string::npos)
    {
        DBG_ERROR("error preprocessing " + path +
                  ": vertex shader function is missing or has the wrong signature.");
        return;
    }
    size_t frag_func_pos = common_code.find(fragment_function_sig);
    if (frag_func_pos == std::string::npos)
    {
        DBG_ERROR("error preprocessing " + path +
                  ": fragment shader function is missing or has the wrong signature.");
        return;
    }

    common_code.insert(vert_func_pos, vertex_fragment_varyings);

    // TODO: format/layout uniforms

    vertex_shader_code   = preprocessVertex(common_code, path);
    fragment_shader_code = preprocessFragment(common_code, path);
}

std::string Shader::preprocessVertex(const std::string& common_code, const std::string& path)
{
    std::string result_code = common_code;
    size_t vert_func_pos    = result_code.find(vertex_function_sig);

    // vertex inputs
    result_code.insert(vert_func_pos, vertex_input_default);
    vert_func_pos = result_code.find(vertex_function_sig);

    // vertex outputs
    result_code.insert(vert_func_pos, vertex_outputs);

    // TODO: custom varyings support

    // default/canvas vertex transform
    size_t default_transform_pragma_pos = result_code.find(default_transform_pragma);
    size_t canvas_transform_pragma_pos  = result_code.find(canvas_transform_pragma);

    if (default_transform_pragma_pos != std::string::npos)
    {
        if (canvas_transform_pragma_pos != std::string::npos)
            DBG_WARNING(
                "error preprocessing " + path +
                ": only DEFAULT_TRANSFORM or CANVAS_TRANSFORM should be used in a shader, not both at once.");
        result_code.erase(default_transform_pragma_pos, default_transform_pragma.size());
        result_code.insert(default_transform_pragma_pos, default_vertex_transform);
    }
    else if (canvas_transform_pragma_pos != std::string::npos)
    {
        result_code.erase(canvas_transform_pragma_pos, canvas_transform_pragma.size());
        result_code.insert(canvas_transform_pragma_pos, canvas_vertex_transform);
    }

    removeFunction(result_code, fragment_function_sig);
    destroyAllPragmas(result_code);

    return result_code;
}

std::string Shader::preprocessFragment(const std::string& common_code, const std::string& path)
{
    std::string result_code = common_code;
    size_t frag_func_pos    = result_code.find(fragment_function_sig);

    // fragment inputs
    result_code.insert(frag_func_pos, fragment_inputs);

    // default/canvas fragment outputs
    size_t default_attachment_pragma_pos = result_code.find(default_attachment_pragma);
    size_t canvas_attachment_pragma_pos  = result_code.find(canvas_attachment_pragma);

    if (default_attachment_pragma_pos != std::string::npos)
    {
        if (canvas_attachment_pragma_pos != std::string::npos)
            DBG_WARNING(
                "error preprocessing " + path +
                ": only DEFAULT_ATTACHMENTS or CANVAS_ATTACHMENTS should be used in a shader, not both at once.");
        result_code.erase(default_attachment_pragma_pos, default_attachment_pragma.size());
        result_code.insert(default_attachment_pragma_pos, default_attachments);
    }
    else if (canvas_attachment_pragma_pos != std::string::npos)
    {
        result_code.erase(canvas_attachment_pragma_pos, canvas_attachment_pragma.size());
        result_code.insert(canvas_attachment_pragma_pos, canvas_attachments);
    }

    removeFunction(result_code, vertex_function_sig);
    destroyAllPragmas(result_code);

    return result_code;
}

void Shader::removeFunction(std::string& code, const std::string& signature)
{
    size_t function_start = code.find(signature);
    size_t function_end   = function_start + signature.size();
    size_t steps_deep     = 0;
    while (function_end < code.size())
    {
        if (code[function_end] == '{') ++steps_deep;
        else if (code[function_end] == '}')
        {
            --steps_deep;
            if (steps_deep == 0) break;
        }
        ++function_end;
    }
    code.erase(function_start, (function_end - function_start) + 1);
}

void Shader::destroyAllPragmas(std::string& code)
{
    size_t pragma_pos = code.find("#pragma");
    while (pragma_pos != std::string::npos)
    {
        size_t end_pos = code.find('\n', pragma_pos);
        code.erase(pragma_pos, (end_pos - pragma_pos) + 1);
        pragma_pos = code.find("#pragma", pragma_pos);
    }
}

bool Shader::compileShaders(const std::string& path, std::vector<uint32_t>& vert_blob,
    std::vector<uint32_t>& frag_blob)
{
    auto shader_data = Package::load(path);

    if (shader_data.empty())
    {
        DBG_ERROR("shader " + path + " not found");
        return false;
    }

    std::filesystem::path current_file_path;
    bool is_res_relative = false;
    if (path.starts_with("res://"))
    {
        current_file_path = path.substr(6);
        is_res_relative   = true;
    }
    else
        current_file_path = path;
    const std::string current_file_location = current_file_path.remove_filename().string();

    std::string shader_text;
    shader_text.resize(shader_data.size());
    memcpy(shader_text.data(), shader_data.data(), shader_data.size());

    Shader::fixIncludes(shader_text, current_file_location, is_res_relative);
    std::string vertex_text;
    std::string fragment_text;
    Shader::preprocess(shader_text, vertex_text, fragment_text, path);
    if (vertex_text.empty() || fragment_text.empty()) return false;

    if (!compile(vertex_text, STAGE_VERTEX, vert_blob, path)) return false;
    if (!compile(fragment_text, STAGE_FRAGMENT, frag_blob, path)) return false;

    return true;
}
