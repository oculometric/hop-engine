#pragma once

#include <map>
#include <vector>
#include <chrono>

#include "common.h"
#include "texture.h"
#include "events.h"

namespace HopEngine
{

struct FrameStats final
{
    float record_time = 0.0f;
    float render_time = 0.0f;
    float update_time = 0.0f;
    std::vector<float> pass_times;
    size_t draw_calls = 0;
    size_t pipeline_rebinds = 0;
    size_t triangles = 0;
    size_t passes = 0;
    size_t cameras = 0;
};

class Engine final
{
public:
    enum Events : EventServer::TypeID
    {
        EVENT_TYPE_INIT_FINISH        = 0x10000001,
        EVENT_TYPE_DESTROY_START      = 0x10000002,
        EVENT_TYPE_FRAME_BEGIN        = 0x10000003,
        EVENT_TYPE_FRAME_END          = 0x10000004,
        EVENT_TYPE_SCENE_CHANGE       = 0x10000005,
        EVENT_TYPE_APPLICATION_CHANGE = 0x10000006,
    };

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
	
	template<class T> static void startApplication();
	template<class T> static void switchApplication();
	static void stop();
    
	static void setScene(const Ref<Scene>& new_scene);
	static Ref<Scene> getScene();

	static float getDeltaTime()         { return getEngine()->delta_time; };
	static float getEngineTime()        { return getEngine()->total_time; }
	static float getSmoothedDeltaTime() { return getEngine()->smoothed_delta_time; }
	static float getSmoothedFPS()       { return getEngine()->smoothed_fps; }
	static size_t getFrameCount()       { return getEngine()->frame_index; }
	static FrameStats getFrameStats()   { return getEngine()->last_frame_stats; }
	
	static bool isWireframeMode()       { return getEngine()->wireframe_view; }
	static void setForceWireframe(bool value);
    
	static void debugSelect(const WeakRef<Object>& object);
	static WeakRef<Object> getDebugSelection();
	static void debugCamera(const WeakRef<Object>& selected_camera);

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
	void updateStats(const FrameStats& stats);
	static void summariseTrackedObjects();
	
	void _drawImGuiDebug(float delta_time) const;
};

template <class T>
inline void Engine::startApplication()
{
	static_assert(std::is_convertible_v<T*, Application*>, "T must be a HopEngine::Application subclass");
	if (getEngine()->start_called)
	{
		DBG_WARNING("an application is already running. did you mean to call switchApplication?");
        return;
	}
	getEngine()->application = new T();
    EventServer::dispatch(EVENT_TYPE_APPLICATION_CHANGE);
	Engine::start();
}

template <class T>
inline void Engine::switchApplication()
{
	static_assert(std::is_convertible_v<T*, Application*>, "T must be a HopEngine::Application subclass");
	getEngine()->next_application = new T();
    EventServer::dispatch(EVENT_TYPE_APPLICATION_CHANGE);
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
