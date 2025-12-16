#pragma once

#include <map>

#include "common.h"

namespace HopEngine
{

class Engine
{
private:
	Ref<Scene> scene;
	Ref<Window> window;
	void(* update_func)(Ref<Scene>, float) = nullptr;
	void(* imgui_func)() = nullptr;

#if !defined(NDEBUG)
	std::map<void*, std::pair<const char*, size_t*>> allocated_refs;
#endif

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Engine);

	static void init();
	static void destroy();

	static void setup(void(* init_func)(Ref<Scene>), void(* update_func)(Ref<Scene>, float), void(* imgui_func)());
	static void mainLoop();
	static Ref<Scene> getScene();
	static void summariseTrackedObjects();

	static void registerCountedRef(const char* type_name, void* ptr, size_t* counter);
	static void unregisterCountedRef(void* ptr);

private:
	Engine();
	~Engine();
};

}
