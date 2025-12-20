#include <filesystem>

#include "material.h"
#include "texture.h"
#include "shader.h"
#include "token_file.h"
#include "package.h"
#include "sampler.h"
#include "render_graph.h"
#include "engine.h"

using namespace HopEngine;
using namespace std;

int getArgument(string name, string& result, TokenReader::TokenType type, const vector<pair<string, TokenReader::Token>>& args)
{
	for (const auto& arg : args)
	{
		if (arg.first != name)
			continue;
		if (arg.second.type != type)
			return 2;
		result = arg.second.s_value;
		return 0;
	}
	return 1;
}

bool getAnonArgument(size_t index, string& result, TokenReader::TokenType type, const vector<pair<string, TokenReader::Token>>& args)
{
	if (index >= args.size())
		return false;
	if (args[index].first != "")
		return false;
	if (args[index].second.type != type)
		return false;
	result = args[index].second.s_value;
	return true;
}

bool getAnonArgument(size_t index, glm::vec4& result, const vector<pair<string, TokenReader::Token>>& args)
{
	if (index >= args.size())
		return false;
	if (args[index].first != "")
		return false;
	if (args[index].second.type != TokenReader::VECTOR)
		return false;
	result = args[index].second.c_value;
	return true;
}

bool getAnonArgument(size_t index, float& result, const vector<pair<string, TokenReader::Token>>& args)
{
	if (index >= args.size())
		return false;
	if (args[index].first != "")
		return false;
	if (args[index].second.type == TokenReader::FLOAT)
	{
		result = args[index].second.f_value;
		return true;
	}
	else if (args[index].second.type == TokenReader::INT)
	{
		result = (float)args[index].second.i_value;
		return true;
	}
	return false;
}

static VkCompareOp getCompareOp(string str)
{
	static map<string, VkCompareOp> op_map =
	{
		{ "ALWAYS", VK_COMPARE_OP_ALWAYS },
		{ "EQUAL", VK_COMPARE_OP_EQUAL },
		{ "GREATER", VK_COMPARE_OP_GREATER },
		{ "GREATER_EQUAL", VK_COMPARE_OP_GREATER_OR_EQUAL },
		{ "LESS", VK_COMPARE_OP_LESS },
		{ "LESS_EQUAL", VK_COMPARE_OP_LESS_OR_EQUAL },
		{ "NEVER", VK_COMPARE_OP_NEVER },
		{ "NOT_EQUAL", VK_COMPARE_OP_NOT_EQUAL }
	};
	auto it = op_map.find(str);
	if (it == op_map.end())
		return VK_COMPARE_OP_MAX_ENUM;
	return it->second;
}

static VkBool32 getBool(string str)
{
	static map<string, VkBool32> bool_map =
	{
		{ "TRUE", VK_TRUE },
		{ "FALSE", VK_FALSE }
	};
	auto it = bool_map.find(str);
	if (it == bool_map.end())
		return -1;
	return it->second;
}

static VkCullModeFlags getCullMode(string str)
{
	static map<string, VkCullModeFlags> cull_map =
	{
		{ "NONE", VK_CULL_MODE_NONE },
		{ "FRONT", VK_CULL_MODE_FRONT_BIT },
		{ "BACK", VK_CULL_MODE_BACK_BIT }
	};
	auto it = cull_map.find(str);
	if (it == cull_map.end())
		return VK_CULL_MODE_FLAG_BITS_MAX_ENUM;
	return it->second;
}

static VkPolygonMode getPolygonMode(string str)
{
	static map<string, VkPolygonMode> polygon_map =
	{
		{ "FILL", VK_POLYGON_MODE_FILL },
		{ "LINE", VK_POLYGON_MODE_LINE },
		{ "POINT", VK_POLYGON_MODE_POINT }
	};
	auto it = polygon_map.find(str);
	if (it == polygon_map.end())
		return VK_POLYGON_MODE_MAX_ENUM;
	return it->second;
}

static VkFilter getFilter(string str)
{
	static map<string, VkFilter> filter_map =
	{
		{ "LINEAR", VK_FILTER_LINEAR },
		{ "NEAREST", VK_FILTER_NEAREST },
	};
	auto it = filter_map.find(str);
	if (it == filter_map.end())
		return VK_FILTER_MAX_ENUM;
	return it->second;
}

static VkSamplerAddressMode getAddressMode(string str)
{
	static map<string, VkSamplerAddressMode> address_map =
	{
		{ "REPEAT", VK_SAMPLER_ADDRESS_MODE_REPEAT },
		{ "MIRROR", VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
		{ "CLAMP", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE }
	};
	auto it = address_map.find(str);
	if (it == address_map.end())
		return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
	return it->second;
}

Ref<Material> Material::deserialise(string name)
{
	auto raw_data = Package::tryLoadFile(name);
	if (raw_data.empty())
		return nullptr;

	std::string token_str((char*)raw_data.data(), raw_data.size());
	auto tokens = TokenReader::tokenise(token_str);
	if (tokens.empty())
		return nullptr;

	auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
	if (syntax_tree.empty())
		return nullptr;

	map<string, Ref<Shader>> shaders;
	map<string, Ref<Texture>> textures;

	VkCompareOp operation = VK_COMPARE_OP_LESS;
	VkBool32 test = VK_TRUE;
	VkBool32 write = VK_TRUE;
	VkCullModeFlags cull = VK_CULL_MODE_BACK_BIT;
	VkPolygonMode polygon = VK_POLYGON_MODE_FILL;
	Ref<Shader> main_shader;

	vector<TokenReader::Statement> uniforms;
	vector<map<string, TokenReader::Token>> texture_bindings;

	for (const TokenReader::Statement& statement : syntax_tree)
	{
		if (statement.keyword == "Resource")
		{
			vector<TokenReader::Token> args;
			if (!TokenReader::readStatement(statement, false, true,
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
			if (!TokenReader::readStatement(statement, false, false,
				{
					{ "operation", { TokenReader::TEXT, false } },
					{ "test", { TokenReader::TEXT, false } },
					{ "write", { TokenReader::TEXT, false } }
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			auto it = args.find("operation");
			if (it != args.end())
			{
				operation = getCompareOp(it->second.s_value);
				if (operation == VK_COMPARE_OP_MAX_ENUM)
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid depth operation value");
					return nullptr;
				}
			}
			it = args.find("test");
			if (it != args.end())
			{
				test = getBool(it->second.s_value);
				if (test == (VkBool32)-1)
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid depth test value");
					return nullptr;
				}
			}
			it = args.find("write");
			if (it != args.end())
			{
				write = getBool(it->second.s_value);
				if (write == (VkBool32)-1)
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid depth write value");
					return nullptr;
				}
			}
		}
		else if (statement.keyword == "Culling")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatement(statement, false, false,
				{
					{ "mode", { TokenReader::TEXT, false } },
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			auto it = args.find("mode");
			if (it != args.end())
			{
				cull = getCullMode(it->second.s_value);
				if (cull == VK_CULL_MODE_FLAG_BITS_MAX_ENUM)
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid culling mode value");
					return nullptr;
				}
			}
		}
		else if (statement.keyword == "Polygon")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatement(statement, false, false,
				{
					{ "mode", { TokenReader::TEXT, false } },
				}, args, "error deserialising material '" + name + "'"))
				return nullptr;
			auto it = args.find("mode");
			if (it != args.end())
			{
				polygon = getPolygonMode(it->second.s_value);
				if (polygon == VK_POLYGON_MODE_MAX_ENUM)
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid polygon mode value");
					return nullptr;
				}
			}
		}
		else if (statement.keyword == "Shader")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatement(statement, false, false,
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
			if (statement.arguments.size() > 0)
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid uniform descritor, too many arguments");
				return nullptr;
			}
			for (const TokenReader::Statement& uniform : statement.children)
			{
				uniforms.push_back(uniform);
			}
		}
		else if (statement.keyword == "Texture")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatement(statement, false, false,
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
			DBG_ERROR("error deserialising material '" + name + "': invalid keyword '" + statement.keyword + "'");;
			return nullptr;
		}
	}

	if (!main_shader)
		return nullptr;
	Ref<Material> material = new Material(main_shader, PipelineBuilder().cullMode(cull).polygonMode(polygon).depthWrite(write).depthTest(test).depthOp(operation));
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
		VkFilter filter = VK_FILTER_LINEAR;
		it = args.find("filter");
		if (it != args.end())
		{
			filter = getFilter(it->second.s_value);
			if (filter == VK_FILTER_MAX_ENUM)
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid texture descriptor filter value");
				return nullptr;
			}
		}
		VkSamplerAddressMode address = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		it = args.find("address");
		if (it != args.end())
		{
			address = getAddressMode(it->second.s_value);
			if (address == VK_SAMPLER_ADDRESS_MODE_MAX_ENUM)
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid texture descriptor address value");
				return nullptr;
			}
		}
		material->setTexture(binding, texture_it->second);
		if (address != VK_SAMPLER_ADDRESS_MODE_REPEAT || filter != VK_FILTER_LINEAR)
			material->setSampler(binding, new Sampler(SamplerBuilder().filter(filter).address(address)));
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

Ref<RenderGraph> RenderGraph::deserialise(string name)
{
	auto raw_data = Package::tryLoadFile(name);
	if (raw_data.empty())
		return nullptr;

	std::string token_str((char*)raw_data.data(), raw_data.size());
	auto tokens = TokenReader::tokenise(token_str);
	if (tokens.empty())
		return nullptr;

	auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
	if (syntax_tree.empty())
		return nullptr;

	map<string, Ref<Shader>> shaders;
	map<string, RenderOutput> render_passes;
	map<string, int> step_identifiers;
	RenderGraphBuilder builder;
	
	for (const TokenReader::Statement& statement : syntax_tree)
	{
		if (statement.keyword == "Resource")
		{
			vector<TokenReader::Token> args;
			if (!TokenReader::readStatement(statement, false, true,
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
			if (!TokenReader::readStatement(statement, false, true,
				{
					TokenReader::TEXT,
					TokenReader::INT
				}, args, "error deserialising render graph '" + name + "'"))
				return nullptr;
			VkBool32 has_depth = getBool(args[0].s_value);
			if (has_depth == (VkBool32)-1)
			{
				DBG_ERROR("error deserialising render graph '" + name + "': invalid 'depth enabled' value for render pass descriptor");
				return nullptr;
			}
			size_t extra_buffers = glm::clamp(args[1].i_value, 0, 6);
			render_passes[statement.identifier] = RenderOutput{ extra_buffers, (bool)has_depth };
		}
		else if (statement.keyword == "Camera")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatement(statement, false, true,
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
				builder.addCamera(slot, render_pass_it->second, scale, { (uint32_t)custom_size.x, (uint32_t)custom_size.y });
			}
			else
				builder.addCamera(slot, scale, { (uint32_t)custom_size.x, (uint32_t)custom_size.y });
			step_identifiers[statement.identifier] = (int)builder.execution_steps.size() - 1;
		}
		else if (statement.keyword == "PostProcess")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatement(statement, true, true,
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
			map<uint32_t, RenderTextureBinding> bindings;
			for (const TokenReader::Statement& sub_statement : statement.children)
			{
				if (sub_statement.keyword != "Input")
				{
					DBG_ERROR("error deserialising render graph '" + name + "': only Input statements are allowed inside a PostProcess statement");
					return nullptr;
				}
				map<string, TokenReader::Token> args2;
				if (!TokenReader::readStatement(sub_statement, false, false,
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
				RenderTextureBinding texture_binding(step_it->second, attachment);
				auto filter_it = args2.find("filter");
				if (filter_it != args2.end())
				{
					VkFilter filter = getFilter(filter_it->second.s_value);
					if (filter == VK_FILTER_MAX_ENUM)
					{
						DBG_ERROR("error deserialising render graph '" + name + "': invalid post-process input filter value");
						return nullptr;
					}
					texture_binding.filter(filter);
				}
				filter_it = args2.find("address");
				if (filter_it != args2.end())
				{
					VkSamplerAddressMode address = getAddressMode(filter_it->second.s_value);
					if (address == VK_SAMPLER_ADDRESS_MODE_MAX_ENUM)
					{
						DBG_ERROR("error deserialising render graph '" + name + "': invalid post-process input address mode value");
						return nullptr;
					}
					texture_binding.address(address);
				}
				bindings[(uint32_t)binding] = texture_binding;
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
				builder.addPostProcess(shader_it->second, bindings, render_pass_it->second, scale, { (uint32_t)custom_size.x, (uint32_t)custom_size.y });
			}
			else
				builder.addPostProcess(shader_it->second, bindings, scale, { (uint32_t)custom_size.x, (uint32_t)custom_size.y });
			step_identifiers[statement.identifier] = static_cast<int>(builder.execution_steps.size()) - 1;
		}
		else
		{
			DBG_ERROR("error deserialising render graph '" + name + "': invalid keyword '" + statement.keyword + "'");;
			return nullptr;
		}
	}
	return new RenderGraph(builder);
}
