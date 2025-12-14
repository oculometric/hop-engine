#include "material.h"
#include "texture.h"
#include "shader.h"
#include "token_file.h"
#include "package.h"
#include "sampler.h"

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

bool getAnonArgument(int index, string& result, TokenReader::TokenType type, const vector<pair<string, TokenReader::Token>>& args)
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

bool getAnonArgument(int index, glm::vec4 result, const vector<pair<string, TokenReader::Token>>& args)
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

bool getAnonArgument(int index, float result, const vector<pair<string, TokenReader::Token>>& args)
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

	static map<string, VkBool32> bool_map =
	{
		{ "TRUE", VK_TRUE },
		{ "FALSE", VK_FALSE }
	};

	static map<string, VkCullModeFlags> cull_map =
	{
		{ "NONE", VK_CULL_MODE_NONE },
		{ "FRONT", VK_CULL_MODE_FRONT_BIT },
		{ "BACK", VK_CULL_MODE_BACK_BIT }
	};

	static map<string, VkPolygonMode> polygon_map =
	{
		{ "FILL", VK_POLYGON_MODE_FILL },
		{ "LINE", VK_POLYGON_MODE_LINE },
		{ "POINT", VK_POLYGON_MODE_POINT }
	};

	static map<string, VkFilter> filter_map =
	{
		{ "LINEAR", VK_FILTER_LINEAR },
		{ "NEAREST", VK_FILTER_NEAREST },
	};

	static map<string, VkSamplerAddressMode> address_map =
	{
		{ "REPEAT", VK_SAMPLER_ADDRESS_MODE_REPEAT },
		{ "MIRROR", VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT },
		{ "CLAMP", VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE }
	};

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
				shaders[statement.identifier] = new Shader(args[1].s_value, false);
			else if (args[0].s_value == "texture")
				textures[statement.identifier] = new Texture(args[1].s_value);
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
				if (op_map.contains(it->second.s_value))
					operation = op_map[it->second.s_value];
				else
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid depth operation value");
					return nullptr;
				}
			}
			it = args.find("test");
			if (it != args.end())
			{
				if (bool_map.contains(it->second.s_value))
					test = bool_map[it->second.s_value];
				else
				{
					DBG_ERROR("error deserialising material '" + name + "': invalid depth test value");
					return nullptr;
				}
			}
			it = args.find("write");
			if (it != args.end())
			{
				if (bool_map.contains(it->second.s_value))
					write = bool_map[it->second.s_value];
				else
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
				if (cull_map.contains(it->second.s_value))
					cull = cull_map[it->second.s_value];
				else
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
				if (polygon_map.contains(it->second.s_value))
					polygon = polygon_map[it->second.s_value];
				else
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
	Ref<Material> material = new Material(main_shader, cull, polygon, write, test, operation);
	if (!material)
		return nullptr;

	for (const auto args : texture_bindings)
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
			auto filter_it = filter_map.find(it->second.s_value);
			if (filter_it == filter_map.end())
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid texture descriptor filter value");
				return nullptr;
			}
			else
				filter = filter_it->second;
		}
		VkSamplerAddressMode address = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		it = args.find("address");
		if (it != args.end())
		{
			auto address_it = address_map.find(it->second.s_value);
			if (address_it == address_map.end())
			{
				DBG_ERROR("error deserialising material '" + name + "': invalid texture descriptor address value");
				return nullptr;
			}
			else
				address = address_it->second;
		}
		material->setTexture(binding, texture_it->second);
		if (address != VK_SAMPLER_ADDRESS_MODE_REPEAT || filter != VK_FILTER_LINEAR)
			material->setSampler(binding, new Sampler(filter, address));
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

	return material;
}
