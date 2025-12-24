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

void Engine::init()
{
    if (engine == nullptr)
        engine = new Engine();
}

void Engine::destroy()
{
    if (engine != nullptr)
        delete engine;
}

void Engine::stop()
{
    engine->stop_requested = true;
}

void Engine::setup(Ref<Scene>(* init_func)(), void(* _update_func)(Ref<Scene>, float), void(* _imgui_func)(Ref<Scene>, float))
{
    RenderServer::waitIdle();

    engine->imgui_func = _imgui_func;
    engine->update_func = _update_func;
    if (init_func)
        engine->scene = init_func();
}

void Engine::mainLoop()
{
    auto last_frame = chrono::steady_clock::now();

    while (!engine->window->getShouldClose() && !engine->stop_requested)
    {
        auto this_frame = chrono::steady_clock::now();
        chrono::duration<float> delta = this_frame - last_frame;
        last_frame = this_frame;
        engine->window->pollEvents();
        Input::pollGamepads();
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
            engine->drawImGuiDebug(delta.count());

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

void Engine::registerCountedRef(const char* type_name, WeakRef<void> reference)
{
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

void HopEngine::registerCountedRef(const char* type_name, WeakRef<void> reference)
{
    Engine::registerCountedRef(type_name, reference);
}

void HopEngine::unregisterCountedRef(void* ptr)
{
    Engine::unregisterCountedRef(ptr);
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

Ref<Shader> Engine::loadShader(const string& path)
{
    const auto it = engine->loaded_shaders.find(path);
    if (it == engine->loaded_shaders.end())
    {
        Ref<Shader> thing = new Shader(path, false);
        engine->loaded_shaders[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused shader '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Material> Engine::loadMaterial(const string& path)
{
    const auto it = engine->loaded_materials.find(path);
    if (it == engine->loaded_materials.end())
    {
        Ref<Material> thing = Material::deserialise(path);
        engine->loaded_materials[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused material '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Texture> Engine::loadTexture(const string& path)
{
    const auto it = engine->loaded_textures.find(path);
    if (it == engine->loaded_textures.end())
    {
        Ref<Texture> thing = new Texture(path);
        engine->loaded_textures[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused texture '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Texture> Engine::loadTexture3D(const string& path, int layers_wide, int layers_high)
{
    const auto it = engine->loaded_textures.find(path);
    if (it == engine->loaded_textures.end())
    {
        Ref<Texture> thing = new Texture(path, TextureBuilder().layers({ static_cast<uint32_t>(layers_wide), static_cast<uint32_t>(layers_high) }));
        engine->loaded_textures[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused texture 3D '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Mesh> Engine::loadMesh(const string& path)
{
    const auto it = engine->loaded_meshes.find(path);
    if (it == engine->loaded_meshes.end())
    {
        Ref<Mesh> thing = new Mesh(path);
        engine->loaded_meshes[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused mesh '" + path + "' instead of duplicating");
    return it->second;
}

size_t Engine::pruneUnusedResources()
{
    DBG_INFO("pruning currently loaded unused resources...");
    size_t pruned_refs = 0;
    auto mesh_it = engine->loaded_meshes.begin();
    while (mesh_it != engine->loaded_meshes.end())
    {
        if (mesh_it->second.getCount() == 1)
        {
            auto next = mesh_it;
            ++next;
            engine->loaded_meshes.erase(mesh_it);
            ++pruned_refs;
            mesh_it = next;
        }
        else
            ++mesh_it;
    }
    auto mat_it = engine->loaded_materials.begin();
    while (mat_it != engine->loaded_materials.end())
    {
        if (mat_it->second.getCount() == 1)
        {
            auto next = mat_it;
            ++next;
            engine->loaded_materials.erase(mat_it);
            ++pruned_refs;
            mat_it = next;
        }
        else
            ++mat_it;
    }
    auto shr_it = engine->loaded_shaders.begin();
    while (shr_it != engine->loaded_shaders.end())
    {
        if (shr_it->second.getCount() == 1)
        {
            auto next = shr_it;
            ++next;
            engine->loaded_shaders.erase(shr_it);
            ++pruned_refs;
            shr_it = next;
        }
        else
            ++shr_it;
    }
    auto tex_it = engine->loaded_textures.begin();
    while (tex_it != engine->loaded_textures.end())
    {
        if (tex_it->second.getCount() == 1)
        {
            auto next = tex_it;
            ++next;
            engine->loaded_textures.erase(tex_it);
            ++pruned_refs;
            tex_it = next;
        }
        else
            ++tex_it;
    }
    DBG_INFO("pruned " + to_string(pruned_refs) + " total reference counted objects");
    return pruned_refs;
}

Engine* Engine::getEngine()
{
    return engine;
}

void Engine::_keepLoaded(Ref<Destructible> ref)
{
    engine->keep_loaded_refs.push_back(ref);
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
    loaded_shaders.clear();
    loaded_materials.clear();
    loaded_textures.clear();
    loaded_meshes.clear();

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
