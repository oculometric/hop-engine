#pragma once

#include "common.h"
#include "math_helpers.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <map>
#include <vector>

namespace HopEngine
{

struct Transform final
{
    friend class Object;

private:
    Object* owner = nullptr;
    glm::vec3 local_position;
    glm::vec3 local_euler;
    glm::vec3 local_scale;
    glm::mat4 local_matrix;
    glm::mat4 world_matrix;

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
    glm::vec3 getEuler() const; // TODO:
    glm::mat4 getMatrix()
    {
        worldFromLocal();
        return world_matrix;
    }
    glm::vec3 right() const { return world_matrix[0]; }    // represents world space X axis
    glm::vec3 up() const { return world_matrix[1]; }       // represents world space Y axis
    glm::vec3 forward() const { return -world_matrix[2]; } // represents world space -Z axis

    void setLocalPosition(glm::vec3 position);
    void setLocalEuler(glm::vec3 euler);
    void setLocalScale(glm::vec3 scale);
    void setPosition(glm::vec3 position);
    void setEuler(glm::vec3 euler); // TODO:
    void setMatrix(const glm::mat4& matrix);

    void translateLocal(glm::vec3 offset);
    void rotateLocal(glm::vec3 degrees);
    void scaleLocal(glm::vec3 factor);
    void scaleLocal(float factor);
    void translate(glm::vec3 offset);
    void rotate(glm::vec3 axis, float degrees); // TODO:
    void rotate(glm::vec3 degrees);
    void scale(float factor);
    void lookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up);

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

class Component : public Destructible
{
    friend class Object;

private:
    WeakRef<Object> owner;
    bool enabled = true;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Component);
    Component()           = default;
    ~Component() override = default;

    WeakRef<Object> getOwner() const { return owner; }
    template<class T> WeakRef<T> getComponent();
    WeakRef<Scene> getScene() const;
    Transform& getTransform() const;
    bool getEnabled() const { return enabled; }
    void setEnabled(bool state) { enabled = state; }

    virtual void awake() {}
    virtual void update(float delta_time) {}

    virtual std::vector<DrawCommand> getDrawCommands() { return {}; }
    virtual BoundingBox getLocalBounds() const { return BoundingBox{}; }

    virtual void drawImGuiDebug();
};

class Object final : public Destructible
{
    friend class Scene;

public:
    std::string name = "object";
    Transform transform;

private:
    WeakRef<Object> self;
    WeakRef<Scene> scene;
    WeakRef<Object> parent;
    std::vector<Ref<Object>> children;
    std::vector<Ref<Component>> components;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Object);
    static Ref<Object> create();
    ~Object() override {}

    WeakRef<Object> getSelf() const { return self; }
    WeakRef<Scene> getScene() const { return scene; }
    WeakRef<Object> getParent() const { return parent; }
    size_t getChildCount() const { return children.size(); }
    WeakRef<Object> getChild(size_t index) const { return children[index]; }
    void removeFromParent();
    void addChild(WeakRef<Object> obj);

    template<class T> WeakRef<T> addComponent()
    {
        static_assert(std::is_convertible_v<T*, Component*>,
            "component must be a HopEngine::Component subclass");
        Ref<T> comp = new T();
        comp->owner = self;
        components.push_back(comp.template cast<Component>());
        comp->awake();
        return comp.weak();
    }
    template<class T> WeakRef<T> getComponent()
    {
        static_assert(std::is_convertible_v<T*, Component*>,
            "component must be a HopEngine::Component subclass");
        for (auto& comp : components)
        {
            if (dynamic_cast<T*>(comp.get())) return comp.template cast<T>();
        }
        return WeakRef<T>();
    }
    template<class T> bool removeComponent()
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

    void update(float delta_time);
    std::vector<DrawCommand> getDrawCommands();
    BoundingBox getLocalBounds() const;

    void drawImGuiDebug();

private:
    Object();
};

template<class T> WeakRef<T> Component::getComponent() { return owner->getComponent<T>(); }

class Scene final : public Destructible
{
public:
    glm::vec3 ambient_colour = { 0.01f, 0.01f, 0.01f };
    Ref<RenderGraph> render_graph;

private:
    std::string origin;
    WeakRef<Scene> self;
    Ref<Object> root;
    std::vector<Ref<Object>> objects;
    glm::u32vec2 last_viewport_size;
    Ref<Material> skybox_material;
    Ref<UniformBlock> skybox_uniforms;
    WeakRef<Texture> skybox;

public:
    DELETE_CONSTRUCTORS(Scene);
    static Ref<Scene> create(const std::string& name = "scene");
    ~Scene() override;

    std::string getOrigin() const
    {
        if (this == nullptr) return "0x0";
        return origin.empty() ? PTR(this) : origin;
    }

    std::vector<WeakRef<Object>> getAllObjects() const;
    WeakRef<Object> findObject(const std::string& name) const;
    WeakRef<Object> insertObject(WeakRef<Object> obj);
    WeakRef<Object> addObject(const std::string& name);
    template<class T> WeakRef<T> addObject(const std::string& name)
    {
        static_assert(std::is_convertible_v<T*, Component*>,
            "component must be a HopEngine::Component subclass");
        auto obj = addObject(name);
        return obj->addComponent<T>();
    }
    void removeObject(WeakRef<Object> obj);

    glm::u32vec2 getViewportSize() const { return last_viewport_size; }
    WeakRef<Object> raycast(glm::vec3 position, glm::vec3 direction) const;

    void setSkybox(WeakRef<Texture> texture);

    void update(float delta_time);
    void draw(Ref<DrawCommandBuffer> command_buffer, glm::u32vec2 viewport_size);
    void bindOutputMaterial(Ref<DrawCommandBuffer> command_buffer);

    static Ref<Scene> deserialiseFile(const std::string& name);
    static Ref<Scene> deserialise(const std::string& token_str, const std::string& origin = "");

    void drawImGuiDebug();

private:
    Scene(const std::string& name);
};

} // namespace HopEngine
