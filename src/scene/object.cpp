#include "graphics_server.h"
#include "scene.h"

using namespace HopEngine;

WeakRef<Scene> Component::getScene() const { return owner->getScene(); }

Transform& Component::getTransform() const { return owner->getTransform(); }

Ref<Object> Object::create()
{
    Ref obj   = new Object();
    obj->self = obj;
    return obj;
}

void Object::removeFromParent()
{
    if (!parent) return;
    for (auto it = parent->children.begin(); it != parent->children.end(); ++it)
    {
        if ((*it) == self)
        {
            parent->children.erase(it);
            parent = nullptr;
            if (scene) scene->insertObject(self.strong());
            return;
        }
    }
    DBG_WARNING("object " + name + " claims to be a child of object " + parent->name +
                ", but could not be found in the parents child list.");
}

void Object::addChild(Ref<Object> obj)
{
    if (obj->scene && obj->scene != scene)
    {
        DBG_WARNING("object " + obj->name + " is already present in another scene hierarchy.");
        return;
    }
    if (obj->parent)
    {
        bool parent_found = false;
        for (auto it = obj->parent->children.begin(); it != obj->parent->children.end(); ++it)
        {
            if ((*it) == obj)
            {
                obj->parent->children.erase(it);
                parent_found = true;
                break;
            }
        }
        if (!parent_found)
        {
            DBG_WARNING("object " + name + " claims to be a child of object " + parent->name +
                        ", but could not be found in the parents child list.");
        }
    }
    obj->parent = self;
    children.push_back(obj);
    if (scene && !obj->scene) scene->insertObject(obj);
}

void Object::update(float delta_time)
{
    for (const auto& comp : components)
    {
        if (!comp->getEnabled()) continue;
        comp->update(delta_time);
    }
    for (const auto& child : children) child->update(delta_time);
}

std::vector<DrawCommand> Object::getDrawCommands()
{
    std::vector<DrawCommand> commands;
    for (const auto& comp : components)
    {
        if (!comp->getEnabled()) continue;
        auto sub_commands = comp->getDrawCommands();
        commands.insert(commands.begin(), sub_commands.begin(), sub_commands.end());
    }
    return commands;
}

BoundingBox Object::getLocalBounds() const
{
    return BoundingBox{
        {    0,    0,    0 },
        { 0.1f, 0.1f, 0.1f }
    };
}

Object::Object() { transform.owner = this; }
