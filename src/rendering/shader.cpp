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

std::vector<Shader::Descriptor> Shader::mergeBindings(const std::vector<Descriptor>& list_a,
    const std::vector<Descriptor>& list_b)
{
    std::multimap<uint32_t, Descriptor> bindings;

    for (const auto& item : list_a) bindings.insert({ item.binding, item });
    for (const auto& item : list_b) bindings.insert({ item.binding, item });

    if (bindings.empty()) return {};
    if (bindings.size() == 1) return { bindings.begin()->second };

    std::vector<Descriptor> resolved_bindings;

    auto binding_it = bindings.begin();
    while (binding_it != bindings.end())
    {
        Descriptor last_binding = binding_it->second;
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

struct ShaderToken
{
    std::string value;
};

class ShaderParser final
{
private:
    bool no_object_uniforms = false;
    bool canvas_transform   = false;
    bool no_transform       = false;
    bool canvas_attachments = false;

    std::string file_path;
    std::string vertex_shader;
    std::string fragment_shader;

public:
    ShaderParser(const std::string& source, const std::string& file);

    std::string getVertexShader() { return vertex_shader; }
    std::string getFragmentShader() { return fragment_shader; }

private:
    void processPragmas(std::string& source);
    void processIncludes(std::string& source_code_text, const std::string& path_prefix,
        const bool res_relative);
    void processUniforms(std::string& source);
    void processShaders(std::string& source);
};

ShaderParser::ShaderParser(const std::string& source, const std::string& file)
{
    file_path = file;
    std::string modified_source;
    std::filesystem::path current_file_path;
    bool is_res_relative = false;
    if (file.starts_with("res://"))
    {
        current_file_path = file.substr(6);
        is_res_relative   = true;
    }
    else
        current_file_path = file;
    const std::string current_file_location = current_file_path.remove_filename().string();

    processPragmas(modified_source);
    processIncludes(modified_source, current_file_location, is_res_relative);
    processUniforms(modified_source);
    processShaders(modified_source);
}

void ShaderParser::processIncludes(std::string& source_code_text, const std::string& path_prefix,
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

static const std::string omit_object_uniform_pragma_str = "#pragma OMIT_OBJECT_UNIFORMS";
static const std::string canvas_transform_pragma_str    = "#pragma CANVAS_TRANSFORM";
static const std::string omit_transform_pragma_str      = "#pragma OMIT_TRANSFORM";
static const std::string canvas_attachment_pragma_str   = "#pragma CANVAS_ATTACHMENTS";

// TODO: process pragmas

// TODO: process uniforms

static const std::string light_struct_str = R"VOGON(struct Light
{
    vec4 position;
    vec4 direction;
    vec4 colour;
    float spot_angle;
    int type;
    bool enabled;
    float padding;
};

)VOGON";

static const std::string descriptor_set_0_str = R"VOGON(layout(set = 0, binding = 0) uniform SceneUniforms
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

)VOGON";

static const std::string descriptor_set_1_str = R"VOGON(layout(set = 1, binding = 0) uniform ObjectUniforms
{
    mat4 model_to_world;
    int id;
} object;

)VOGON";

static const std::string varyings_struct_str = R"VOGON(struct Varyings
{
    vec4 position;
    vec4 colour;
    vec4 normal;
    vec4 tangent;
    vec3 uv;
};

)VOGON";

static const std::string vertex_struct_str = R"VOGON(struct Vertex
{
    vec4 position;
    vec4 colour;
    vec4 normal;
    vec4 tangent;
    vec3 uv;
};

)VOGON";

static const std::string fragment_struct_str = R"VOGON(struct Fragment
{
    vec4 out_colour;
    vec4 out_normal;
    vec4 out_params;
    vec4 out_custom;
};

)VOGON";

static const std::string vertex_main_str[6] = {
    R"VOGON(
layout(location = 0) in vec4 in_position;
layout(location = 1) in vec4 in_colour;
layout(location = 2) in vec4 in_normal;
layout(location = 3) in vec4 in_tangent;
layout(location = 4) in vec3 in_uv;
layout(location = 0) out Varyings __varyings;)VOGON",
    R"VOGON(
void main()
{
    Vertex vert;
    vert.position = in_position;
    vert.colour = in_colour;
    vert.normal = in_normal
    vert.tangent = in_tangent;
    vert.uv = in_uv;

    Varyings vars;)VOGON",
    R"VOGON(
    vars.position = object.model_to_world * vec4(vert.position.xyz, 1);
    vars.colour = vert.colour;
    vars.normal = vec4(normalize((object.model_to_world * vec4(vert.normal.xyz, 0)).xyz), 0);
    vars.tangent = vec4(normalize((object.model_to_world * vec4(vert.tangent.xyz, 0)).xyz), 0);
    vars.uv = vert.uv;
    gl_Position = scene.view_to_clip * scene.world_to_view * vars.position;)VOGON",
    R"VOGON(
    vertex(vert, gl_Position, vars)VOGON",
    R"VOGON();

    __varyings = vars;)VOGON",
    R"VOGON(
})VOGON",
};

static const std::string vertex_main_str_2_canvas = R"VOGON(
    vars.position = vec4(vert.position.xyz, 1);
    vars.colour = vert.colour;
    vars.normal = vert.normal;
    vars.tangent = vert.tangent;
    vars.uv = vert.uv;
    gl_Position = vec4(vert.position.xyz, 1);)VOGON";

static const std::string fragment_main_str[7] = {
    R"VOGON(
layout(location = 0) in Varyings __varyings;)VOGON",
    R"VOGON(
layout(location = 0) out vec4 out_colour;)VOGON",
    R"VOGON(
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_params;
layout(location = 3) out vec4 out_custom;)VOGON",
    R"VOGON(
void main()
{
    Fragment frag = fragment(__varyings)VOGON",
    R"VOGON();
    out_colour = frag.out_colour;)VOGON",
    R"VOGON(
    out_normal = frag.out_normal;
    out_params = frag.out_params;
    out_custom = frag.out_custom;)VOGON",
    R"VOGON(
})VOGON"
};

void ShaderParser::processShaders(std::string& source)
{
    // TODO: check for pragmas:
    //    - omit object uniforms
    //    - canvas transform
    //    - omit transform
    //    - canvas attachments

    // add descriptor layouts 0 and 1 at the start
    size_t insert_point = 0;
    source.insert(insert_point, light_struct_str);
    insert_point += light_struct_str.size();
    source.insert(insert_point, descriptor_set_0_str);
    insert_point += descriptor_set_0_str.size();
    if (!no_object_uniforms)
    {
        source.insert(insert_point, descriptor_set_1_str);
        insert_point += descriptor_set_1_str.size();
    }

    // add Vertex, Varyings, and Fragment structs
    source.insert(insert_point, vertex_struct_str);
    insert_point += vertex_struct_str.size();
    source.insert(insert_point, varyings_struct_str);
    insert_point += varyings_struct_str.size();
    source.insert(insert_point, fragment_struct_str);
    insert_point += fragment_struct_str.size();

    std::vector<ShaderToken> tokens;
    // TODO: detect vertex and fragment functions
    //    - detect location and isolate code (not needed?)
    //    - detect signature, validate, and detect extra varying structs
    //    - insert appropriate main function

    {
        vertex_shader = source;
        vertex_shader.append(vertex_main_str[0]);
        // TODO: insert extra varying outputs here
        vertex_shader.append(vertex_main_str[1]);
        // TODO: declare extra varying structs here
        if (!canvas_transform && !no_transform) vertex_shader.append(vertex_main_str[2]);
        else if (canvas_transform)
            vertex_shader.append(vertex_main_str_2_canvas);
        vertex_shader.append(vertex_main_str[3]);
        // TODO: insert extra vertex arguments (structs) here
        vertex_shader.append(vertex_main_str[4]);
        // TOOD: assign varying outputs here
        vertex_shader.append(vertex_main_str[5]);
    }

    {
        fragment_shader = source;
        fragment_shader.append(fragment_main_str[0]);
        // TODO: insert extra varying inputs here
        fragment_shader.append(fragment_main_str[1]);
        if (!canvas_attachments) fragment_shader.append(fragment_main_str[2]);
        fragment_shader.append(fragment_main_str[3]);
        // TODO: insert extra fragment arguments (structs) here
        fragment_shader.append(fragment_main_str[4]);
        if (!canvas_attachments) fragment_shader.append(fragment_main_str[5]);
        fragment_shader.append(fragment_main_str[6]);
    }
}

//
//
//
//

// void Shader::destroyAllPragmas(std::string& code)
// {
//     size_t pragma_pos = code.find("#pragma");
//     while (pragma_pos != std::string::npos)
//     {
//         size_t end_pos = code.find('\n', pragma_pos);
//         code.erase(pragma_pos, (end_pos - pragma_pos) + 1);
//         pragma_pos = code.find("#pragma", pragma_pos);
//     }
// }

bool Shader::compileShaders(const std::string& path, std::vector<uint32_t>& vert_blob,
    std::vector<uint32_t>& frag_blob)
{
    auto shader_data = Package::load(path);

    if (shader_data.empty())
    {
        DBG_ERROR("shader " + path + " not found");
        return false;
    }

    std::string shader_text;
    shader_text.resize(shader_data.size());
    memcpy(shader_text.data(), shader_data.data(), shader_data.size());

    ShaderParser parser(shader_text, path);
    if (parser.getVertexShader().empty() || parser.getFragmentShader().empty()) return false;

    if (!compile(parser.getVertexShader(), STAGE_VERTEX, vert_blob, path)) return false;
    if (!compile(parser.getFragmentShader(), STAGE_FRAGMENT, frag_blob, path)) return false;

    return true;
}
