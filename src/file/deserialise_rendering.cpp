#include "material.h"
#include "texture.h"
#include "deserialise.h"
#include "package.h"
#include "render_graph.h"
#include "engine.h"

using namespace HopEngine;
using namespace std;

static bool getCompareOp(const string& str, Pipeline::CompareOp& out)
{
	static map<string, Pipeline::CompareOp> op_map =
	{
		{ "ALWAYS", Pipeline::COMPARE_ALWAYS },
		{ "EQUAL", Pipeline::COMPARE_EQUAL },
		{ "GREATER", Pipeline::COMPARE_GREATER },
		{ "GREATER_EQUAL", Pipeline::COMPARE_GREATER_OR_EQUAL },
		{ "LESS", Pipeline::COMPARE_LESS },
		{ "LESS_EQUAL", Pipeline::COMPARE_LESS_OR_EQUAL },
		{ "NEVER", Pipeline::COMPARE_NEVER },
		{ "NOT_EQUAL", Pipeline::COMPARE_NOT_EQUAL }
	};
	const auto it = op_map.find(str);
	if (it == op_map.end())
		return false;
    out = it->second;
	return true;
}

static bool getCullMode(const string& str, Pipeline::CullMode& out)
{
	static map<string, Pipeline::CullMode> cull_map =
	{
		{ "NONE", Pipeline::CULL_NONE },
		{ "FRONT", Pipeline::CULL_FRONT },
		{ "BACK", Pipeline::CULL_BACK }
	};
	const auto it = cull_map.find(str);
	if (it == cull_map.end())
		return false;
    out = it->second;
	return true;
}

static bool getPolygonMode(const string& str, Pipeline::PolygonMode& out)
{
	static map<string, Pipeline::PolygonMode> polygon_map =
	{
		{ "FILL", Pipeline::POLYGON_FILL },
		{ "LINE", Pipeline::POLYGON_LINE },
		{ "POINT", Pipeline::POLYGON_POINT }
	};
	const auto it = polygon_map.find(str);
	if (it == polygon_map.end())
		return false;
    out = it->second;
	return true;
}

static bool getFilter(const string& str, Sampler::Filter& out)
{
	static map<string, Sampler::Filter> filter_map =
	{
		{ "LINEAR", Sampler::FILTER_LINEAR },
		{ "NEAREST", Sampler::FILTER_NEAREST },
	};
	const auto it = filter_map.find(str);
	if (it == filter_map.end())
		return false;
    out = it->second;
	return true;
}

static bool getAddressMode(const string& str, Sampler::Address& out)
{
	static map<string, Sampler::Address> address_map =
	{
		{ "REPEAT", Sampler::ADDRESS_REPEAT },
		{ "MIRROR", Sampler::ADDRESS_MIRRORED },
		{ "CLAMP", Sampler::ADDRESS_CLAMP_EDGE }
	};
	const auto it = address_map.find(str);
	if (it == address_map.end())
		return false;
    out = it->second;
	return true;
}

Ref<Material> Material::deserialise(const string& name)
{
	auto raw_data = Package::load(name);
	if (raw_data.empty())
		return nullptr;

	std::string token_str(reinterpret_cast<char*>(raw_data.data()), raw_data.size());
	auto tokens = TokenReader::tokenise(token_str);
	if (tokens.empty())
		return nullptr;

	auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
	if (syntax_tree.empty())
		return nullptr;

	map<string, Ref<Shader>> shaders;
	map<string, Ref<Texture>> textures;

	Pipeline::Builder pipeline_builder;
	Ref<Shader> main_shader;

	vector<TokenReader::Statement> uniforms;
	vector<tuple<Ref<Texture>, string, Sampler::Filter, Sampler::Address>> texture_bindings;

    Deserialiser deserialiser("error deserialising material '" + name + "'");
    deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("Resource", Deserialiser::STATEMENT_IDENTIFIER_REQUIRED, false)
            .argument(TokenReader::TEXT)
            .argument(TokenReader::STRING),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
    {
        string res_type;
        result.read(0, res_type);
        string res_addr;
        result.read(1, res_addr);
        if (res_type == "shader")
            shaders[result.statement.identifier] = Engine::loadShader(res_addr);
        else if (res_type == "texture")
            textures[result.statement.identifier] = Engine::loadTexture(res_addr);
        else
            return deserialiser.emitError("invalid resource type");
        return true;
    });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Depth", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("operation", TokenReader::TEXT, false)
            .argument("test",      TokenReader::TEXT, false)
            .argument("write",     TokenReader::TEXT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        if (!result.read<Pipeline::CompareOp>("operation", pipeline_builder.depth_compare_op, getCompareOp))
            return deserialiser.emitError("invalid depth operation value");
        if (!result.read("test", pipeline_builder.depth_test_enable))
            return deserialiser.emitError("invalid depth test value");
        if (!result.read("write", pipeline_builder.depth_write_enable))
            return deserialiser.emitError("invalid depth write value");
        return true;
    });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Culling", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("mode", TokenReader::TEXT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        if (!result.read<Pipeline::CullMode>("mode", pipeline_builder.culling_mode, getCullMode))
            return deserialiser.emitError("invalid culling mode value");
        return true;
    });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Polygon", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("mode", TokenReader::TEXT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        if (!result.read<Pipeline::PolygonMode>("mode", pipeline_builder.polygon_mode, getPolygonMode))
            return deserialiser.emitError("invalid polygon mode value");
        return true;
    });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Stencil", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("compare", TokenReader::TEXT, true)
            .argument("compare_value", TokenReader::INT, true)
            .argument("compare_mask", TokenReader::INT, true)
            .argument("write_mask", TokenReader::INT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        pipeline_builder.stencil_enable = true;
        if (!result.read<Pipeline::CompareOp>("compare", pipeline_builder.stencil_compare_op, getCompareOp))
            return deserialiser.emitError("invalid stencil operation value");
        result.read("compare_value", pipeline_builder.stencil_compare_value);
        result.read("compare_mask", pipeline_builder.stencil_compare_mask);
        result.read("write_mask", pipeline_builder.stencil_write);
        return true;
    });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Shader", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("resource", TokenReader::IDENTIFIER, true),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        string shader_res;
        result.read("resource", shader_res);
        auto shader_it = shaders.find(shader_res);
        if (shader_it == shaders.end())
            return deserialiser.emitError("invalid shader statement, no such resource loaded");
        main_shader = shader_it->second;
        return true;
    });
    deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("Uniform", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, true),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
    {
        for (const TokenReader::Statement& uniform : result.statement.children)
            uniforms.emplace_back(uniform);
        return true;
    });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Texture", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("resource", TokenReader::IDENTIFIER, true)
            .argument("binding", TokenReader::STRING, true)
            .argument("filter", TokenReader::TEXT, false)
            .argument("address", TokenReader::TEXT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        string texture_res;
        result.read("resource", texture_res);
        auto texture_it = textures.find(texture_res);
        if (texture_it == textures.end())
            return deserialiser.emitError("invalid texture statement, no such resource loaded");
        string binding;
        result.read("binding", binding);
        Sampler::Filter filter = Sampler::FILTER_NEAREST;
        if (!result.read<Sampler::Filter>("filter", filter, getFilter))
            return deserialiser.emitError("invalid texture filter value");
        Sampler::Address address = Sampler::ADDRESS_REPEAT;
        if (!result.read<Sampler::Address>("address", address, getAddressMode))
            return deserialiser.emitError("invalid texture address value");
        texture_bindings.emplace_back(texture_it->second, binding, filter, address);
        return true;
    });

    if (!deserialiser.execute(syntax_tree))
        return nullptr;

    if (!main_shader)
		return nullptr;
	Ref<Material> material = new Material(main_shader, pipeline_builder);
	if (!material)
		return nullptr;

    for (const auto& binding : texture_bindings)
    {
        material->setTexture(std::get<string>(binding), std::get<Ref<Texture>>(binding));
		material->setSampler(std::get<string>(binding),
            Engine::makeSampler({ std::get<Sampler::Filter>(binding), std::get<Sampler::Address>(binding) })
        );
    }

    Deserialiser uniform_deserialiser("error deserialising material '" + name + "'");
    uniform_deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("vec4", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument(TokenReader::STRING)
            .argument(TokenReader::VECTOR),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
    {
        string binding;  result.read(0, binding);
        glm::vec4 value; result.read(1, value);
        material->setVec4Uniform(binding, value);
        return true;
    });
    uniform_deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("float", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument(TokenReader::STRING)
            .argument(TokenReader::FLOAT),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
    {
        string binding; result.read(0, binding);
        float value;    result.read(1, value);
        material->setFloatUniform(binding, value);
        return true;
    });
    uniform_deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("float", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument(TokenReader::STRING)
            .argument(TokenReader::FLOAT),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
    {
        string binding; result.read(0, binding);
        float value;    result.read(1, value);
        material->setFloatUniform(binding, value);
        return true;
    });

    uniform_deserialiser.execute(uniforms);

    material->origin = name;
	return material;
}

Ref<RenderGraph> RenderGraph::deserialise(const string& name)
{
	auto raw_data = Package::load(name);
	if (raw_data.empty())
		return nullptr;

	std::string token_str(reinterpret_cast<char*>(raw_data.data()), raw_data.size());
	auto tokens = TokenReader::tokenise(token_str);
	if (tokens.empty())
		return nullptr;

	auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
	if (syntax_tree.empty())
		return nullptr;

	map<string, Ref<Shader>> shaders;
	map<string, RenderPass::Config> render_passes;
	map<string, int> step_identifiers;
	Builder builder;

    Deserialiser deserialiser("error deserialising render graph '" + name + "'");
    deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("Resource", Deserialiser::STATEMENT_IDENTIFIER_REQUIRED, false)
            .argument(TokenReader::TEXT)
            .argument(TokenReader::STRING),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
    {
        string res_type;
        result.read(0, res_type);
        string res_addr;
        result.read(1, res_addr);
        if (res_type == "shader")
            shaders[result.statement.identifier] = Engine::loadShader(res_addr);
        else
            return deserialiser.emitError("invalid resource type");
        return true;
    });
    deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("RenderPass", Deserialiser::STATEMENT_IDENTIFIER_REQUIRED, false)
            .argument(TokenReader::TEXT)
            .argument(TokenReader::INT),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
    {
        bool depth_enabled;
        if (!result.read(0, depth_enabled))
            return deserialiser.emitError("invalid 'depth enabled' value for render pass descriptor");
        uint32_t extra_buffers;
        result.read(1, extra_buffers);
        render_passes[result.statement.identifier] = RenderPass::Config{ extra_buffers, depth_enabled };
        return true;
    });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Camera", Deserialiser::STATEMENT_IDENTIFIER_REQUIRED, false)
            .argument("slot", TokenReader::INT, true)
            .argument("scale", TokenReader::FLOAT, false)
            .argument("custom_size", TokenReader::VECTOR, false)
            .argument("render_pass", TokenReader::IDENTIFIER, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        int slot;
        result.read("slot", slot);
        if (slot < 0)
            return deserialiser.emitError("camera slot must be greater than 0");
        float scale = 1.0f;
        result.read("scale", scale);
        glm::vec2 custom_size{ -1, -1 };
        result.read("custom_size", custom_size);
        if (custom_size != glm::vec2{ -1, -1 })
        {
            scale = 0.0f;
            custom_size = glm::max(custom_size, 1.0f);
        }
        string render_pass_id;
        result.read("render_pass", render_pass_id);
        if (render_pass_id.empty())
            builder.addCamera(slot, scale, custom_size);
        else
        {
            auto render_pass_it = render_passes.find(render_pass_id);
            if (render_pass_it == render_passes.end())
                return deserialiser.emitError("unknown render pass identifier '" + render_pass_id + "'");
            builder.addCamera(slot, render_pass_it->second, scale, custom_size);
        }
        builder.execution_steps[builder.execution_steps.size() - 1].name = result.statement.identifier;
        step_identifiers[result.statement.identifier] = static_cast<int>(builder.execution_steps.size()) - 1;
        return true;
    });

    Deserialiser binding_deserialiser("error deserialising render graph '" + name + "'");
    map<uint32_t, AttachmentBinding> bindings;
    binding_deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Input", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("binding", TokenReader::INT, true)
            .argument("step", TokenReader::IDENTIFIER, true)
            .argument("attachment", TokenReader::INT, true)
            .argument("filter", TokenReader::TEXT, false)
            .argument("address", TokenReader::TEXT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        int binding; result.read("binding", binding);
        if (binding < 0)
            return binding_deserialiser.emitError("post-process input binding must be positive");
        string step_id; result.read("step", step_id);
        auto step_it = step_identifiers.find(step_id);
        if (step_it == step_identifiers.end())
            return binding_deserialiser.emitError("nonexistent step identifier '" + step_id + "'");
        int attachment; result.read("attachment", attachment);
        if (attachment < 0)
            return binding_deserialiser.emitError("post-process target attachment reference must be positive");
        AttachmentBinding texture_binding(step_it->second, static_cast<uint32_t>(attachment));
        if (!result.read<Sampler::Filter>("filter", texture_binding.filter_mode, getFilter))
            return binding_deserialiser.emitError("invalid post-process input filter value");
        if (!result.read<Sampler::Address>("address", texture_binding.address_mode, getAddressMode))
            return binding_deserialiser.emitError("invalid post-process input address value");
        bindings[static_cast<uint32_t>(binding)] = texture_binding;
        return true;
    });

    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("PostProcess", Deserialiser::STATEMENT_IDENTIFIER_REQUIRED, true)
            .argument("shader", TokenReader::IDENTIFIER, true)
            .argument("scale", TokenReader::FLOAT, false)
            .argument("custom_size", TokenReader::VECTOR, false)
            .argument("render_pass", TokenReader::IDENTIFIER, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        string shader_res;
        result.read("shader", shader_res);
        auto shader_it = shaders.find(shader_res);
        if (shader_it == shaders.end())
            return deserialiser.emitError("unknown shader identifier '" + shader_res + "'");
        float scale = 1.0f;
        result.read("scale", scale);
        glm::vec2 custom_size{ -1, -1 };
        result.read("custom_size", custom_size);
        if (custom_size != glm::vec2{ -1, -1 })
        {
            scale = 0.0f;
            custom_size = glm::max(custom_size, 1.0f);
        }
        bindings.clear();
        if (!binding_deserialiser.execute(result.statement.children))
            return false;
        string render_pass_id;
        result.read("render_pass", render_pass_id);
        if (render_pass_id.empty())
            builder.addPostProcess(shader_it->second, bindings, scale, custom_size);
        else
        {
            auto render_pass_it = render_passes.find(render_pass_id);
            if (render_pass_it == render_passes.end())
                return deserialiser.emitError("unknown render pass identifier '" + render_pass_id + "'");
            builder.addPostProcess(shader_it->second, bindings, render_pass_it->second, scale, custom_size);
        }
        builder.execution_steps[builder.execution_steps.size() - 1].name = result.statement.identifier;
        step_identifiers[result.statement.identifier] = static_cast<int>(builder.execution_steps.size()) - 1;
        return true;
    });

    if (!deserialiser.execute(syntax_tree))
        return nullptr;
	
	auto rg = new RenderGraph(builder);
	rg->origin = name;
	return rg;
}