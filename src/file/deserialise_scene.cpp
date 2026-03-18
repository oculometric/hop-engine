#include <filesystem>

#include "material.h"
#include "texture.h"
#include "deserialise.h"
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


struct SceneResources
{
	map<string, Ref<Material>> materials;
	map<string, Ref<Mesh>> meshes;
	map<string, Ref<Texture>> textures;
};

static Ref<Object> deserialiseStaticMesh(const map<string, TokenReader::Token>& args, const Ref<Scene>& scene, const SceneResources& resources)
{
	Ref<Object> object = Object::create();
	WeakRef<StaticMeshComponent> obj = object->addComponent<StaticMeshComponent>();
	auto it = args.find("mesh");
	if (it != args.end())
	{
		const auto mesh_it = resources.meshes.find(it->second.s_value);
		if (mesh_it == resources.meshes.end())
		{
			DBG_ERROR("error deserialising static mesh, invalid mesh identifier '" + it->second.s_value + "'");
			return nullptr;
		}
		obj->mesh = mesh_it->second;
	}
	it = args.find("material");
	if (it != args.end())
	{
		const auto material_it = resources.materials.find(it->second.s_value);
		if (material_it == resources.materials.end())
		{
			DBG_ERROR("error deserialising static mesh, invalid material identifier '" + it->second.s_value + "'");
			return nullptr;
		}
		obj->material = material_it->second;
	}
	it = args.find("camera_mask");
	if (it != args.end())
	{
		obj->camera_mask = it->second.i_value;
	}
	
	return object;
}

static Ref<Object> deserialiseLight(const map<string, TokenReader::Token>& args, const Ref<Scene>& scene, const SceneResources& resources)
{
	Ref<Object> object = Object::create();
	WeakRef<LightComponent> obj = object->addComponent<LightComponent>();
	auto it = args.find("type");
	if (it != args.end())
	{
		if (it->second.s_value == "DIRECTIONAL")
			obj->type = LightComponent::DIRECTIONAL;
		else if (it->second.s_value == "POINT")
			obj->type = LightComponent::POINT;
		else if (it->second.s_value == "SPOT")
			obj->type = LightComponent::SPOT;
		else
		{
			DBG_ERROR("error deserialising light, invalid light type '" + it->second.s_value + "'");
			return nullptr;
		}
	}
	it = args.find("colour");
	if (it != args.end())
	{
		obj->colour = it->second.c_value;
	}
	it = args.find("angle");
	if (it != args.end())
	{
		obj->spot_angle = it->second.f_value;
	}
	
	return object;
}

static Ref<Object> deserialiseCamera(const map<string, TokenReader::Token>& args, const Ref<Scene>& scene, const SceneResources& resources)
{
	Ref<Object> object = Object::create();
	WeakRef<CameraComponent> obj = object->addComponent<CameraComponent>();
	auto it = args.find("slot");
	if (it != args.end())
		obj->camera_slot = it->second.i_value;
	else
		DBG_WARNING("deserialising a camera object without a slot binding, this camera will not render!");
	it = args.find("clear_colour");
	if (it != args.end())
		obj->clear_colour = it->second.c_value;
	it = args.find("near_clip");
	if (it != args.end())
		obj->near_clip = it->second.f_value;
	it = args.find("far_clip");
	if (it != args.end())
		obj->far_clip = it->second.f_value;
	it = args.find("fov");
	if (it != args.end())
		obj->fov = it->second.f_value;
	
	return object;
}

static Ref<Object> deserialiseTextBlock(const map<string, TokenReader::Token>& args, const Ref<Scene>& scene, const SceneResources& resources)
{
	Ref<Object> object = Object::create();
	WeakRef<TextComponent> obj = object->addComponent<TextComponent>();
	auto it = args.find("text");
	if (it != args.end())
		obj->setText(it->second.s_value);
	it = args.find("tint");
	if (it != args.end())
		obj->setTint(it->second.c_value);
	
	return object;
}

struct ObjectDeserialiseConfig
{
	Ref<Object> (* builder_function)(const map<string, TokenReader::Token>&, const Ref<Scene>&, const SceneResources& resources);
	map<string, TokenReader::TokenType> arguments;
};

static map<string, ObjectDeserialiseConfig> object_deserialisers = {
	{ "StaticMesh", { deserialiseStaticMesh, {
						{ "mesh", TokenReader::IDENTIFIER },
						{ "material", TokenReader::IDENTIFIER },
                        { "camera_mask", TokenReader::INT }
	} } },
	{ "Light", { deserialiseLight, {
						{ "type", TokenReader::TEXT },
						{ "colour", TokenReader::VECTOR },
						{ "angle", TokenReader::FLOAT }
	} } },
	{ "Camera", { deserialiseCamera, {
						{ "slot", TokenReader::INT },
						{ "clear_colour", TokenReader::VECTOR },
						{ "near_clip", TokenReader::FLOAT },
						{ "far_clip", TokenReader::FLOAT },
						{ "fov", TokenReader::FLOAT },
	} } },
	{ "TextBlock", { deserialiseTextBlock, {
							{ "text", TokenReader::STRING },
							{ "tint", TokenReader::VECTOR }
	} } }
};

static bool deserialiseObject(const TokenReader::Statement& statement, const Ref<Scene>& scene, const SceneResources& resources, const Ref<Object>& parent, const string& name)
{
	Ref<Object> obj;
	map<string, pair<TokenReader::TokenType, bool>> expected = {
		{ "position", { TokenReader::VECTOR, false } },
		{ "euler", { TokenReader::VECTOR, false } },
		{ "scale", { TokenReader::VECTOR, false } }
	};
	ObjectDeserialiseConfig info;
	if (statement.keyword != "Object")
	{
		info = object_deserialisers[statement.keyword];
		for (const auto& thing : info.arguments)
			expected[thing.first] = { thing.second, false };
	}
	map<string, TokenReader::Token> args;
	if (!TokenReader::readStatementNamed(statement, true, false,
				expected, args, "error deserialising scene '" + name + "'"))
		return false;
	
	if (statement.keyword == "Object")
		obj = Object::create();
	else
		obj = info.builder_function(args, scene, resources);
	if (obj == nullptr)
		return false;
	
	scene->insertObject(obj);
	if (parent)
		parent->addChild(obj);
	
	auto it = args.find("position");
	if (it != args.end())
		obj->transform.setLocalPosition(it->second.c_value);
	else
		obj->transform.setLocalPosition({ 0, 0, 0 });
	it = args.find("euler");
	if (it != args.end())
		obj->transform.setLocalEuler(it->second.c_value);
	else
		obj->transform.setLocalEuler({ 0, 0, 0 });
	it = args.find("scale");
	if (it != args.end())
		obj->transform.setLocalScale(it->second.c_value);
	else
		obj->transform.setLocalScale({ 1, 1, 1 });
	if (!statement.identifier.empty())
		obj->name = statement.identifier;
	
	for (const TokenReader::Statement& child : statement.children)
	{
		if (object_deserialisers.contains(child.keyword) || child.keyword == "Object")
		{
			if (!deserialiseObject(child, scene, resources, obj, name))
				return false;
		}
		else
		{
			DBG_ERROR("error deserialising scene '" + name + "': invalid keyword '" + child.keyword + "'");
			return false;
		}
	}
	return true;
}

Ref<Scene> Scene::deserialise(const string& name)
{
	auto raw_data = Package::load(name);
	if (raw_data.empty())
		return nullptr;

	const string token_str(reinterpret_cast<char*>(raw_data.data()), raw_data.size());
	const auto tokens = TokenReader::tokenise(token_str);
	if (tokens.empty())
		return nullptr;

	const auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
	if (syntax_tree.empty())
		return nullptr;

	map<string, Ref<Material>> materials;
	map<string, Ref<Mesh>> meshes;
	map<string, Ref<Texture>> textures;
	map<string, Ref<RenderGraph>> render_graphs;
	
	Ref<Scene> scene = Scene::create(name);
	
	for (const TokenReader::Statement& statement : syntax_tree)
	{
		if (statement.keyword == "Resource")
		{
			vector<TokenReader::Token> args;
			if (!TokenReader::readStatementAnonymous(statement, false, true,
				{
					TokenReader::TEXT,
					TokenReader::STRING
				}, args, "error deserialising scene '" + name + "'"))
				return nullptr;
			if (args[0].s_value == "material")
				materials[statement.identifier] = Engine::loadMaterial(args[1].s_value);
			else if (args[0].s_value == "texture")
				textures[statement.identifier] = Engine::loadTexture(args[1].s_value);
			else if (args[0].s_value == "mesh")
				meshes[statement.identifier] = Engine::loadMesh(args[1].s_value);
			else if (args[0].s_value == "render_graph")
				render_graphs[statement.identifier] = RenderGraph::deserialise(args[1].s_value);
			else
			{
				DBG_ERROR("error deserialising scene '" + name + "': invalid resource type");
				return nullptr;
			}
		}
		else if (statement.keyword == "AmbientLight")
		{
			vector<TokenReader::Token> args;
			if (!TokenReader::readStatementAnonymous(statement, false, false,
				{
					TokenReader::VECTOR
				}, args, "error deserialising scene '" + name + "'"))
				return nullptr;
			scene->ambient_colour = args[0].c_value;
		}
		else if (statement.keyword == "Skybox")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, false,
				{
					{ "resource", { TokenReader::IDENTIFIER, true } }
				}, args, "error deserialising scene '" + name + "'"))
				return nullptr;
			auto it = textures.find(args["resource"].s_value);
			if (it == textures.end())
			{
				DBG_ERROR("error deserialising scene '" + name + "': no such texture loaded '" + args["resource"].s_value + "'");
				return nullptr;
			}
			scene->skybox = it->second;
		}
		else if (statement.keyword == "RenderGraph")
		{
			map<string, TokenReader::Token> args;
			if (!TokenReader::readStatementNamed(statement, false, false,
				{
					{ "resource", { TokenReader::IDENTIFIER, true } }
				}, args, "error deserialising scene '" + name + "'"))
				return nullptr;
			auto it = render_graphs.find(args["resource"].s_value);
			if (it == render_graphs.end())
			{
				DBG_ERROR("error deserialising scene '" + name + "': no such render graph loaded '" + args["resource"].s_value + "'");
				return nullptr;
			}
			scene->render_graph = it->second;
		}
		else if (object_deserialisers.contains(statement.keyword) || statement.keyword == "Object")
		{
			if (!deserialiseObject(statement, scene, { materials, meshes, textures }, nullptr, name))
				return nullptr;
		}
		else
		{
			DBG_ERROR("error deserialising scene '" + name + "': invalid keyword '" + statement.keyword + "'");
			return nullptr;
		}
	}
	return scene;
}