#include "engine.h"

#include <chrono>

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

static Engine* engine = nullptr;

void Engine::init()
{
	if (engine == nullptr)
		engine = new Engine();
}

void Engine::setup(void(* init_func)(Ref<Scene>), void(* _update_func)(Ref<Scene>, float), void(* _imgui_func)())
{
    // TODO: make this a RenderServer func for easy access
    RenderServer::waitIdle();

    engine->render_server->draw_imgui_function = _imgui_func;
    engine->update_func = _update_func;
    engine->render_server->scene = new Scene();
    if (init_func)
        init_func(engine->render_server->scene);
}

void Engine::mainLoop()
{
    auto last_frame = chrono::steady_clock::now();

    while (!engine->window->getShouldClose())
    {
        auto this_frame = chrono::steady_clock::now();
        chrono::duration<float> delta = this_frame - last_frame;
        last_frame = this_frame;
        engine->window->pollEvents();
        if (engine->window->isMinified())
            continue;
        if (engine->window->isResized())
            engine->render_server->resizeSwapchain();
        engine->render_server->drawFrame(delta.count());
        engine->update_func(engine->render_server->scene, delta.count());
    }
}

void Engine::destroy()
{
	if (engine != nullptr)
		delete engine;
}

Engine::Engine()
{
    Debug::init(Debug::DEBUG_FAULT);
    Window::initEnvironment();
    window = new Window(1024, 1024, "hop!");
    Input::init(window->getWindow());
    Package::init();
    Package::loadPackage("resources.hop");
    render_server = new RenderServer(window);
}

Engine::~Engine()
{
    render_server->scene = nullptr;
    render_server = nullptr;

    window = nullptr;
    Window::terminateEnvironment();

    Debug::close();

    engine = nullptr;
}
