#include "engine.h"

#include <chrono>
#include <imgui/imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

static Engine* engine = nullptr;

#if !defined(NDEBUG)
void Engine::registerCountedRef(const char* type_name, void* ptr, size_t* counter)
{
    if (engine->allocated_refs.contains(ptr))
        DBG_ERROR("a reference counted object was allocated, but it's raw pointer is already allocated! you probably used a raw pointer twice. this will lead to double-freeing.");
    else
        engine->allocated_refs[ptr] = { type_name, counter };
}

void Engine::unregisterCountedRef(void* ptr)
{
    if (!engine->allocated_refs.contains(ptr))
        DBG_ERROR("a reference counted object was deallocated, but it's raw pointer is not allocated! you probably used a raw pointer twice. we are about to double-free that pointer. i am praying for you.");
    else
        engine->allocated_refs.erase(ptr);
}

void HopEngine::registerCountedRef(const char* type_name, void* ptr, size_t* counter)
{
    Engine::registerCountedRef(type_name, ptr, counter);
}

void HopEngine::unregisterCountedRef(void* ptr)
{
    Engine::unregisterCountedRef(ptr);
}
#endif

void Engine::init()
{
	if (engine == nullptr)
		engine = new Engine();
}

void Engine::setup(void(* init_func)(Ref<Scene>), void(* _update_func)(Ref<Scene>, float), void(* _imgui_func)(Ref<Scene>, float))
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

            engine->imgui_func(engine->scene, delta.count());

            ImGui::Render();
        }
        RenderServer::draw(delta.count());
        if (engine->update_func)
            engine->update_func(engine->scene, delta.count());
    }
}

Ref<Scene> Engine::getScene()
{
    return engine->scene;
}

void Engine::summariseTrackedObjects()
{
#if !defined(NDEBUG)
    DBG_INFO("enumerating allocated objects (" + to_string(engine->allocated_refs.size()) + "):");
    for (const auto& pair : engine->allocated_refs)
        DBG_INFO("object " + PTR(pair.first) + ", with type '" + pair.second.first + "', has " + to_string(*pair.second.second) + " references");
#endif
}

void Engine::destroy()
{
	if (engine != nullptr)
		delete engine;
}

Engine::Engine()
{
    engine = this;
    Debug::init(Debug::DEBUG_FAULT);
    Package::init();
    Package::loadPackage("resources.hop");
    Window::initEnvironment();
    window = new Window(1024, 1024, "hop!");
    window->setIcon("res://icon.png");
    Input::init(window);
    RenderServer::init(window);
    Engine::summariseTrackedObjects();
    RenderServer::draw(0.0f);
    window->setVisible(true);
}

Engine::~Engine()
{
    scene = nullptr;

    Engine::summariseTrackedObjects();
    RenderServer::destroy();
    Package::destroy();
    Input::destroy();
    window = nullptr;
#if !defined(NDEBUG)
    if (allocated_refs.size() > 0)
    {
        DBG_ERROR("uh oh! there are objects still allocated! prepare for vulkan errors and possibly crashes! see below:");
        Engine::summariseTrackedObjects();
    }
    else
        DBG_INFO("well done for cleaning up!");
#endif
    Window::terminateEnvironment();
    Debug::close();

    engine = nullptr;
}
