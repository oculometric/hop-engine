#pragma once

#include <vector>
#include <map>

#include "common.h"
#include "object.h"
#include "draw_command.h"

namespace HopEngine
{

struct FrameStats;

class Scene : public Destructible
{
public:
	glm::vec3 ambient_colour = { 0.01f, 0.01f, 0.01f };
	Ref<Texture> skybox;
	Ref<RenderGraph> render_graph;

private:
	std::string origin;
	WeakRef<Scene> self;
	Ref<Object> root;
	std::vector<Ref<Object>> objects;
	Ref<Camera> backup_camera;
	std::map<size_t, Ref<Camera>> cameras;
	std::vector<Ref<Light>> lights;
	glm::u32vec2 last_viewport_size;

public:
	DELETE_CONSTRUCTORS(Scene);
	static Ref<Scene> create(const std::string& name = "scene");
	~Scene() override;
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	std::vector<WeakRef<Object>> getAllObjects() const;
	Ref<Object> findObject(const std::string& name) const;
	template<class T> Ref<T> findObject(const std::string& name) const;
	Ref<Object> insertObject(Ref<Object> obj);
	template<class T> Ref<T> insertObject(Ref<T> obj);
	void removeObject(Ref<Object> obj);
	
	glm::u32vec2 getViewportSize() const { return last_viewport_size; }
	Ref<Camera> getCamera(size_t slot) const;
	std::vector<LightParams> getLightParams() const;
	std::vector<DrawCommand> getDrawCommands() const;
	WeakRef<Object> raycast(glm::vec3 position, glm::vec3 direction) const;
	void setCameraSlot(const Ref<Camera>& camera, size_t slot);
	void updateUniforms(uint32_t image_index, float time_since_start, glm::u32vec2 viewport_size, FrameStats& stats);
	
	static Ref<Scene> deserialise(const std::string& name);
	
	void drawImGuiDebug();
	
private:
	Scene(const std::string& name);
};

template <class T>
Ref<T> Scene::findObject(const std::string& name) const
{
	static_assert(std::is_convertible_v<T*, Object*>, "expected type must be a HopEngine::Object subclass");
	Ref<Object> obj = findObject(name);
	if (obj)
		return obj.cast<T>();
	return nullptr;
}

template <class T>
Ref<T> Scene::insertObject(Ref<T> obj)
{
	static_assert(std::is_convertible_v<T*, Object*>, "object must be a HopEngine::Object subclass");
	insertObject(obj.template cast<Object>()).cast<T>();
	return obj;
}

}
