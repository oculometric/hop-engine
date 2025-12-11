#pragma once

#include "common.h"

namespace HopEngine
{

class Engine
{
private:
	Ref<Window> window;
	Ref<RenderServer> render_server;
	void(* update_func)(Ref<Scene>, float);

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Engine);

	// TODO: make it so that this class manages all the other singleton classes internally
	// TODO: move static get() functions into here?
	// TODO: make destructors for other singletons private
	// TODO: move RenderServer scene into here

	static void init();
	static void setup(void(* init_func)(Ref<Scene>), void(* update_func)(Ref<Scene>, float), void(* imgui_func)());
	static void mainLoop();
	static void destroy();

private:
	Engine();
	~Engine();
};

}
