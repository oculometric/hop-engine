#pragma once

#include <map>
#include <vector>

#include "common.h"

namespace HopEngine
{

struct FrameStats
{
	float imgui_time = 0.0f;
	float build_time = 0.0f;
	float record_time = 0.0f;
	float render_time = 0.0f;
	std::vector<float> pass_times;
	float update_time = 0.0f;
	float delta_time = 0.0f;
	size_t draw_calls = 0;
	size_t pipeline_rebinds = 0;
	size_t triangles = 0;
	size_t vertices = 0;
	size_t passes = 0;
	size_t cameras = 0;
	size_t lights = 0;
};

class Engine
{
private:
	Ref<Scene> scene;
	Ref<Window> window;
	void(* update_func)(Ref<Scene>, float) = nullptr;
	void(* imgui_func)(Ref<Scene>, float) = nullptr;

	std::multimap<const char*, WeakRef<void>> allocated_refs;
	std::map<std::string, Ref<Shader>> loaded_shaders;
	std::map<std::string, Ref<Material>> loaded_materials;
	std::map<std::string, Ref<Texture>> loaded_textures;
	std::map<std::string, Ref<Mesh>> loaded_meshes;
	std::vector<Ref<Destructible>> keep_loaded_refs;

	FrameStats last_frame_stats;
	float smoothed_delta_time = 0.0f;
	float smoothed_fps = 0.0f;
	float delta_time_history[512];
	float fps_history[512];
	int history_offset = 0;
	
	bool wireframe_view = false;
	
	bool stop_requested = false;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Engine);

	static void init();
	static void destroy();
	static void stop();

	static void setup(Ref<Scene>(* init_func)(), void(* _update_func)(Ref<Scene>, float), void(* _imgui_func)(Ref<Scene>, float));
	static void mainLoop();
	static Ref<Scene> getScene();
	static void summariseTrackedObjects();
	static FrameStats getFrameStats();
	static float getSmoothedDeltaTime();
	static float getSmoothedFPS();
	static bool isWireframeMode();
	template <class T> static std::vector<WeakRef<T>> getAllRefs();
	static void setForceWireframe(bool value);
	static void registerCountedRef(const char* type_name, const WeakRef<void>& reference);
	static void unregisterCountedRef(const void* ptr);
	template <class T> static Ref<T> keepLoaded(Ref<T> ref);
	template <class T> static Ref<T> keepLoaded(T* ref) { return keepLoaded(Ref<T>(ref)); }

	static void debugCamera(float delta_time);
	static void debugSelect(const WeakRef<Object>& object);
	static void debugClearSelection(const WeakRef<Object>& object = WeakRef<Object>(), const WeakRef<Material>& material = WeakRef<Material>(), WeakRef<Camera> camera = WeakRef<Camera>());
	static WeakRef<Object> getDebugSelection();
	
	static Ref<Shader> loadShader(const std::string& path);
	static Ref<Material> loadMaterial(const std::string& path);
	static Ref<Texture> loadTexture(const std::string& path);
	static Ref<Texture> loadTexture3D(const std::string& path, int layers_wide, int layers_high);
	static Ref<Mesh> loadMesh(const std::string& path);
	static size_t pruneUnusedResources();
	static void drawImGuiDebug(float delta_time);
	
private:
	Engine();
	~Engine();
	
	static Engine* getEngine();
	static std::vector<WeakRef<void>> getRefsWithType(const char* type_name);
	static void _keepLoaded(const Ref<Destructible>& ref);
	void updateStats(const FrameStats& stats);
	
	void _drawImGuiDebug(float delta_time) const;
};

template<class T>
std::vector<WeakRef<T>> Engine::getAllRefs()
{
	auto refs = getRefsWithType(typeid(T).name());
	std::vector<WeakRef<T>> cast_refs;
	for (auto& r : refs)
		cast_refs.push_back(r.cast<T>());
	return cast_refs;
}

template<class T>
Ref<T> Engine::keepLoaded(Ref<T> ref)
{
	static_assert(std::is_convertible_v<T*, Destructible*>, "reference must be a HopEngine::Destructible subclass");
	_keepLoaded(ref.template cast<Destructible>());
	return ref;
}

}
