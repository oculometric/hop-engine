#include "material.h"
#include "package.h"

#include <filesystem>
#include <format>

using namespace HopEngine;

struct ShaderToken
{
    std::string text;
    size_t start;
    int type;
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

    bool success = true;

public:
    ShaderParser(const std::string& source, const std::string& file);

    std::string getVertexShader() { return vertex_shader; }
    std::string getFragmentShader() { return fragment_shader; }

private:
    void processComments(std::string& source);
    void processIncludes(std::string& source_code_text, const std::string& path_prefix,
        const bool res_relative);
    void processPragmas(std::string& source);
    void processUniforms(std::string& source);
    void processShaders(std::string& source);

    bool extractFunction(size_t* shader_locations, std::vector<ShaderToken>::iterator& it,
        size_t& token_index, const std::vector<ShaderToken>::iterator begin,
        const std::vector<ShaderToken>::iterator end, const std::string& shader,
        const std::string& ret_type);
};

ShaderParser::ShaderParser(const std::string& source, const std::string& file)
{
    file_path                   = file;
    std::string modified_source = source;
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

    processComments(modified_source);
    if (!success) return;
    processIncludes(modified_source, current_file_location, is_res_relative);
    if (!success) return;
    processPragmas(modified_source);
    if (!success) return;
    processUniforms(modified_source);
    if (!success) return;
    processShaders(modified_source);
}

void ShaderParser::processComments(std::string& source)
{
    {
        size_t comment_pos = source.find("/*");
        while (comment_pos != std::string::npos)
        {
            size_t comment_end = source.find("*/", comment_pos);
            if (comment_end == std::string::npos)
            {
                DBG_ERROR("error parsing shader " + file_path + ": unterminated multiline comment");
                success = false;
                return;
            }
            source.erase(comment_pos, (comment_end - comment_pos) + 2);
            comment_pos = source.find("/*", comment_pos);
        }
    }

    {
        size_t comment_pos = source.find("//");
        while (comment_pos != std::string::npos)
        {
            size_t comment_end = source.find('\n', comment_pos);
            source.erase(comment_pos, comment_end - comment_pos);
            comment_pos = source.find("//", comment_pos);
        }
    }
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
            DBG_ERROR("error parsing shader " + file_path + ": malformed include found!");
            success = false;
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
            DBG_WARNING("error parsing shader " + file_path + ": included file " + real_path +
                        " did not exist, or contained no data!");
        }
        std::string include_string(include_data.size(), ' ');
        memcpy(include_string.data(), include_data.data(), include_data.size());
        source_code_text.insert(source_code_text.begin() + static_cast<long long>(offset),
            include_data.begin(), include_data.end());

        offset = source_code_text.find(include_search, offset);
    }
}

static const std::string omit_object_uniform_pragma_str = "OMIT_OBJECT_UNIFORMS";
static const std::string canvas_transform_pragma_str    = "CANVAS_TRANSFORM";
static const std::string omit_transform_pragma_str      = "OMIT_TRANSFORM";
static const std::string canvas_attachment_pragma_str   = "CANVAS_ATTACHMENTS";

void ShaderParser::processPragmas(std::string& source)
{
    static const std::string pragma_search = "#pragma ";
    size_t offset                          = source.find(pragma_search);
    while (offset != std::string::npos)
    {
        const size_t start = offset + pragma_search.size();
        const size_t end =
            glm::min(source.find(' ', start), glm::min(source.find('\n', start), source.find('\r', start)));
        std::string pragma_str = source.substr(start, end - start);
        source.erase(offset, (end - offset) + 1);

        if (pragma_str == omit_object_uniform_pragma_str) no_object_uniforms = true;
        else if (pragma_str == canvas_transform_pragma_str)
        {
            if (no_transform)
            {
                DBG_ERROR("error parsing shader " + file_path + ": " + omit_transform_pragma_str + " and " +
                          canvas_transform_pragma_str + " cannot be used in the same shader.");
                success = false;
                return;
            }
            canvas_transform = true;
        }
        else if (pragma_str == omit_transform_pragma_str)
        {
            if (canvas_transform)
            {
                DBG_ERROR("error parsing shader " + file_path + ": " + omit_transform_pragma_str + " and " +
                          canvas_transform_pragma_str + " pragmas cannot be used in the same shader.");
                success = false;
                return;
            }
            no_transform = true;
        }
        else if (pragma_str == canvas_attachment_pragma_str)
            canvas_attachments = true;
        else
        {
            DBG_ERROR("error parsing shader " + file_path + ": unrecognised pragma " + pragma_str);
            success = false;
            return;
        }

        offset = source.find(pragma_search, offset);
    }
}

void ShaderParser::processUniforms(std::string& source)
{
    static const std::string uniform_search = "uniform ";
    size_t uniform_index                    = 0;
    size_t offset                           = source.find(uniform_search);
    while (offset != std::string::npos)
    {
        std::string insert_str = std::format("layout(set = 2, binding = {}) ", uniform_index);
        source.insert(offset, insert_str);
        ++uniform_index;
        offset = source.find(uniform_search, offset + uniform_search.size() + insert_str.size());
    }
}

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
    vec4 colour;
    vec4 normal;
    vec4 params;
    vec4 custom;
};

)VOGON";

static const std::string vertex_main_str[6] = {
    R"VOGON(
layout(location = 0) in vec4 _HEI_in_position;
layout(location = 1) in vec4 _HEI_in_colour;
layout(location = 2) in vec4 _HEI_in_normal;
layout(location = 3) in vec4 _HEI_in_tangent;
layout(location = 4) in vec3 _HEI_in_uv;
layout(location = 0) out Varyings _HEI_varyings;)VOGON",
    R"VOGON(
void main()
{
    Vertex vert;
    vert.position = _HEI_in_position;
    vert.colour = _HEI_in_colour;
    vert.normal = _HEI_in_normal;
    vert.tangent = _HEI_in_tangent;
    vert.uv = _HEI_in_uv;

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

    _HEI_varyings = vars;)VOGON",
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
layout(location = 0) in Varyings _HEI_varyings;)VOGON",
    R"VOGON(
layout(location = 0) out vec4 _HEI_out_colour;)VOGON",
    R"VOGON(
layout(location = 1) out vec4 _HEI_out_normal;
layout(location = 2) out vec4 _HEI_out_params;
layout(location = 3) out vec4 _HEI_out_custom;)VOGON",
    R"VOGON(
void main()
{
    Fragment frag;
    frag.colour = vec4(0, 0, 0, 0);
    frag.normal = vec4(0, 0, 0, 0);
    frag.params = vec4(0, 0, 0, 0);
    frag.custom = vec4(0, 0, 0, 0);
    if (!fragment(_HEI_varyings)VOGON",
    R"VOGON(, frag))
        discard;
    _HEI_out_colour = frag.colour;)VOGON",
    R"VOGON(
    _HEI_out_normal = frag.normal;
    _HEI_out_params = frag.params;
    _HEI_out_custom = frag.custom;)VOGON",
    R"VOGON(
})VOGON"
};

static const std::string vertex_function_def[2] = {
    R"VOGON(void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars))VOGON",
    R"VOGON({}
)VOGON"
};

static const std::string fragment_function_def[2] = {
    R"VOGON(bool fragment(in Varyings vars, inout Fragment frag))VOGON", R"VOGON({
    frag.colour = vec4(mod(abs(vars.position.xy * 4.0f), 1.0f), 0, 1);
    return true;
}
)VOGON"
};

int getType(char c)
{
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') return -1;
    else if (c == ' ' || c == '\n' || c == '\r' || c == '\t')
        return -2;
    else
        return c;
}

std::vector<std::pair<ShaderToken, ShaderToken>> getArguments(const std::vector<ShaderToken>& tokens,
    size_t start_token, size_t end_token)
{
    if (start_token + 1 == end_token) return {};
    size_t first_arg_token = start_token + 1;
    size_t comma_or_end    = start_token + 1;
    std::vector<std::pair<ShaderToken, ShaderToken>> pairs;
    while (true)
    {
        if (first_arg_token == end_token)
        {
            pairs.push_back({});
            return pairs;
        }
        while (tokens[comma_or_end].text != "," && comma_or_end < end_token) ++comma_or_end;

        if (comma_or_end == first_arg_token) { pairs.push_back({}); }
        else if (comma_or_end == first_arg_token + 1) { pairs.push_back({}); }
        else if (comma_or_end == first_arg_token + 2)
        {
            pairs.push_back({ ShaderToken{}, tokens[first_arg_token] });
        }
        else
        {
            pairs.push_back({ tokens[first_arg_token], tokens[first_arg_token + 1] });
        }

        if (comma_or_end >= end_token) return pairs;
        else
        {
            first_arg_token = comma_or_end + 1;
            comma_or_end    = first_arg_token;
        }
    }
}

void ShaderParser::processShaders(std::string& source)
{
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
    {
        std::string current_token;
        size_t token_start = 0;
        for (size_t i = 0; i < source.size(); ++i)
        {
            char next = source[i];
            if (current_token.empty())
            {
                current_token.push_back(next);
                token_start = i;
                continue;
            }
            int type = getType(current_token[current_token.size() - 1]);
            if (type != getType(next))
            {
                if (type != -2) tokens.emplace_back(current_token, token_start, type);
                current_token = "";
                token_start   = i;
            }
            current_token.push_back(next);
        }
        if (!current_token.empty())
        {
            int type = getType(current_token[current_token.size() - 1]);
            if (type != -2) tokens.emplace_back(current_token, token_start, type);
        }
    }

    size_t vertex_shader_locations[4]   = { SIZE_MAX };
    size_t fragment_shader_locations[4] = { SIZE_MAX };
    std::vector<std::string> structs;
    size_t token_index = 0;
    for (auto it = tokens.begin(); it != tokens.end(); ++it, ++token_index)
    {
        if (it->text == "struct")
        {
            if ((it + 1) == tokens.end())
            {
                DBG_ERROR("error parsing shader " + file_path + ": trunctated struct definition");
                success = false;
                return;
            }
            if ((it + 1)->type == -1) structs.push_back((it + 1)->text);
        }
        else if (it->text == "vertex")
        {
            if (!extractFunction(vertex_shader_locations, it, token_index, tokens.begin(), tokens.end(),
                    "vertex", "void"))
                return;
        }
        else if (it->text == "fragment")
        {
            if (!extractFunction(fragment_shader_locations, it, token_index, tokens.begin(), tokens.end(),
                    "fragment", "bool"))
                return;
        }
    }

    std::vector<std::string> extra_vertex_varyings;
    std::vector<std::string> extra_fragment_varyings;
    if (vertex_shader_locations[0] == SIZE_MAX)
    {
        vertex_shader_locations[0] = source.size();
        source.append(vertex_function_def[0]);
        source.append(vertex_function_def[1]);
        vertex_shader_locations[1] = source.size() - 1;
    }
    else
    {
        auto result = getArguments(tokens, vertex_shader_locations[2], vertex_shader_locations[3]);
        if (result.size() < 3 || result[0].first.text != "in" || result[0].second.text != "Vertex" ||
            result[1].first.text != "inout" || result[1].second.text != "vec4" ||
            result[2].first.text != "inout" || result[2].second.text != "Varyings")
        {
            DBG_ERROR("error parsing shader " + file_path +
                      ": invalid vertex shader definition, the minimum definition should match: " +
                      vertex_function_def[0]);
            success = false;
            return;
        }
        for (size_t i = 3; i < result.size(); ++i)
        {
            if (result[i].first.text != "out")
            {
                DBG_ERROR(
                    "error parsing shader " + file_path +
                    ": invalid vertex shader definition, additional varyings must use the 'out' keyword");
                success = false;
                return;
            }
            if (std::find(structs.begin(), structs.end(), result[i].second.text) == structs.end())
            {
                DBG_ERROR(
                    "error parsing shader " + file_path +
                    ": invalid vertex shader definition, additional varyings must use a valid struct type");
                success = false;
                return;
            }
            extra_vertex_varyings.push_back(result[i].second.text);
        }
    }

    if (fragment_shader_locations[0] == SIZE_MAX)
    {
        fragment_shader_locations[0] = source.size();
        source.append(fragment_function_def[0]);
        source.append(fragment_function_def[1]);
        fragment_shader_locations[1] = source.size() - 1;
    }
    else
    {
        auto result = getArguments(tokens, fragment_shader_locations[2], fragment_shader_locations[3]);
        size_t result_last = result.size() - 1;
        if (result.size() < 2 || result[0].first.text != "in" || result[0].second.text != "Varyings" ||
            result[result_last].first.text != "inout" || result[result_last].second.text != "Fragment")
        {
            DBG_ERROR("error parsing shader " + file_path +
                      ": invalid fragment shader definition, the minimum definition should match: " +
                      fragment_function_def[0]);
            success = false;
            return;
        }
        for (size_t i = 1; i < result_last; ++i)
        {
            if (result[i].first.text != "in")
            {
                DBG_ERROR(
                    "error parsing shader " + file_path +
                    ": invalid fragment shader definition, additional varyings must use the 'in' keyword");
                success = false;
                return;
            }
            if (std::find(structs.begin(), structs.end(), result[i].second.text) == structs.end())
            {
                DBG_ERROR(
                    "error parsing shader " + file_path +
                    ": invalid fragment shader definition, additional varyings must use a valid struct type");
                success = false;
                return;
            }
            extra_fragment_varyings.push_back(result[i].second.text);
        }
    }

    constexpr size_t varyings_struct_components = 5;

    {
        vertex_shader = source;
        vertex_shader.erase(fragment_shader_locations[0],
            (fragment_shader_locations[1] - fragment_shader_locations[0]) + 1);
        vertex_shader.append(vertex_main_str[0]);
        for (size_t i = 0; i < extra_vertex_varyings.size(); ++i)
            vertex_shader.append(std::format("\nlayout(location = {2}) out {1} _HEI_varyings_{1}_{0};",
                i + 1, extra_vertex_varyings[i], i + varyings_struct_components));
        vertex_shader.append(vertex_main_str[1]);
        for (size_t i = 0; i < extra_vertex_varyings.size(); ++i)
            vertex_shader.append(
                std::format("\n    {1} _internal_{1}_{0};", i + 1, extra_vertex_varyings[i]));
        if (!canvas_transform && !no_transform) vertex_shader.append(vertex_main_str[2]);
        else if (canvas_transform)
            vertex_shader.append(vertex_main_str_2_canvas);
        vertex_shader.append(vertex_main_str[3]);
        for (size_t i = 0; i < extra_vertex_varyings.size(); ++i)
            vertex_shader.append(std::format(", _internal_{1}_{0}", i + 1, extra_vertex_varyings[i]));
        vertex_shader.append(vertex_main_str[4]);
        for (size_t i = 0; i < extra_vertex_varyings.size(); ++i)
            vertex_shader.append(std::format("\n    _HEI_varyings_{1}_{0} = _internal_{1}_{0};", i + 1,
                extra_vertex_varyings[i]));
        vertex_shader.append(vertex_main_str[5]);
    }

    {
        fragment_shader = source;
        fragment_shader.erase(vertex_shader_locations[0],
            (vertex_shader_locations[1] - vertex_shader_locations[0]) + 1);
        fragment_shader.append(fragment_main_str[0]);
        for (size_t i = 0; i < extra_fragment_varyings.size(); ++i)
            fragment_shader.append(std::format("\nlayout(location = {2}) in {1} _HEI_varyings_{1}_{0};",
                i + 1, extra_fragment_varyings[i], i + varyings_struct_components));
        fragment_shader.append(fragment_main_str[1]);
        if (!canvas_attachments) fragment_shader.append(fragment_main_str[2]);
        fragment_shader.append(fragment_main_str[3]);
        for (size_t i = 0; i < extra_fragment_varyings.size(); ++i)
            fragment_shader.append(
                std::format(", _HEI_varyings_{1}_{0}", i + 1, extra_fragment_varyings[i]));
        fragment_shader.append(fragment_main_str[4]);
        if (!canvas_attachments) fragment_shader.append(fragment_main_str[5]);
        fragment_shader.append(fragment_main_str[6]);
    }
}

bool ShaderParser::extractFunction(size_t* shader_locations, std::vector<ShaderToken>::iterator& it,
    size_t& token_index, const std::vector<ShaderToken>::iterator begin,
    const std::vector<ShaderToken>::iterator end, const std::string& shader, const std::string& ret_type)
{
    if (it == begin) return true;
    if ((it - 1)->text != ret_type) return true;
    shader_locations[0] = (it - 1)->start;
    if ((it + 1) == end)
    {
        DBG_ERROR("error parsing shader " + file_path + ": trunctated " + shader + " shader definition");
        success = false;
        return false;
    }
    if ((it + 1)->text != "(")
    {
        DBG_ERROR("error parsing shader " + file_path + ": invalid " + shader +
                  " shader definition, expected '('");
        success = false;
        return false;
    }
    shader_locations[2] = token_index + 1;
    int brackets        = 0;
    for (++it, ++token_index; it != end; ++it, ++token_index)
    {
        if (it->text == ")") --brackets;
        else if (it->text == "(")
            ++brackets;
        if (brackets == 0) break;
    }
    if (it == end)
    {
        DBG_ERROR("error parsing shader " + file_path + ": invalid " + shader +
                  " shader definition, expected ')'");
        success = false;
        return false;
    }
    shader_locations[3] = token_index;
    if ((it + 1) == end || (it + 1)->text != "{")
    {
        DBG_ERROR("error parsing shader " + file_path + ": invalid " + shader +
                  " shader definition, expected '{'");
        success = false;
        return false;
    }
    brackets = 0;
    for (++it, ++token_index; it != end; ++it, ++token_index)
    {
        if (it->text == "}") --brackets;
        else if (it->text == "{")
            ++brackets;
        if (brackets == 0) break;
    }
    if (it == end)
    {
        DBG_ERROR("error parsing shader " + file_path + ": invalid " + shader +
                  " shader definition, expected '}'");
        success = false;
        return false;
    }
    shader_locations[1] = it->start;
    return true;
}

bool Shader::compileShaders(const std::string& path, std::vector<uint32_t>& vert_blob,
    std::vector<uint32_t>& frag_blob)
{
    auto shader_data = Package::load(path);

    if (shader_data.empty()) return false;

    std::string shader_text;
    shader_text.resize(shader_data.size());
    memcpy(shader_text.data(), shader_data.data(), shader_data.size());

    ShaderParser parser(shader_text, path);
    if (parser.getVertexShader().empty() || parser.getFragmentShader().empty()) return false;

    if (!compile(parser.getVertexShader(), STAGE_VERTEX, vert_blob, path)) return false;
    if (!compile(parser.getFragmentShader(), STAGE_FRAGMENT, frag_blob, path)) return false;

    return true;
}
