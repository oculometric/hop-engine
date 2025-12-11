#include "engine.h"

#include <chrono>
#include <imgui/imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

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
    RenderServer::waitIdle();

    engine->imgui_func = _imgui_func;
    engine->update_func = _update_func;
    engine->scene = new Scene();
    if (init_func)
        init_func(engine->scene);
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
            RenderServer::resize();
        if (engine->imgui_func)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            engine->imgui_func();

            ImGui::Render();
        }
        RenderServer::draw(delta.count());
        engine->update_func(engine->scene, delta.count());
    }
}

Ref<Scene> Engine::getScene()
{
    return engine->scene;
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
    Input::init(window);
    Package::init();
    Package::loadPackage("resources.hop");
    RenderServer::init(window);
}

Engine::~Engine()
{
    scene = nullptr;

    RenderServer::destroy();
    Package::destroy();
    Input::destroy();
    window = nullptr;
    Window::terminateEnvironment();
    Debug::close();

    engine = nullptr;
}
