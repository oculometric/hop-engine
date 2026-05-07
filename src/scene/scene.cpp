#include "scene.h"

#include "basic_components.h"
#include "command_buffer.h"
#include "render_graph.h"
#include "render_server.h"
#include "texture.h"

#include <map>

using namespace HopEngine;

Ref<Scene> Scene::create(const std::string& name)
{
    Ref scn          = new Scene(name);
    scn->self        = scn;
    scn->root->scene = scn;
    return scn;
}

Scene::~Scene() { DBG_INFO("destroying scene " + getOrigin()); }

std::vector<WeakRef<Object>> Scene::getAllObjects() const
{
    std::vector<WeakRef<Object>> objs;
    objs.reserve(objects.size());
    for (auto& obj : objects) objs.push_back(obj);
    return objs;
}

WeakRef<Object> Scene::findObject(const std::string& name) const
{
    for (auto& test_obj : objects)
    {
        if (test_obj->name == name) return test_obj;
    }
    return nullptr;
}

Ref<Object> Scene::insertObject(Ref<Object> obj)
{
    if (obj->scene)
    {
        if (obj->scene == self)
        {
            if (!obj->parent)
            {
                obj->parent = root;
                root->children.push_back(obj);
                return obj;
            }
            return obj;
        }
        obj->scene->removeObject(obj);
    }
    objects.push_back(obj);
    obj->scene = self;
    if (!obj->parent)
    {
        obj->parent = root;
        root->children.push_back(obj);
    }
    for (const auto& child : obj->children) insertObject(child);
    return obj;
}

Ref<Object> Scene::addObject(const std::string& name)
{
    Ref<Object> obj = Object::create();
    obj->name       = name;
    obj->scene      = self;
    objects.push_back(obj);
    obj->parent = root;
    root->children.push_back(obj);
    return obj;
}

void Scene::removeObject(Ref<Object> obj)
{
    if (obj->scene != self)
    {
        DBG_WARNING("object " + obj->name + " is not a member of scene " + getOrigin());
        return;
    }

    // remove children from the scene
    auto temp_children = obj->children;
    for (auto& child : temp_children) removeObject(child);
    temp_children.clear();

    // remove object from parent
    if (obj->parent)
    {
        for (auto it2 = obj->parent->children.begin(); it2 != obj->parent->children.end(); ++it2)
        {
            if ((*it2) == obj)
            {
                obj->parent->children.erase(it2);
                obj->parent = nullptr;
                break;
            }
        }
        if (obj->parent)
            DBG_WARNING("object " + obj->name + " claims to be a child of object " + obj->parent->name +
                        ", but could not be found in the parents child list.");
    }
    else
        DBG_WARNING("object " + obj->name + " has no parent. this is not allowed.");

    // remove object from scene
    for (auto it = objects.begin(); it != objects.end(); ++it)
    {
        if ((*it) == obj)
        {
            objects.erase(it);
            obj->scene = nullptr;
            break;
        }
    }
    if (obj->scene)
        DBG_WARNING("object " + obj->name + " claims to be a member of scene " + getOrigin() +
                    ", but is not known to the scene. scene hierarchy is corrupt.");
}

WeakRef<Object> Scene::raycast(const glm::vec3 position, const glm::vec3 direction) const
{
    float min_dist = INFINITY;
    WeakRef<Object> closest_obj;
    for (auto& object : objects)
    {
        const float result =
            intersect(position, direction, object->getLocalBounds(), object->transform.getMatrix());
        if (result < 0.01f) continue;
        if (result < min_dist)
        {
            min_dist    = result;
            closest_obj = object;
        }
    }
    return closest_obj;
}

void Scene::update(float delta_time) { root->update(delta_time); }

void Scene::draw(Ref<DrawCommandBuffer> command_buffer, glm::u32vec2 viewport_size)
{
    last_viewport_size = viewport_size;
    if (!render_graph) return;

    // resize render graph
    render_graph->resizeBuffers(viewport_size);

    // collect all lights from the scene
    std::vector<LightParams> lights(8);

    size_t index = 0;
    for (const auto& light : objects)
    {
        auto light_comp = light->getComponent<LightComponent>();
        if (!light_comp) continue;
        lights[index] = light_comp->getParamsStructure();
        ++index;
        if (index >= 8) break;
    }

    // check the size of each camera slot
    std::map<size_t, std::pair<WeakRef<UniformBlock>, glm::vec4>> cameras;
    auto camera_sizes = render_graph->getCameraSlots();
    for (const auto& object : objects)
    {
        auto camera_comp = object->getComponent<CameraComponent>();
        if (!camera_comp) continue;
        if (!camera_sizes.contains(camera_comp->camera_slot)) continue;
        // find the first camera for each slot, and update its uniforms to be correct (and store)
        cameras[camera_comp->camera_slot] = { camera_comp->getUniforms(
                                                  camera_sizes[camera_comp->camera_slot], lights,
                                                  glm::vec4(ambient_colour, 0)),
            glm::vec4(camera_comp->clear_colour, 1) };
    }

    // collect all draw calls from the scene (plus the skybox!)
    std::vector<DrawCommand> draw_commands;
    if (sky) draw_commands.push_back(sky->getDrawCommand());
    for (const auto& object : objects)
    {
        auto temp = object->getDrawCommands();
        draw_commands.insert(draw_commands.end(), temp.begin(), temp.end());
    }

    // call render graph draw with the cameras and draw calls
    render_graph->draw(command_buffer, draw_commands, cameras);
}

void Scene::bindOutputMaterial(Ref<DrawCommandBuffer> command_buffer)
{
    if (render_graph) render_graph->bindOutputMaterial(command_buffer);
}

Scene::Scene(const std::string& name)
{
    origin       = name;
    render_graph = new RenderGraph(RenderGraph::Builder().addCamera(0));
    root         = Object::create();
    root->name   = "scene root";

    DBG_INFO("created new scene " + getOrigin());
}
