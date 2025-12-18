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
	std::vector<Ref<void>> keep_loaded_refs;

	FrameStats last_frame_stats;
	float smoothed_delta_time;
	float smoothed_fps;
	float delta_time_history[512];
	float fps_history[512];
	int history_offset = 0;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Engine);

	static void init();
	static void destroy();

	static void setup(void(* init_func)(Ref<Scene>), void(* update_func)(Ref<Scene>, float), void(* imgui_func)(Ref<Scene>, float));
	static void mainLoop();
	static Ref<Scene> getScene();
	static void summariseTrackedObjects();

	static void registerCountedRef(const char* type_name, WeakRef<void> reference);
	static void unregisterCountedRef(void* ptr);
	template <class T>
	static std::vector<WeakRef<T>> getAllRefs();
	template <class T>
	static Ref<T> keepLoaded(Ref<T> ref);
	template <class T>
	static inline Ref<T> keepLoaded(T* ref) { return keepLoaded(Ref<T>(ref)); }

	static FrameStats getFrameStats();
	static float getSmoothedDeltaTime();
	static float getSmoothedFPS();
	static void drawImGuiDebug(float delta_time);
	static void debugCamera(float delta_time);
	static void debugClearSelection(WeakRef<Object> object = WeakRef<Object>(), WeakRef<Material> material = WeakRef<Material>());

private:
	Engine();
	~Engine();

	static void _keepLoaded(Ref<void> ref);
	static std::vector<WeakRef<void>> getRefsWithType(const char* type_name);
	void updateStats(FrameStats stats);
	void _drawImGuiDebug(float delta_time);
};

template<class T>
inline std::vector<WeakRef<T>> Engine::getAllRefs()
{
	auto refs = getRefsWithType(typeid(T).name());
	std::vector<WeakRef<T>> cast_refs;
	for (auto& r : refs)
		cast_refs.push_back(r.cast<T>());
	return cast_refs;
}

template<class T>
inline Ref<T> Engine::keepLoaded(Ref<T> ref)
{
	_keepLoaded(ref.cast<void>());
	return ref;
}

}
