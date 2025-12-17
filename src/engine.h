#pragma once

#include <map>
#include <vector>

#include "common.h"

namespace HopEngine
{

class Engine
{
private:
	Ref<Scene> scene;
	Ref<Window> window;
	void(* update_func)(Ref<Scene>, float) = nullptr;
	void(* imgui_func)(Ref<Scene>, float) = nullptr;

	std::multimap<const char*, WeakRef<void>> allocated_refs;
	std::vector<Ref<void>> keep_loaded_refs;

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

private:
	Engine();
	~Engine();

	static void _keepLoaded(Ref<void> ref);
	static std::vector<WeakRef<void>> getRefsWithType(const char* type_name);
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
