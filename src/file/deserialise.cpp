#include <filesystem>

#include "material.h"
#include "texture.h"
#include "token_file.h"
#include "package.h"
#include "render_graph.h"
#include "engine.h"
#include "scene.h"
#include "mesh.h"
#include "basic_components.h"

using namespace HopEngine;
using namespace std;

static int getArgument(const string& name, string& result, const TokenReader::TokenType type, const vector<pair<string, TokenReader::Token>>& args)
{
	for (const auto& [arg_name, token] : args)
	{
		if (arg_name != name)
			continue;
		if (token.type != type)
			return 2;
		result = token.s_value;
		return 0;
	}
	return 1;
}

static bool getAnonArgument(const size_t index, string& result, const TokenReader::TokenType type, const vector<pair<string, TokenReader::Token>>& args)
{
	if (index >= args.size())
		return false;
	if (!args[index].first.empty())
		return false;
	if (args[index].second.type != type)
		return false;
	result = args[index].second.s_value;
	return true;
}

static bool getAnonArgument(const size_t index, glm::vec4& result, const vector<pair<string, TokenReader::Token>>& args)
{
	if (index >= args.size())
		return false;
	if (!args[index].first.empty())
		return false;
	if (args[index].second.type != TokenReader::VECTOR)
		return false;
	result = args[index].second.c_value;
	return true;
}

static bool getAnonArgument(const size_t index, float& result, const vector<pair<string, TokenReader::Token>>& args)
{
	if (index >= args.size())
		return false;
	if (!args[index].first.empty())
		return false;
	if (args[index].second.type == TokenReader::FLOAT)
	{
		result = args[index].second.f_value;
		return true;
	}
	if (args[index].second.type == TokenReader::INT)
	{
		result = static_cast<float>(args[index].second.i_value);
		return true;
	}
	return false;
}

static Pipeline::CompareOp getCompareOp(const string& str)
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
		return static_cast<Pipeline::CompareOp>(-1);
	return it->second;
}

static int getBool(const string& str)
{
	static map<string, bool> bool_map =
	{
		{ "TRUE", true },
		{ "FALSE", false }
	};
	const auto it = bool_map.find(str);
	if (it == bool_map.end())
		return -1;
	return it->second;
}

static Pipeline::CullMode getCullMode(const string& str)
{
	static map<string, Pipeline::CullMode> cull_map =
	{
		{ "NONE", Pipeline::CULL_NONE },
		{ "FRONT", Pipeline::CULL_FRONT },
		{ "BACK", Pipeline::CULL_BACK }
	};
	const auto it = cull_map.find(str);
	if (it == cull_map.end())
		return static_cast<Pipeline::CullMode>(-1);
	return it->second;
}

static Pipeline::PolygonMode getPolygonMode(const string& str)
{
	static map<string, Pipeline::PolygonMode> polygon_map =
	{
		{ "FILL", Pipeline::POLYGON_FILL },
		{ "LINE", Pipeline::POLYGON_LINE },
		{ "POINT", Pipeline::POLYGON_POINT }
	};
	const auto it = polygon_map.find(str);
	if (it == polygon_map.end())
		return static_cast<Pipeline::PolygonMode>(-1);
	return it->second;
}

static Sampler::Filter getFilter(const string& str)
{
	static map<string, Sampler::Filter> filter_map =
	{
		{ "LINEAR", Sampler::FILTER_LINEAR },
		{ "NEAREST", Sampler::FILTER_NEAREST },
	};
	const auto it = filter_map.find(str);
	if (it == filter_map.end())
		return static_cast<Sampler::Filter>(-1);
	return it->second;
}

static Sampler::Address getAddressMode(const string& str)
{
	static map<string, Sampler::Address> address_map =
	{
		{ "REPEAT", Sampler::ADDRESS_REPEAT },
		{ "MIRROR", Sampler::ADDRESS_MIRRORED },
		{ "CLAMP", Sampler::ADDRESS_CLAMP_EDGE }
	};
	const auto it = address_map.find(str);
	if (it == address_map.end())
		return static_cast<Sampler::Address>(-1);
	return it->second;
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
	vector<map<string, TokenReader::Token>> texture_bindings;

	for (const TokenReader::Statement& statement : syntax_tree)
	{
		if (statement.keyword == "Resource")
		{
			vector<TokenReader::Token> args;
			if (!TokenReader::readStatementAnonymous(statement, false, true,
				{
					TokenReader::TEXT,
					TokenReader::STRING
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			if (args[0].s_value == "shader")
				shaders[statement.identifier] = Engine::loadShader(args[1].s_value);
			else if (args[0].s_value == "texture")
				textures[statement.identifier] = Engine::loadTexture(args[1].s_value);
			else
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid resource type");
				return nullptr;
			}
		}
		else if (statement.keyword == "Depth")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, false,
				{
					{ "operation", { TokenReader::TEXT, false } },
					{ "test", { TokenReader::TEXT, false } },
					{ "write", { TokenReader::TEXT, false } }
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			auto it = args.find("operation");
			if (it != args.end())
			{
				Pipeline::CompareOp operation = getCompareOp(it->second.s_value);
				if (operation == static_cast<Pipeline::CompareOp>(-1))
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid depth operation value");
					return nullptr;
				}
				pipeline_builder.depthOp(operation);
			}
			it = args.find("test");
			if (it != args.end())
			{
				int test = getBool(it->second.s_value);
				if (test == -1)
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid depth test value");
					return nullptr;
				}
				pipeline_builder.depthTest(test);
			}
			it = args.find("write");
			if (it != args.end())
			{
				int write = getBool(it->second.s_value);
				if (write == -1)
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid depth write value");
					return nullptr;
				}
				pipeline_builder.depthWrite(write);
			}
		}
		else if (statement.keyword == "Culling")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, false,
				{
					{ "mode", { TokenReader::TEXT, false } },
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			auto it = args.find("mode");
			if (it != args.end())
			{
				Pipeline::CullMode cull = getCullMode(it->second.s_value);
				if (cull == static_cast<Pipeline::CullMode>(-1))
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid culling mode value");
					return nullptr;
				}
				pipeline_builder.cullMode(cull);
			}
		}
		else if (statement.keyword == "Polygon")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, false,
				{
					{ "mode", { TokenReader::TEXT, false } },
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			auto it = args.find("mode");
			if (it != args.end())
			{
				Pipeline::PolygonMode polygon = getPolygonMode(it->second.s_value);
				if (polygon == static_cast<Pipeline::PolygonMode>(-1))
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid polygon mode value");
					return nullptr;
				}
				pipeline_builder.polygonMode(polygon);
			}
		}
		else if (statement.keyword == "Stencil")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, false,
				{
					{ "compare", { TokenReader::TEXT, true } },
					{ "compare_value", { TokenReader::INT, true } },
					{ "compare_mask", { TokenReader::INT, true } },
					{ "write_mask", { TokenReader::INT, false } },
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			Pipeline::CompareOp compare = getCompareOp(args["compare"].s_value);
			if (compare == static_cast<Pipeline::CompareOp>(-1))
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid stencil compare op value");
				return nullptr;
			}
			pipeline_builder.stencilCompare(compare, args["compare_value"].i_value, args["compare_mask"].i_value);
			auto it = args.find("write_mask");
			if (it != args.end())
				pipeline_builder.stencilWrite(it->second.i_value);
		}
		else if (statement.keyword == "Shader")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, false,
				{
					{ "resource", { TokenReader::IDENTIFIER, true } },
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			auto it = args.find("resource");
			if (it != args.end())
			{
				auto shader_it = shaders.find(it->second.s_value);
				if (shader_it == shaders.end())
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid shader descriptor, no such resource loaded");
					return nullptr;
				}
				main_shader = shader_it->second;
			}
		}
		else if (statement.keyword == "Uniform")
		{
			if (!statement.arguments.empty())
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid uniform descritor, arguments are not allowed");
				return nullptr;
			}
			for (const TokenReader::Statement& uniform : statement.children)
				uniforms.push_back(uniform);
		}
		else if (statement.keyword == "Texture")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, false,
				{
					{ "resource", { TokenReader::IDENTIFIER, true } },
					{ "binding", { TokenReader::STRING, true } },
					{ "filter", { TokenReader::TEXT, false } },
					{ "address", { TokenReader::TEXT, false } },
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			texture_bindings.push_back(args);
		}
		else
		{
			DBG_ERROR("error deserialising material '" + name + "': invalid keyword '" + statement.keyword + "'");
			return nullptr;
		}
	}

	if (!main_shader)
		return nullptr;
	Ref<Material> material = new Material(main_shader, pipeline_builder);
	if (!material)
		return nullptr;

	for (const auto& args : texture_bindings)
	{
		auto it = args.find("resource");
		auto texture_it = textures.find(it->second.s_value);
		if (texture_it == textures.end())
		{
			DBG_ERROR("error deserialising material '" + name + "': texture descriptor resource is not loaded");
			return nullptr;
		}
		string binding;
		binding = args.find("binding")->second.s_value;
		Sampler::Builder sampler_builder;
		it = args.find("filter");
		if (it != args.end())
		{
			Sampler::Filter filter = getFilter(it->second.s_value);
			if (filter == static_cast<Sampler::Filter>(-1))
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid texture descriptor filter value");
				return nullptr;
			}
			sampler_builder.filter(filter);
		}
		it = args.find("address");
		if (it != args.end())
		{
			Sampler::Address address = getAddressMode(it->second.s_value);
			if (address == static_cast<Sampler::Address>(-1))
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid texture descriptor address value");
				return nullptr;
			}
			sampler_builder.address(address);
		}
		material->setTexture(binding, texture_it->second);
		material->setSampler(binding, Engine::makeSampler(sampler_builder));
	}

	for (const TokenReader::Statement& statement : uniforms)
	{
		if (statement.keyword == "vec4")
		{
			string binding;
			if (!getAnonArgument(0, binding, TokenReader::STRING, statement.arguments))
			{
				DBG_ERROR("error deserialising material '" + name + "': vec4 statement first argument must be a shader variable name");
				return nullptr;
			}
			glm::vec4 value = { 0, 0, 0, 0 };
			if (!getAnonArgument(1, value, statement.arguments))
			{
				DBG_ERROR("error deserialising material '" + name + "': vec4 statement second argument must be a vector");
				return nullptr;
			}
			material->setVec4Uniform(binding, value);
		}
		else if (statement.keyword == "float")
		{
			string binding;
			if (!getAnonArgument(0, binding, TokenReader::STRING, statement.arguments))
			{
				DBG_ERROR("error deserialising material '" + name + "': float statement first argument must be a shader variable name");
				return nullptr;
			}
			float value = 0;
			if (!getAnonArgument(1, value, statement.arguments))
			{
				DBG_ERROR("error deserialising material '" + name + "': float statement second argument must be a float");
				return nullptr;
			}
			material->setFloatUniform(binding, value);
		}
		else
		{
			DBG_ERROR("error deserialising material '" + name + "': invalid uniform keyword '" + statement.keyword + "'");
			return nullptr;
		}
	}

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
	
	for (const TokenReader::Statement& statement : syntax_tree)
	{
		if (statement.keyword == "Resource")
		{
			vector<TokenReader::Token> args;
			if (!TokenReader::readStatementAnonymous(statement, false, true,
				{
					TokenReader::TEXT,
					TokenReader::STRING
				}, args, "error deserialising render graph '" + name + "'"))
				return nullptr;
			if (args[0].s_value == "shader")
				shaders[statement.identifier] = Engine::loadShader(args[1].s_value);
			else
			{
				DBG_ERROR("error deserialising render graph '" + name + "': invalid resource type");
				return nullptr;
			}
		}
		else if (statement.keyword == "RenderPass")
		{
			vector<TokenReader::Token> args;
			if (!TokenReader::readStatementAnonymous(statement, false, true,
				{
					TokenReader::TEXT,
					TokenReader::INT
				}, args, "error deserialising render graph '" + name + "'"))
				return nullptr;
			int has_depth = getBool(args[0].s_value);
			if (has_depth == -1)
			{
				DBG_ERROR("error deserialising render graph '" + name + "': invalid 'depth enabled' value for render pass descriptor");
				return nullptr;
			}
			size_t extra_buffers = glm::clamp(args[1].i_value, 0, 6);
			render_passes[statement.identifier] = RenderPass::Config{ extra_buffers, static_cast<bool>(has_depth) };
		}
		else if (statement.keyword == "Camera")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, true,
				{
					{ "slot", { TokenReader::INT, true } },
					{ "scale", { TokenReader::FLOAT, false } },
					{ "custom_size", { TokenReader::VECTOR, false } },
					{ "render_pass", { TokenReader::IDENTIFIER, false } },
				}, args, "error deserialising render graph '" + name + "'"))
				return nullptr;
			int slot = args["slot"].i_value;
			if (slot < 0)
			{
				DBG_ERROR("error deserialising render graph '" + name + "': camera slot must be greater than 0");
				return nullptr;
			}
			float scale = 1.0f;
			auto it = args.find("scale");
			if (it != args.end())
				scale = it->second.f_value;
			glm::vec2 custom_size{ 1, 1 };
			it = args.find("custom_size");
			if (it != args.end())
			{
				scale = 0.0f;
				custom_size = glm::max(it->second.c_value, 1.0f);
			}
			it = args.find("render_pass");
			if (it != args.end())
			{
				auto render_pass_it = render_passes.find(it->second.s_value);
				if (render_pass_it == render_passes.end())
				{
					DBG_ERROR("error deserialising render graph '" + name + "': unknown render pass identifier '" + it->second.s_value + "'");
					return nullptr;
				}
				builder.addCamera(slot, render_pass_it->second, scale, { static_cast<uint32_t>(custom_size.x), static_cast<uint32_t>(custom_size.y) });
			}
			else
				builder.addCamera(slot, scale, { static_cast<uint32_t>(custom_size.x), static_cast<uint32_t>(custom_size.y) });
			builder.execution_steps[builder.execution_steps.size() - 1].name = statement.identifier;
			step_identifiers[statement.identifier] = static_cast<int>(builder.execution_steps.size()) - 1;
		}
		else if (statement.keyword == "PostProcess")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, true, true,
				{
					{ "shader", { TokenReader::IDENTIFIER, true } },
					{ "scale", { TokenReader::FLOAT, false } },
					{ "custom_size", { TokenReader::VECTOR, false } },
					{ "render_pass", { TokenReader::IDENTIFIER, false } },
				}, args, "error deserialising render graph '" + name + "'"))
				return nullptr;
			auto shader_it = shaders.find(args["shader"].s_value);
			if (shader_it == shaders.end())
			{
				DBG_ERROR("error deserialising render graph '" + name + "': unknown shader identifier '" + args["shader"].s_value + "'");
				return nullptr;
			}
			float scale = 1.0f;
			auto it = args.find("scale");
			if (it != args.end())
				scale = it->second.f_value;
			glm::vec2 custom_size{ 1, 1 };
			it = args.find("custom_size");
			if (it != args.end())
			{
				scale = 0.0f;
				custom_size = glm::max(it->second.c_value, 1.0f);
			}
			map<uint32_t, AttachmentBinding> bindings;
			for (const TokenReader::Statement& sub_statement : statement.children)
			{
				if (sub_statement.keyword != "Input")
				{
					DBG_ERROR("error deserialising render graph '" + name + "': only Input statements are allowed inside a PostProcess statement");
					return nullptr;
				}
				map<string, TokenReader::Token> args2;
				if (!TokenReader::readStatementNamed(sub_statement, false, false,
					{
						{ "binding", { TokenReader::INT, true } },
						{ "step", { TokenReader::IDENTIFIER, true } },
						{ "attachment", { TokenReader::INT, true } },
						{ "filter", { TokenReader::TEXT, false } },
						{ "address", { TokenReader::TEXT, false } },
					}, args2, "error deserialising render graph '" + name + "'"))
					return nullptr;
				int binding = args2["binding"].i_value;
				if (binding < 0)
				{
					DBG_ERROR("error deserialising render graph '" + name + "': post-process input binding must be positive");
					return nullptr;
				}
				auto step_it = step_identifiers.find(args2["step"].s_value);
				if (step_it == step_identifiers.end())
				{
					DBG_ERROR("error deserialising render graph '" + name + "': nonexistent step identifier '" + args2["step"].s_value + "'");
					return nullptr;
				}
				int attachment = args2["attachment"].i_value;
				if (binding < 0)
				{
					DBG_ERROR("error deserialising render graph '" + name + "': post-process input attachment must be positive");
					return nullptr;
				}
				AttachmentBinding texture_binding(step_it->second, attachment);
				auto filter_it = args2.find("filter");
				if (filter_it != args2.end())
				{
					Sampler::Filter filter = getFilter(filter_it->second.s_value);
					if (filter == static_cast<Sampler::Filter>(-1))
					{
						DBG_ERROR("error deserialising render graph '" + name + "': invalid post-process input filter value");
						return nullptr;
					}
					texture_binding.filter(filter);
				}
				filter_it = args2.find("address");
				if (filter_it != args2.end())
				{
					Sampler::Address address = getAddressMode(filter_it->second.s_value);
					if (address == static_cast<Sampler::Address>(-1))
					{
						DBG_ERROR("error deserialising render graph '" + name + "': invalid post-process input address mode value");
						return nullptr;
					}
					texture_binding.address(address);
				}
				bindings[static_cast<uint32_t>(binding)] = texture_binding;
			}
			it = args.find("render_pass");
			if (it != args.end())
			{
				auto render_pass_it = render_passes.find(it->second.s_value);
				if (render_pass_it == render_passes.end())
				{
					DBG_ERROR("error deserialising render graph '" + name + "': unknown render pass identifier '" + it->second.s_value + "'");
					return nullptr;
				}
				builder.addPostProcess(shader_it->second, bindings, render_pass_it->second, scale, { static_cast<uint32_t>(custom_size.x), static_cast<uint32_t>(custom_size.y) });
			}
			else
				builder.addPostProcess(shader_it->second, bindings, scale, { static_cast<uint32_t>(custom_size.x), static_cast<uint32_t>(custom_size.y) });
			builder.execution_steps[builder.execution_steps.size() - 1].name = statement.identifier;
			step_identifiers[statement.identifier] = static_cast<int>(builder.execution_steps.size()) - 1;
		}
		else
		{
			DBG_ERROR("error deserialising render graph '" + name + "': invalid keyword '" + statement.keyword + "'");
			return nullptr;
		}
	}
	
	auto rg = new RenderGraph(builder);
	rg->origin = name;
	return rg;
}
