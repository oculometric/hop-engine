#include "basic_components.h"
#include "deserialise.h"
#include "engine.h"
#include "material.h"
#include "mesh.h"
#include "package.h"
#include "render_graph.h"
#include "scene.h"
#include "texture.h"
#include "user_interface.h"

#include <filesystem>

using namespace HopEngine;

struct SceneResources
{
    std::map<std::string, Ref<Material>> materials;
    std::map<std::string, Ref<Mesh>> meshes;
    std::map<std::string, Ref<Texture>> textures;
};

static bool deserialiseStaticMesh(const Deserialiser::NamedStatementResult& result, WeakRef<Object> object,
    const Ref<Scene>& scene, const SceneResources& resources)
{
    WeakRef<StaticMeshComponent> obj = object->addComponent<StaticMeshComponent>();
    std::string mesh_res;
    result.read("mesh", mesh_res);
    if (!mesh_res.empty())
    {
        const auto mesh_it = resources.meshes.find(mesh_res);
        if (mesh_it == resources.meshes.end())
        {
            DBG_ERROR("error deserialising static mesh, invalid mesh identifier '" + mesh_res + "'");
            return false;
        }
        obj->mesh = mesh_it->second;
    }
    std::string mat_res;
    result.read("material", mat_res);
    if (!mat_res.empty())
    {
        const auto material_it = resources.materials.find(mat_res);
        if (material_it == resources.materials.end())
        {
            DBG_ERROR("error deserialising static mesh, invalid material identifier '" + mat_res + "'");
            return false;
        }
        obj->material = material_it->second;
    }
    result.read("camera_mask", obj->camera_mask);
    return true;
}

static bool deserialiseLight(const Deserialiser::NamedStatementResult& result, WeakRef<Object> object,
    const Ref<Scene>& scene, const SceneResources& resources)
{
    WeakRef<LightComponent> obj = object->addComponent<LightComponent>();
    if (!result.read<LightComponent::LightType>("type", obj->type,
            [](const std::string& s, LightComponent::LightType& d) -> bool
            {
                if (s == "DIRECTIONAL") d = LightComponent::DIRECTIONAL;
                else if (s == "POINT")
                    d = LightComponent::POINT;
                else if (s == "SPOT")
                    d = LightComponent::SPOT;
                else
                {
                    DBG_ERROR("error deserialising light, invalid light type '" + s + "'");
                    return false;
                }
                return true;
            }))
        return false;
    result.read("colour", obj->colour);
    result.read("angle", obj->spot_angle);

    return true;
}

static bool deserialiseCamera(const Deserialiser::NamedStatementResult& result, WeakRef<Object> object,
    const Ref<Scene>& scene, const SceneResources& resources)
{
    WeakRef<CameraComponent> obj = object->addComponent<CameraComponent>();
    int slot                     = -1;
    result.read("slot", slot);
    if (slot >= 0) obj->camera_slot = slot;
    else
        DBG_WARNING("deserialising a camera object without a slot binding, this camera will not render!");
    result.read("clear_colour", obj->clear_colour);
    result.read("near_clip", obj->near_clip);
    result.read("far_clip", obj->far_clip);
    result.read("fov", obj->fov);

    return true;
}

static bool deserialiseTextBlock(const Deserialiser::NamedStatementResult& result, WeakRef<Object> object,
    const Ref<Scene>& scene, const SceneResources& resources)
{
    WeakRef<TextComponent> obj = object->addComponent<TextComponent>();
    std::string text           = "Text";
    result.read("text", text);
    obj->setText(text);
    glm::vec3 tint{ 1, 1, 1 };
    result.read("tint", tint);
    obj->setTint(tint);

    return true;
}

struct ObjectDeserialiseConfig
{
    bool (*builder_function)(const Deserialiser::NamedStatementResult&, WeakRef<Object>, const Ref<Scene>&,
        const SceneResources&);
    std::map<std::string, TokenReader::TokenType> arguments;
};

static std::map<std::string, ObjectDeserialiseConfig> object_deserialisers = {
    { "StaticMesh",{ deserialiseStaticMesh, { { "mesh", TokenReader::TOKEN_IDENTIFIER },
{ "material", TokenReader::TOKEN_IDENTIFIER },
{ "camera_mask", TokenReader::TOKEN_INT } } }                                                },
    {      "Light",
     { deserialiseLight, { { "type", TokenReader::TOKEN_TEXT }, { "colour", TokenReader::TOKEN_VECTOR },
     { "angle", TokenReader::TOKEN_FLOAT } } }                                                           },
    {     "Camera",                                                                                   { deserialiseCamera,
                                                                                   {
                                                                                   { "slot", TokenReader::TOKEN_INT },
                                                                                   { "clear_colour", TokenReader::TOKEN_VECTOR },
                                                                                   { "near_clip", TokenReader::TOKEN_FLOAT },
                                                                                   { "far_clip", TokenReader::TOKEN_FLOAT },
                                                                                   { "fov", TokenReader::TOKEN_FLOAT },
                                                                                   } } },
    {  "TextBlock", { deserialiseTextBlock,
 { { "text", TokenReader::TOKEN_STRING }, { "tint", TokenReader::TOKEN_VECTOR } } }   }
};

Ref<Scene> Scene::deserialiseFile(const std::string& name)
{
    auto raw_data = Package::load(name);
    if (raw_data.empty()) return nullptr;
    std::string code(reinterpret_cast<char*>(raw_data.data()), raw_data.size());
    raw_data.clear();
    return deserialise(code, name);
}

Ref<Scene> Scene::deserialise(const std::string& token_str, const std::string& origin)
{
    const auto tokens = TokenReader::tokenise(token_str);
    if (tokens.empty()) return nullptr;

    const auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
    if (syntax_tree.empty()) return nullptr;

    std::map<std::string, Ref<Material>> materials;
    std::map<std::string, Ref<Mesh>> meshes;
    std::map<std::string, Ref<Texture>> textures;
    std::map<std::string, Ref<RenderGraph>> render_graphs;

    Ref<Scene> scene = Scene::create(origin);

    Deserialiser deserialiser("error deserialising scene '" + origin + "'");
    deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("Resource", Deserialiser::STATEMENT_IDENTIFIER_REQUIRED, false)
            .argument(TokenReader::TOKEN_TEXT)
            .argument(TokenReader::TOKEN_STRING),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
        {
            std::string res_type;
            result.read(0, res_type);
            std::string res_addr;
            result.read(1, res_addr);
            if (res_type == "material")
                materials[result.statement.identifier] = Engine::loadMaterial(res_addr);
            else if (res_type == "texture")
                textures[result.statement.identifier] = Engine::loadTexture(res_addr);
            else if (res_type == "mesh")
                meshes[result.statement.identifier] = Engine::loadMesh(res_addr);
            else if (res_type == "render_graph")
                render_graphs[result.statement.identifier] = RenderGraph::deserialiseFile(res_addr);
            else
                return deserialiser.emitError("invalid resource type", result.offsetOf(0), token_str);
            return true;
        });
    deserialiser.addStatementAnonymous(Deserialiser::AnonymousStatementSpec("AmbientLight",
                                           Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
                                           .argument(TokenReader::TOKEN_VECTOR),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
        {
            result.read(0, scene->ambient_colour);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Skybox", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("resource", TokenReader::TOKEN_IDENTIFIER, true),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            std::string texture_res;
            result.read("resource", texture_res);
            auto texture_it = textures.find(texture_res);
            if (texture_it == textures.end())
                return deserialiser.emitError("no such texture loaded '" + texture_res + "'",
                    result.offsetOf("resource"), token_str);
            scene->sky = new Sky(texture_it->second);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("RenderGraph", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("resource", TokenReader::TOKEN_IDENTIFIER, true),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            std::string rg_res;
            result.read("resource", rg_res);
            auto rg_it = render_graphs.find(rg_res);
            if (rg_it == render_graphs.end())
                return deserialiser.emitError("no such render graph loaded '" + rg_res + "'",
                    result.offsetOf("resource"), token_str);
            scene->render_graph = rg_it->second;
            return true;
        });

    std::vector<WeakRef<Object>> parent_stack;
    Deserialiser object_deserialiser("error deserialising scene '" + origin + "'");

    auto object_spec =
        Deserialiser::NamedStatementSpec("Object", Deserialiser::STATEMENT_IDENTIFIER_OPTIONAL, true)
            .argument("position", TokenReader::TOKEN_VECTOR, false)
            .argument("euler", TokenReader::TOKEN_VECTOR, false)
            .argument("scale", TokenReader::TOKEN_VECTOR, false);
    auto object_handler_base = [&](Deserialiser::NamedStatementResult result) -> WeakRef<Object>
    {
        Ref<Object> obj;
        if (result.statement.identifier.empty()) obj = scene->addObject("object");
        else
            obj = scene->addObject(result.statement.identifier);
        if (!parent_stack.empty()) parent_stack[parent_stack.size() - 1]->addChild(obj);
        glm::vec3 temp = { 0, 0, 0 };
        result.read("position", temp);
        obj->transform.setLocalPosition(temp);
        temp = { 0, 0, 0 };
        result.read("euler", temp);
        obj->transform.setLocalEuler(temp);
        temp = { 1, 1, 1 };
        result.read("scale", temp);
        obj->transform.setLocalScale(temp);
        parent_stack.push_back(obj);
        if (!object_deserialiser.execute(result.statement.children, token_str)) return nullptr;
        parent_stack.pop_back();
        return obj;
    };
    auto object_handler = [&](Deserialiser::NamedStatementResult result) -> bool
    {
        if (!object_handler_base(result)) return false;
        return true;
    };

    deserialiser.addStatementNamed(object_spec, object_handler);
    object_deserialiser.addStatementNamed(object_spec, object_handler);

    for (const auto& pair : object_deserialisers)
    {
        auto spec         = object_spec;
        spec.keyword_name = pair.first;
        for (const auto& arg : pair.second.arguments) spec.argument(arg.first, arg.second, false);
        auto handler = [&](Deserialiser::NamedStatementResult result) -> bool
        {
            WeakRef<Object> obj = object_handler_base(result);
            if (!obj) return false;
            return pair.second.builder_function(result, obj, scene, { materials, meshes, textures });
        };
        deserialiser.addStatementNamed(spec, handler);
        object_deserialiser.addStatementNamed(spec, handler);
    }

    if (!deserialiser.execute(syntax_tree, token_str)) return nullptr;

    return scene;
}