/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"
#include "math_helpers.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace HopEngine
{

/**
 * @brief 3D transformation structure. contains utilities for applying transformations both in local space
 * and world space. applies transformations in a nested hierarchy. all transformation functions of the form
 * `...Local...` operate in local space, whereas other functions operate in world space.
 */
struct Transform final
{
    friend class Object;

private:
    Object* owner = nullptr;  // object which this transform belongs to, used to query parent
    glm::vec3 local_position; // position within parent's local space
    glm::vec3 local_euler;    // rotation (applied XYZ) within parent's local space
    glm::vec3 local_scale;    // scale within parent's local space
    glm::mat4 local_matrix;   // matrix transforming points from local space into parent's local space
    glm::mat4 world_matrix;   // matrix transforming points from local space to world space

    // TODO: need separate internal matrices for rotation, offset, scale, and simplified functions to
    // recalculate
public:
    Transform() : local_position({ 0, 0, 0 }), local_euler({ 0, 0, 0 }), local_scale({ 1, 1, 1 })
    { localFromVars(); };
    Transform(const glm::vec3 position, const glm::vec3 euler, const glm::vec3 scale) :
        local_position(position), local_euler(euler), local_scale(scale)
    { localFromVars(); }

    glm::vec3 getLocalPosition() const { return local_position; }
    glm::vec3 getLocalEuler() const { return local_euler; }
    glm::vec3 getLocalScale() const { return local_scale; }
    glm::mat4 getLocalMatrix() const { return local_matrix; }
    glm::vec3 getPosition() const { return world_matrix[3]; }
    // TODO: glm::vec3 getEuler() const;
    glm::mat4 getMatrix() { return world_matrix; }
    glm::vec3 right() const { return world_matrix[0]; }    // represents world space X axis
    glm::vec3 up() const { return world_matrix[1]; }       // represents world space Y axis
    glm::vec3 forward() const { return -world_matrix[2]; } // represents world space -Z axis

    void setLocalPosition(glm::vec3 position);
    void setLocalEuler(glm::vec3 euler);
    void setLocalScale(glm::vec3 scale);
    void setPosition(glm::vec3 position);
    // TODO: void setEuler(glm::vec3 euler);
    void setMatrix(const glm::mat4& matrix);

    void translateLocal(glm::vec3 offset);
    void rotateLocal(glm::vec3 degrees);
    void scaleLocal(glm::vec3 factor);
    void scaleLocal(float factor);
    void translate(glm::vec3 offset);
    // TODO: void rotate(glm::vec3 axis, float degrees);
    void rotate(glm::vec3 degrees);
    void scale(float factor);
    void lookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up);
    // TODO: void fromBasis(glm::vec3 negative_z, glm::vec3 positive_y)

    // TODO: quaternion support

private:
    void localFromWorld();
    void worldFromLocal();
    void localFromVars();
};

/**
 * @brief describes an instruction to render something, primarily consisting of a mesh and material.
 */
struct DrawCommand final
{
    // material to render the surface. contains shader, pipeline, and material uniforms (descriptor set 2)
    WeakRef<Material> material;
    WeakRef<Mesh> mesh;                // mesh to be drawn
    WeakRef<UniformBlock> uniforms;    // instance-/object-specific uniforms to be bound (descriptor set 1)
    int draw_priority    = 0;          // ordering bias to force objects to render early/late
    uint32_t camera_mask = 0xFFFFFFFF; // bit mask determining which camera slots the mesh should render in

    /**
     * @brief compares draw commands for ordering them. order is determined first by priority, then by
     * shader, then by material, then by uniforms, then by mesh. ordering draw commands can help reduce the
     * number of bind commands needed to be sent to the GPU.
     * @param a first draw command.
     * @param b second draw command.
     * @returns `true` if `a` should be ordered before `b`, otherwise `false`.
     */
    static bool compare(const DrawCommand& a, const DrawCommand& b);

    /**
     * @brief non-static operator version of `compare`.
     * @param a first draw command.
     * @param b second draw command.
     * @returns `true` if `a` should be ordered before `b`, otherwise `false`.
     */
    bool operator()(const DrawCommand& a, const DrawCommand& b) const;

    DrawCommand() = default;
    DrawCommand(const WeakRef<Material>& _material, const WeakRef<Mesh>& _mesh,
        const WeakRef<UniformBlock>& _uniforms = WeakRef<UniformBlock>()) :
        material(_material), mesh(_mesh), uniforms(_uniforms)
    {
    }

    /**
     * @brief builder-style function which sets the draw priority of the draw command.
     * @param value new value for `draw_priority`.
     * @returns self-reference for chaining function calls.
     */
    DrawCommand& priority(const int value)
    {
        draw_priority = value;
        return *this;
    }
    /**
     * @brief builder-style function which sets the camera mask of the draw command.
     * @param value new value for `camera_mask`.
     * @returns self-reference for chaining function calls.
     */
    DrawCommand& mask(const uint32_t value)
    {
        camera_mask = value;
        return *this;
    }
};

/**
 * @brief helper struct describing how multiple priority factors (for instance, draw command priority)
 * should interact.
 */
struct Priority final
{
    enum Mode
    {
        PRIORITY_REPLACE,  // simply replace the base priority with the incoming one
        PRIORITY_ADD,      // return the sum of the two priorities
        PRIORITY_MULTIPLY, // return the product of the two priorities
        PRIORITY_SUBTRACT, // subtract the incoming priority from the base one
        PRIORITY_IGNORE    // ignore the incoming priority and simply return the base one
    };

    Mode mode    = PRIORITY_REPLACE;
    int priority = 0;

    int calculatePriority(int base) const;
};

/**
 * @brief scene component base class which can be attached to an object to participate in a scene. all
 * actual behaviour happens through scene components. may be extended to provide additional functionality.
 * components should never be constructed directly, only via `Object::addComponent`.
 */
class Component : public Destructible
{
    friend class Object;

private:
    WeakRef<Object> owner; // object to which this component is attached
    // if `false`, this component will be ignored and its functions will not be called (`getDrawCommands`,
    // `getLocalBounds`, and `update`)
    bool enabled = true;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Component);
    Component()           = default;
    ~Component() override = default;

    WeakRef<Object> getOwner() const { return owner; }
    /**
     * @brief looks for the first component of type `T` on the owning object.
     * @returns reference to the first matching component, or `nullptr` if nothing matching was found.
     */
    template<class T> WeakRef<T> getComponent();
    WeakRef<Scene> getScene() const;
    Transform& getTransform() const;
    /**
     * @brief toggles whether the component participates in the scene. when `state` is `false`, the
     * component will be ignored and its functions will not be called (`getDrawCommands`, `getLocalBounds`,
     * and `update`) during scene operations.
     * @param state whether or not the component will be active/enabled.
     */
    void setEnabled(bool state) { enabled = state; }
    bool getEnabled() const { return enabled; }

    /**
     * @brief called when the component is instantiated. may be overridden by the user to provide custom
     * startup behaviour if desired.
     */
    virtual void awake() {}
    /**
     * @brief called every frame, if the component is enabled. may be overridden by the user to provide
     * custom per-frame behaviour if desired.
     * @param delta_time amount of time in seconds since the last frame.
     */
    virtual void update(float delta_time) {}

    /**
     * @brief called when rendering the scene, allowing the component to specify one or more rendering
     * instructions (i.e. drawing a mesh). updates to uniform buffers or other similar pre-rendering tasks
     * are also performed here. may be overridden by the user to provide custom drawing behaviour if
     * desired.
     * @returns array of rendering commands which will be submitted as part of rendering.
     */
    virtual std::vector<DrawCommand> getDrawCommands() { return {}; }
    /**
     * @brief called to query the apparent bounding box of the component, used for selection and other
     * debug-related behaviour. may be overridden by the user to provide custom bounding box information if
     * desired.
     * @returns bounding box of the component in object-local space.
     */
    virtual BoundingBox getLocalBounds() const { return BoundingBox{}; }

    virtual void drawImGuiDebug();
};

/**
 * @brief scene hierarchy participant. can have any number of components attached which implement various
 * behaviours, although having multiple components of the same type is not fully supported.
 */
class Object final : public Destructible
{
    friend class Scene;

public:
    std::string name = "object"; // name of the object for debug and searching purposes

private:
    Transform transform;                    // 3D transformation information for the object
    WeakRef<Object> self;                   // self reference for passing to children/parents
    WeakRef<Scene> scene;                   // scene which this object is a participant of
    WeakRef<Object> parent;                 // parent object in the hierarchy, if any
    std::vector<Ref<Object>> children;      // list of child objects in the hierarchy, if any
    std::vector<Ref<Component>> components; // list of owned/attached components

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Object);
    static Ref<Object> create();
    ~Object() override {}

    WeakRef<Object> getSelf() const { return self; }
    WeakRef<Scene> getScene() const { return scene; }

    WeakRef<Object> getParent() const { return parent; }
    Transform& getTransform() { return transform; }
    size_t getChildCount() const { return children.size(); }
    /**
     * @brief retrieves the child object at the specified index. does not perform bounds checking.
     * @param index index of the child to retrieve, must be less than the value returned by `getChildCount`.
     * @returns reference to the queried child object.
     */
    WeakRef<Object> getChild(size_t index) const { return children[index]; }
    /**
     * @brief adds a specified child object into the hierarchy. you should not attempt to call this function
     * with objects from a different scene.
     * @param obj object which will become a child of this object.
     */
    void addChild(Ref<Object> obj);
    /**
     * @brief removes the object from its parent object, if there is any parent object. if the object is
     * part of a scene, then the object will be made a child of only the scene root object.
     */
    void removeFromParent();

    /**
     * @brief adds a new component of the specified type. the new component is constructed and initialised
     * appropriately. you may only call this function with templates such that `T` inherits from
     * `Component`.
     * @returns reference to the newly created component.
     */
    template<class T> Ref<T> addComponent();
    /**
     * @brief searches through the list of attached components to find a matching component. only returns
     * the first matching component. you may only call this function with templates such that `T` inherits
     * from `Component`.
     * @returns reference to the first component of the specified type, or `nullptr` if none could be found.
     */
    template<class T> WeakRef<T> getComponent();
    /**
     * @brief searches through the list of attached components to find a matching component, and removes it.
     * only applied to the first matching component. you may only call this function with templates such
     * that `T` inherits from `Component`.
     * @returns `true` if a component was successfully found and removed, otherwise `false`.
     */
    template<class T> bool removeComponent();

    /**
     * @brief called once per frame, and propagated to child objects and attached components.
     * @param delta_time time in seconds since the last frame.
     */
    void update(float delta_time);
    /**
     * @brief queries attaced components for rendering commands and collates them.
     * @returns list of drawing commands for the object.
     */
    std::vector<DrawCommand> getDrawCommands();
    /**
     * @brief queries attached components for bounding boxes and combines them via a union operation to find
     * the overall bounding box for the object.
     * @returns overall bounding box for the object.
     */
    BoundingBox getLocalBounds() const;

    void drawImGuiDebug();

private:
    /**
     * @brief internal-use-only. use `Object::create` instead.
     */
    Object();
};

template<class T> WeakRef<T> Component::getComponent() { return owner->getComponent<T>(); }

template<class T> Ref<T> Object::addComponent()
{
    static_assert(std::is_convertible_v<T*, Component*>,
        "component must be a HopEngine::Component subclass");
    Ref<T> comp = new T();
    comp->owner = self;
    components.push_back(comp.template cast<Component>());
    comp->awake();
    return comp;
}

template<class T> WeakRef<T> Object::getComponent()
{
    static_assert(std::is_convertible_v<T*, Component*>,
        "component must be a HopEngine::Component subclass");
    for (auto& comp : components)
    {
        if (dynamic_cast<T*>(comp.get())) return comp.template cast<T>();
    }
    return WeakRef<T>();
}

template<class T> bool Object::removeComponent()
{
    static_assert(std::is_convertible_v<T*, Component*>,
        "component must be a HopEngine::Component subclass");
    for (auto it = components.begin(); it != components.end(); ++it)
    {
        if (dynamic_cast<T>((*it).get()))
        {
            components.erase(it);
            return true;
        }
    }
    return false;
}

/**
 * @brief contains information about how to render the backdrop (or skybox) for a scene. supports both
 * simple skybox-rendering (where a texture can be supplied) as well as custom material rendering using
 * either a cube or an icosahedron as the mesh.
 */
class Sky final : public Destructible
{
private:
    // if `true` the skybox cube mesh will be rendered, otherwise the sky sphere will be rendered
    bool render_as_cube = true;
    // if `true` the skybox is using a custom material for rendering, not the standard skybox material
    bool render_custom_material = false;
    Ref<UniformBlock> uniforms; // object uniforms for the skybox
    Ref<Material> material;     // material used to render the skybox

public:
    DELETE_CONSTRUCTORS(Sky);
    Sky(Ref<Texture> skybox_texture);
    Sky(Ref<Material> custom_material, bool render_cube);
    ~Sky() override = default;

    /**
     * @brief updates the sky to use a skybox cubemap texture. the texture should be 2D, but arranged in a
     * 6x1 confiuration of +X, -X, +Y, -Y, +Z, -Z faces as seen from the inside of the skybox. (see
     * `res_engine/textures/basic_skybox.png` for an example). switches to standard skybox material if a
     * custom material is currently in use. forces the use of the skybox cube for rendering.
     * @param skybox_texture texture to use for the skybox.
     */
    void setSkyboxCubemap(Ref<Texture> skybox_texture);
    /**
     * @brief updates the sky to use a custom material.
     * @param custom_material material used for rendering the skybox.
     * @param render_cube if `true` the standard skybox cube will be used for rendering, otherwise a sky
     * sphere (icosahedron) will be used instead.
     */
    void setCustomMaterial(Ref<Material> custom_material, bool render_cube);

    /**
     * @brief generates a command for drawing the skybox into a command buffer. resulting command has
     * extremely high priority to ensure the object is drawn first.
     * @returns rendering command to draw the skybox.
     */
    DrawCommand getDrawCommand() const;
};

/**
 * @brief scene system which manages a collection of hierarchically-nested objects, each of which may have
 * one or more components providing functionality, interactivity, visuals, etc.
 */
class Scene final : public Destructible
{
public:
    // background light colour of the scene, passed to shaders
    glm::vec3 ambient_colour = { 0.01f, 0.01f, 0.01f };
    Ref<RenderGraph> render_graph; // render graph for the scene, used to record render commands
    Ref<Sky> sky;                  // controls how the sky(box)/backdrop is drawn

private:
    std::string origin;               // if not empty, contains the path from which this scene was loaded
    WeakRef<Scene> self;              // self reference for passing to objects
    Ref<Object> root;                 // invisible root object, acts as parent for all scene objects
    std::vector<Ref<Object>> objects; // all members of the scene
    glm::u32vec2 last_viewport_size;  // last known size of the viewport used to render the scene

public:
    DELETE_CONSTRUCTORS(Scene);
    /**
     * @brief creates a new empty scene with the specified name.
     * @param name name of the scene. usually unimportant.
     * @returns newly created scene.
     */
    static Ref<Scene> create(const std::string& name = "scene");
    ~Scene() override;

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }

    /**
     * @brief fetches a list of all objects which participate in the scene.
     * @returns list of references to scene objects, in no particular order.
     */
    std::vector<WeakRef<Object>> getAllObjects() const;
    /**
     * @brief searches list of objects for the first object with a matching name.
     * @param name object name to search for.
     * @returns reference to the first matching object, or `nullptr` if none was found.
     */
    WeakRef<Object> findObject(const std::string& name) const;
    /**
     * @brief adds an existing object to the scene, as a child of the root object. automatically removes the
     * object from whatever scene it is already present in if needed.
     * @param obj object to add.
     * @returns reference to `obj`.
     */
    Ref<Object> insertObject(Ref<Object> obj);
    /**
     * @brief adds a new object to the scene, constructing it and giving it a name.
     * @param name identifier which will be assigned to the new object, allowing it to be retrieved with
     * `findObject`.
     * @returns reference to the new object.
     */
    Ref<Object> addObject(const std::string& name);
    /**
     * @brief adds a new object to the scene, constructing it and giving it a name, and then adds a new
     * component of the specified type `T` to the newly created object. useful shortcut for setting up
     * scenes programmatically. you may only call this function with templates such that `T` inherits
     * from `Component`.
     * @param name identifier which will be assigned to the new object, allowing it to be retrieved with
     * `findObject`.
     * @returns reference to the component which was created on the new object.
     */
    template<class T> Ref<T> addObject(const std::string& name);
    /**
     * @brief removes the specified object from the scene. the object's parent will be cleared. `obj` should
     * be a member of the current scene.
     * @param obj object to remove.
     */
    void removeObject(Ref<Object> obj);

    glm::u32vec2 getViewportSize() const { return last_viewport_size; }
    /**
     * @brief performs a raycast, in world space, against the objects in the scene using their bounding box
     * data. a bounding box is ignored if the starting position is inside it. the object with the closest
     * intersection with the ray is returned.
     * @param position starting point of the ray in world space.
     * @param direction direction of the ray in world space.
     * @returns closest intersected object, or `nullptr`, if no objects were intersected.
     */
    WeakRef<Object> raycast(glm::vec3 position, glm::vec3 direction) const;

    /**
     * @brief called once per frame, and propagated to scene objects and their attached components.
     * @param delta_time time in seconds since the last frame.
     */
    void update(float delta_time);

    /**
     * @brief collects and executes scene drawing commands, via the internal render graph. if the scene does
     * not have a render graph attached, this does nothing.
     * @param command_buffer draw command buffer into which commands will be recorded.
     * @param viewport_size size of the viewport into which the scene is intended to be rendered. uniform
     * buffers and render graph attachments are updated automatically.
     */
    void draw(Ref<DrawCommandBuffer> command_buffer, glm::u32vec2 viewport_size);
    /**
     * @brief binds the render graph's passthrough material allowing the rendered scene to be copied to the
     * screen (or another render pass).
     * @param command_buffer draw command buffer into which the command will be recorded.
     */
    void bindOutputMaterial(Ref<DrawCommandBuffer> command_buffer);

    /**
     * @brief constructs a scene from a text-based serialised representation.
     * @param name path to the target file from which to read the text representation.
     * @returns scene constructed based on serialised representation, or `nullptr` if an error occurred
     * during deserialisation.
     */
    static Ref<Scene> deserialiseFile(const std::string& name);
    /**
     * @brief constructs a scene from a text-based serialised representation.
     * @param token_str text representation to decode.
     * @param origin original path from which the scene was loaded, may be empty.
     * @returns scene constructed based on serialised representation, or `nullptr` if an error occurred
     * during deserialisation.
     */
    static Ref<Scene> deserialise(const std::string& token_str, const std::string& origin = "");

    void drawImGuiDebug();

private:
    /**
     * @brief internal-use-only. use `Scene::create` instead.
     */
    Scene(const std::string& name);
};

template<class T> Ref<T> Scene::addObject(const std::string& name)
{
    static_assert(std::is_convertible_v<T*, Component*>,
        "component must be a HopEngine::Component subclass");
    auto obj = addObject(name);
    return obj->addComponent<T>();
}

} // namespace HopEngine
