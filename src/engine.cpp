#include "engine.h"

#include <chrono>
#include <imgui/imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <format>

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

static Engine* engine = nullptr;

void Engine::registerCountedRef(const char* type_name, WeakRef<void> reference)
{
    /*if (engine->allocated_refs.contains(ptr))
        DBG_ERROR("a reference counted object was allocated, but it's raw pointer is already allocated! you probably used a raw pointer twice. this will lead to double-freeing.");
    else
        engine->allocated_refs[ptr] = { type_name, counter };*/
    engine->allocated_refs.insert({ type_name, reference });
}

void Engine::unregisterCountedRef(void* ptr)
{
    for (auto pair = engine->allocated_refs.begin(); pair != engine->allocated_refs.end(); ++pair)
    {
        if (pair->second.get() == ptr)
        {
            engine->allocated_refs.erase(pair);
            return;
        }
    }
    DBG_ERROR("a reference counted object was deallocated, but it's raw pointer is not allocated! you probably used a raw pointer twice. we are about to double-free that pointer. i am praying for you.");
}

void Engine::_keepLoaded(Ref<Destructible> ref)
{
    engine->keep_loaded_refs.push_back(ref);
}

void HopEngine::registerCountedRef(const char* type_name, WeakRef<void> reference)
{
    Engine::registerCountedRef(type_name, reference);
}

void HopEngine::unregisterCountedRef(void* ptr)
{
    Engine::unregisterCountedRef(ptr);
}

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
        auto imgui_start = chrono::steady_clock::now();
        if (engine->imgui_func)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            RenderServer::waitIdle();
            engine->imgui_func(engine->scene, delta.count());

            ImGui::Render();
        }
        chrono::duration<float> imgui_duration = chrono::steady_clock::now() - imgui_start;
        FrameStats stats = RenderServer::draw();
        auto update_start = chrono::steady_clock::now();
        if (engine->update_func)
            engine->update_func(engine->scene, delta.count());
        chrono::duration<float> update_duration = chrono::steady_clock::now() - update_start;
        stats.imgui_time = imgui_duration.count();
        stats.update_time = update_duration.count();
        static auto last_frame_end = chrono::steady_clock::now();
        chrono::duration<float> frame_delta = chrono::steady_clock::now() - last_frame_end;
        last_frame_end = chrono::steady_clock::now();
        stats.delta_time = frame_delta.count();
        engine->updateStats(stats);
    }
}

Ref<Scene> Engine::getScene()
{
    return engine->scene;
}

void Engine::summariseTrackedObjects()
{
    DBG_INFO("enumerating allocated objects (" + to_string(engine->allocated_refs.size()) + "):");
    for (auto& pair : engine->allocated_refs)
        DBG_INFO("object " + PTR(pair.second.get()) + ", with type '" + pair.first + '\'');
}

void Engine::destroy()
{
	if (engine != nullptr)
		delete engine;
}

FrameStats Engine::getFrameStats()
{
    return engine->last_frame_stats;
}

float Engine::getSmoothedDeltaTime()
{
    return engine->smoothed_delta_time;
}

float Engine::getSmoothedFPS()
{
    return engine->smoothed_fps;
}

void Engine::drawImGuiDebug(float delta_time)
{
    engine->_drawImGuiDebug(delta_time);
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
    RenderServer::draw();
    window->setVisible(true);
}

Engine::~Engine()
{
    scene = nullptr;
    keep_loaded_refs.clear();

    RenderServer::destroy();
    Package::destroy();
    Input::destroy();
    window = nullptr;
    if (allocated_refs.size() > 0)
    {
        DBG_ERROR("uh oh! there are objects still allocated! prepare for vulkan errors and possibly crashes! see below:");
        Engine::summariseTrackedObjects();
    }
    else
        DBG_INFO("good girl for cleaning up!");
    Window::terminateEnvironment();
    Debug::close();

    engine = nullptr;
}

vector<WeakRef<void>> Engine::getRefsWithType(const char* type_name)
{
    vector<WeakRef<void>> refs;
    auto range = engine->allocated_refs.equal_range(type_name);
    while (range.first != range.second)
    {
        refs.push_back(range.first->second);
        ++range.first;
    }
    return refs;
}

void Engine::updateStats(FrameStats stats)
{
    last_frame_stats = stats;

    smoothed_delta_time = (smoothed_delta_time * 0.5f) + (stats.delta_time * 0.5f);
    float fps = 1.0f / stats.delta_time;
    smoothed_fps = (smoothed_fps * 0.5f) + (fps * 0.5f);

    delta_time_history[history_offset] = stats.delta_time;
    fps_history[history_offset] = 1.0f / stats.delta_time;
    history_offset = (history_offset + 1) % 512;

    engine->window->setTitle(format("hop-engine   -   {:>4.2f}ms   -   {:>6.2f} fps", smoothed_delta_time * 1000.0f, smoothed_fps));
}
