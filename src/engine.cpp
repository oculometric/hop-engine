#include "engine.h"

#include <chrono>
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <format>
#if defined(_WIN32)
#include <Windows.h>
#endif

#include "../resource.h"
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
{ delete engine; }

void Engine::stop()
{ engine->stop_requested = true; }

void Engine::setup(void(* _update_func)(Ref<Scene>, float), void(* _imgui_func)(Ref<Scene>))
{
    RenderServer::waitIdle();

    engine->imgui_func = _imgui_func;
    engine->update_func = _update_func;
}

void Engine::mainLoop()
{
    auto last_frame = chrono::steady_clock::now();

    while (!RenderServer::getWindowShouldClose() && !engine->stop_requested)
    {
        auto this_frame = chrono::steady_clock::now();
        chrono::duration<float> delta = this_frame - last_frame;
        last_frame = this_frame;
        engine->delta_time = delta.count();
        chrono::duration<float> since_start = this_frame - engine->engine_start_timestamp;
        engine->total_time = since_start.count();

        Input::pollInput();
        auto imgui_start = chrono::steady_clock::now();
        if (engine->imgui_func)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            RenderServer::waitIdle();
            engine->imgui_func(engine->scene);

            ImGui::Render();
        }
        chrono::duration<float> imgui_duration = chrono::steady_clock::now() - imgui_start;
        FrameStats stats = RenderServer::draw();
        auto update_start = chrono::steady_clock::now();
        if (engine->update_func)
            engine->update_func(engine->scene, getDeltaTime());
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

void Engine::setScene(const Ref<Scene>& new_scene)
{
    Engine::debugClearSelection();
    engine->scene = new_scene;
    RenderServer::setSingleScene(new_scene);
}

void Engine::summariseTrackedObjects()
{
    DBG_INFO("enumerating allocated objects (" + ::to_string(engine->allocated_refs.size()) + "):");
    for (auto& [type_name, ptr] : engine->allocated_refs)
        DBG_INFO("object " + PTR(ptr.get()) + ", with type '" + type_name + '\'');
}

void Engine::registerCountedRef(const char* type_name, const WeakRef<void>& reference)
{ engine->allocated_refs.insert({ type_name, reference }); }

void Engine::unregisterCountedRef(const void* ptr)
{
    for (auto pair = engine->allocated_refs.begin(); pair != engine->allocated_refs.end(); ++pair)
    {
        if (pair->second.get() == ptr)
        {
            engine->allocated_refs.erase(pair);
            return;
        }
    }
    DBG_ERROR("a reference counted object was deallocated, but its raw pointer is not allocated! you probably used a raw pointer twice. we are about to double-free that pointer. fuck you.");
}

void HopEngine::registerCountedRef(const char* type_name, const WeakRef<void>& reference)
{ Engine::registerCountedRef(type_name, reference); }

void HopEngine::unregisterCountedRef(const void* ptr)
{ Engine::unregisterCountedRef(ptr); }

FrameStats Engine::getFrameStats()
{ return engine->last_frame_stats; }

float Engine::getDeltaTime()
{ return engine->delta_time; }

float Engine::getEngineTime()
{ return engine->total_time; }

float Engine::getSmoothedDeltaTime()
{ return engine->smoothed_delta_time; }

float Engine::getSmoothedFPS()
{ return engine->smoothed_fps; }

bool Engine::isWireframeMode()
{ return engine->wireframe_view; }

void Engine::setForceWireframe(const bool value)
{ engine->wireframe_view = value; }

Ref<Shader> Engine::loadShader(const string& path)
{
    const auto it = engine->loaded_shaders.find(path);
    if (it == engine->loaded_shaders.end())
    {
        Ref<Shader> thing = new Shader(path);
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

Ref<Texture> Engine::loadTexture3D(const string& path, const int layers_wide, const int layers_high)
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

Ref<Sampler> Engine::makeSampler(const SamplerBuilder& builder)
{
    return engine->premade_samplers[builder];
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
    DBG_INFO("pruned " + ::to_string(pruned_refs) + " total reference counted objects");
    return pruned_refs;
}

void Engine::drawImGuiDebug()
{
    engine->_drawImGuiDebug(getDeltaTime());
}

Engine::Engine()
{
    engine = this;

    engine->engine_start_timestamp = chrono::steady_clock::now();

    Debug::init(DEBUG_FAULT);
    Package::init();

// #if defined(_WIN32)
//     const HRSRC res = FindResource(nullptr, MAKEINTRESOURCE(IDR_HOP1), L"HOP");
//     const DWORD size = SizeofResource(nullptr, res);
//     const HGLOBAL data = LoadResource(nullptr, res);
//     vector<uint8_t> engine_package;
//     engine_package.resize(size);
//     memcpy(engine_package.data(), data, size);
//     Package::loadPackageFromMemory(engine_package, "engine.hop (internal)");
// #else
    Package::loadPackage("engine.hop");
// #endif
    RenderServer::init();
    RenderServer::setIcon("res://engine/icon.png");
    Input::init();
    
    SamplerBuilder builders[6] =
    {
        { FILTER_LINEAR, ADDRESS_REPEAT },
        { FILTER_NEAREST, ADDRESS_REPEAT },
        { FILTER_LINEAR, ADDRESS_MIRRORED },
        { FILTER_NEAREST, ADDRESS_MIRRORED },
        { FILTER_LINEAR, ADDRESS_CLAMP_EDGE },
        { FILTER_NEAREST, ADDRESS_CLAMP_EDGE },
    };
    for (SamplerBuilder s : builders)
        premade_samplers[s] = new Sampler(s);
    
    debugClearSelection();
}

Engine::~Engine()
{
    debugClearSelection();
    scene = nullptr;
    keep_loaded_refs.clear();
    loaded_shaders.clear();
    loaded_materials.clear();
    loaded_textures.clear();
    loaded_meshes.clear();
    premade_samplers.clear();

    Package::destroy();
    Input::destroy();
    RenderServer::destroy();
    if (!allocated_refs.empty())
    {
        DBG_ERROR("uh oh! there are objects still allocated! prepare for vulkan errors and possibly crashes! see below:");
        Engine::summariseTrackedObjects();
    }
    else
        DBG_INFO("good girl for cleaning up!");
    Debug::close();

    engine = nullptr;
}

Engine* Engine::getEngine()
{ return engine; }

vector<WeakRef<void>> Engine::getRefsWithType(const char* type_name)
{
    vector<WeakRef<void>> refs;
    auto [range_start, range_end] = engine->allocated_refs.equal_range(type_name);
    while (range_start != range_end)
    {
        refs.push_back(range_start->second);
        ++range_start;
    }
    return refs;
}

void Engine::_keepLoaded(const Ref<Destructible>& ref)
{ engine->keep_loaded_refs.push_back(ref); }

void Engine::updateStats(const FrameStats& stats)
{
    last_frame_stats = stats;

    smoothed_delta_time = (smoothed_delta_time * 0.5f) + (stats.delta_time * 0.5f);
    const float fps = 1.0f / stats.delta_time;
    smoothed_fps = (smoothed_fps * 0.5f) + (fps * 0.5f);

    delta_time_history[history_offset] = stats.delta_time;
    fps_history[history_offset] = 1.0f / stats.delta_time;
    history_offset = (history_offset + 1) % 512;

    RenderServer::setTitle(format("hop-engine   -   {:>4.2f}ms   -   {:>6.2f} fps", smoothed_delta_time * 1000.0f, smoothed_fps));
}
