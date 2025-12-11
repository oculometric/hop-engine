#pragma once

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

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Engine);

	static void init();
	static void destroy();

	static void setup(void(* init_func)(Ref<Scene>), void(* update_func)(Ref<Scene>, float), void(* imgui_func)());
	static void mainLoop();
	static Ref<Scene> getScene();

private:
	Engine();
	~Engine();
};

}
