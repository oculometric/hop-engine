#pragma once

#include <map>
#include <vector>
#include <chrono>

#include "common.h"
#include "texture.h"
#include "frame_stats.h"

namespace HopEngine
{

class Application;

class Engine final
{
private:
	Ref<Scene> scene;
	Ref<Application> application;
	Ref<Application> next_application;

	std::multimap<const char*, WeakRef<void>> allocated_refs;
	std::map<std::string, Ref<Shader>> loaded_shaders;
	std::map<std::string, Ref<Material>> loaded_materials;
	std::map<std::string, Ref<Texture>> loaded_textures;
	std::map<std::string, Ref<Mesh>> loaded_meshes;
	std::map<Sampler::Builder, Ref<Sampler>> premade_samplers;
	std::vector<Ref<Destructible>> keep_loaded_refs;

	FrameStats last_frame_stats;
	std::chrono::steady_clock::time_point engine_start_timestamp;
	float delta_time = 0.0f;
	float total_time = 0.0f;
	float smoothed_delta_time = 0.0f;
	float smoothed_fps = 0.0f;
	size_t frame_index = 0;
	float delta_time_history[200];
	int history_offset = 0;
	
	bool wireframe_view = false;
	
	bool stop_requested = false;
	bool start_called = false;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Engine);

	static void init();
	static void destroy();
	
	template<class T> static void runApplication();
	template<class T> static void switchApplication();
	static void setScene(const Ref<Scene>& new_scene);
	static void stop();

	static Ref<Scene> getScene();

	static FrameStats getFrameStats();
	static float getDeltaTime();
	static float getEngineTime();
	static float getSmoothedDeltaTime();
	static float getSmoothedFPS();
	static size_t getFrameCount();
	
	static bool isWireframeMode();
	static void setForceWireframe(bool value);

	static void debugCamera();
	static void debugSelect(const WeakRef<Object>& object);
	static void debugClearSelection(const WeakRef<Object>& object = WeakRef<Object>(), const WeakRef<Material>& material = WeakRef<Material>(), WeakRef<CameraComponent> camera = WeakRef<CameraComponent>());
	static WeakRef<Object> getDebugSelection();
	
	static Ref<Shader> loadShader(const std::string& path);
	static Ref<Material> loadMaterial(const std::string& path);
	static Ref<Texture> loadTexture(const std::string& path);
	static Ref<Texture> loadTexture3D(const std::string& path, int layers_wide, int layers_high);
	static Ref<Mesh> loadMesh(const std::string& path);
	static Ref<Sampler> makeSampler(const Sampler::Builder& builder);
	static size_t pruneUnusedResources();
	template <class T> static std::vector<WeakRef<T>> getAllRefs();
	static void registerCountedRef(const char* type_name, const WeakRef<void>& reference);
	static void unregisterCountedRef(const void* ptr);

	static void drawImGuiDebug();
	
private:
	Engine();
	~Engine();
	
	static void start();
	static Engine* getEngine();
	static std::vector<WeakRef<void>> getRefsWithType(const char* type_name);
	static void _keepLoaded(const Ref<Destructible>& ref);
	void updateStats(const FrameStats& stats);
	static void summariseTrackedObjects();
	
	void _drawImGuiDebug(float delta_time) const;
};

template <class T>
inline void Engine::runApplication()
{
	if (engine->start_called)
	{
		DBG_WARNING("an application is already running. did you mean to call switchApplication?");
        return;
	}
	application = new T();
	Engine::start();
}

template <class T>
inline void Engine::switchApplication()
{
	next_application = new T();
}

template <class T>
std::vector<WeakRef<T>> Engine::getAllRefs()
{
	auto refs = getRefsWithType(typeid(T).name());
	std::vector<WeakRef<T>> cast_refs;
	for (auto& r : refs)
		cast_refs.push_back(r.cast<T>());
	return cast_refs;
}

class Application : public Destructible
{
public:
	DELETE_NOT_ALL_CONSTRUCTORS(Application);
	Application() = default;
	~Application() = default;

	virtual void update(float delta_time) { }
	virtual void drawImGui() { }
};

}
