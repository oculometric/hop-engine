#include "engine.h"

#include <chrono>
#include <format>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/imgui.h>
#if defined(_WIN32)
#include <Windows.h>
#endif
#include "hop_engine.h"

#include <discord-presence/discord-rpc.hpp>

using namespace HopEngine;
using namespace std;

static Engine* engine = nullptr;

void Engine::init(const InitParams& params)
{
    if (engine == nullptr) engine = new Engine(params);
}

void Engine::destroy()
{
    if (engine != nullptr)
    {
        delete engine;
        engine = nullptr;
    }
}

void Engine::start()
{
    if (engine->start_called)
    {
        DBG_WARNING(
            "attempt to start the engine mainloop when the mainloop is already running. don't do that!");
        return;
    }
    engine->start_called = true;

    auto last_frame = chrono::steady_clock::now();

    while (!RenderServer::getWindowShouldClose() && !engine->stop_requested)
    {
        EventServer::dispatch(EVENT_TYPE_FRAME_BEGIN);

        if (engine->next_application)
        {
            engine->application      = engine->next_application;
            engine->next_application = nullptr;
            engine->application->awake();
            EventServer::dispatch(EVENT_TYPE_APPLICATION_CHANGE);
        }

        auto this_frame                     = chrono::steady_clock::now();
        chrono::duration<float> delta       = this_frame - last_frame;
        last_frame                          = this_frame;
        engine->delta_time                  = delta.count();
        chrono::duration<float> since_start = this_frame - engine->engine_start_timestamp;
        engine->total_time                  = since_start.count();

        Input::pollInput();

        if (Input::wasKeyPressed(Input::KEY_F11))
            RenderServer::setFullscreenEnabled(!RenderServer::getFullscreenEnabled());
        if (Input::wasKeyPressed(Input::KEY_F10)) Engine::setForceWireframe(!Engine::isWireframeMode());
        if (Input::wasKeyPressed(Input::KEY_F9))
            RenderServer::setOverlayLogs(!RenderServer::getOverlayLogs());

        auto update_start = chrono::steady_clock::now();
        if (engine->application) engine->application->update(getDeltaTime());
        chrono::duration<float> update_duration = chrono::steady_clock::now() - update_start;

        if (engine->application)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (engine->application) engine->application->drawImGui();

            ImGui::Render();
        }
        FrameStats stats  = RenderServer::draw();
        stats.update_time = update_duration.count();

        engine->updateStats(stats);

        ++(engine->frame_index);

        EventServer::dispatch(EVENT_TYPE_FRAME_END);
    }

    engine->start_called = false;
}

void Engine::setScene(const Ref<Scene>& new_scene)
{
    Engine::debugSelect(WeakRef<Object>());
    engine->scene = new_scene;
    RenderServer::setSingleScene(new_scene);
    EventServer::dispatch(EVENT_TYPE_SCENE_CHANGE);
}

void Engine::stop() { engine->stop_requested = true; }

Ref<Scene> Engine::getScene() { return engine->scene; }

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
    DBG_ERROR(
        "a reference counted object was deallocated, but its raw pointer is not allocated! you probably used a raw pointer twice. we are about to double-free that pointer. fuck you.");
}

void HopEngine::registerCountedRef(const char* type_name, const WeakRef<void>& reference)
{ Engine::registerCountedRef(type_name, reference); }

void HopEngine::unregisterCountedRef(const void* ptr) { Engine::unregisterCountedRef(ptr); }

Ref<Shader> Engine::loadShader(const string& path)
{
    const auto it = engine->loaded_shaders.find(path);
    if (it == engine->loaded_shaders.end())
    {
        Ref<Shader> thing            = new Shader(path);
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
        Ref<Material> thing            = Material::deserialiseFile(path);
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
        Ref<Texture> thing            = Texture::loadImage(path);
        engine->loaded_textures[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused texture '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Texture> Engine::loadTexture3D(const string& path, const int layers_wide, const int layers_high)
{
    const auto it = engine->loaded_3d_textures.find(path);
    if (it == engine->loaded_3d_textures.end())
    {
        Ref<Texture> thing               = Texture::loadImage3D(path,
            { static_cast<uint32_t>(layers_wide), static_cast<uint32_t>(layers_high) });
        engine->loaded_3d_textures[path] = thing;
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
        Ref<Mesh> thing             = Mesh::loadMesh(path);
        engine->loaded_meshes[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused mesh '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Sampler> Engine::getSampler(Sampler::Filter filter)
{ return engine->premade_samplers[{ filter, Sampler::ADDRESS_REPEAT }]; }

Ref<Sampler> Engine::getSampler(Sampler::Address address)
{ return engine->premade_samplers[{ Sampler::FILTER_NEAREST, address }]; }

Ref<Sampler> Engine::getSampler(Sampler::Filter filter, Sampler::Address address)
{ return engine->premade_samplers[{ filter, address }]; }

size_t Engine::pruneUnusedResources()
{
    DBG_INFO("pruning currently loaded unused resources...");
    size_t pruned_refs = 0;
    auto mesh_it       = engine->loaded_meshes.begin();
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

void Engine::setRPCDescription(const std::string& details)
{ discord::RPCManager::get().getPresence().setDetails(details).refresh(); }

void Engine::setRPCStatus(const std::string& state, int32_t party_size, int32_t party_max)
{
    discord::RPCManager::get()
        .getPresence()
        .setState(state)
        .setPartySize(party_size)
        .setPartyMax(party_max)
        .refresh();
}

void Engine::setRPCActivity(RPCActivityType activity)
{
    discord::ActivityType type;
    switch (activity)
    {
    case RPC_PLAYING:   type = discord::ActivityType::Game; break;
    case RPC_COMPETING: type = discord::ActivityType::Competing; break;
    case RPC_LISTENING: type = discord::ActivityType::Listening; break;
    case RPC_STREAMING: type = discord::ActivityType::Streaming; break;
    case RPC_WATCHING:  type = discord::ActivityType::Watching; break;
    }

    discord::RPCManager::get().getPresence().setActivityType(type).refresh();
}

void Engine::setRPCTimestamp(std::chrono::system_clock::time_point start_time)
{
    discord::RPCManager::get()
        .getPresence()
        .setStartTimestamp(start_time.time_since_epoch().count() / 1000000000)
        .refresh();
}

void Engine::setRPCTimestamp(std::chrono::system_clock::time_point start_time,
    std::chrono::system_clock::time_point end_time)
{
    discord::RPCManager::get()
        .getPresence()
        .setStartTimestamp(start_time.time_since_epoch().count() / 1000000000)
        .setEndTimestamp(end_time.time_since_epoch().count() / 1000000000)
        .refresh();
}

void Engine::clearRPCActivity()
{
    discord::RPCManager::get().clearPresence();
}

void Engine::drawImGuiDebug() { engine->_drawImGuiDebug(); }

extern unsigned char engine_hop_raw[];
extern unsigned long long engine_hop_raw_size;

class HopEngine::InitMachine final
{
public:
    static void initialise(const Engine::InitParams& params)
    {
        Debug::init(params.create_log_file);
        Debug::setLogLevel(params.debug_log_level);

        EventServer::init();

        Package::init();
        DataBlock engine_hop(engine_hop_raw_size);
        memcpy(engine_hop.data(), engine_hop_raw, engine_hop.size());
        Package::importPackage(engine_hop);

        RenderServer::init(params.enable_vulkan_validation);
        Input::init();
    }

    static void destroy()
    {
        Input::destroy();
        Package::destroy();
        RenderServer::destroy();
        if (!engine->allocated_refs.empty())
        {
            DBG_ERROR(
                "uh oh! there are objects still allocated! prepare for vulkan errors and possibly crashes! see below:");
            Engine::summariseTrackedObjects();
        }
        else
            DBG_INFO("good girl for cleaning up!");

        EventServer::destroy();

        Debug::close();
    }
};

Engine::Engine(const InitParams& params)
{
    engine = this;

    engine->engine_start_timestamp = chrono::steady_clock::now();

    InitMachine::initialise(params);

    RenderServer::setIcon("res://engine/icon.png");
    std::pair<Sampler::Filter, Sampler::Address> builders[6] = {
        {  Sampler::FILTER_LINEAR,     Sampler::ADDRESS_REPEAT },
        { Sampler::FILTER_NEAREST,     Sampler::ADDRESS_REPEAT },
        {  Sampler::FILTER_LINEAR,   Sampler::ADDRESS_MIRRORED },
        { Sampler::FILTER_NEAREST,   Sampler::ADDRESS_MIRRORED },
        {  Sampler::FILTER_LINEAR, Sampler::ADDRESS_CLAMP_EDGE },
        { Sampler::FILTER_NEAREST, Sampler::ADDRESS_CLAMP_EDGE },
    };
    for (auto s : builders) premade_samplers[s] = new Sampler(s.first, s.second);

    auto discord_application_id_data = Package::loadFromDisk("discord_appid.txt");
    std::string discord_application_id(discord_application_id_data.begin(),
        discord_application_id_data.end());

    if (!discord_application_id.empty())
    {
        discord::RPCManager::get()
            .setClientID(discord_application_id)
            .onReady(
                [](discord::User const& user)
                {
                    DBG_INFO("connected to discord user " + user.username);
                    setRPCActivity(RPC_PLAYING);
                    setRPCDescription("hop-engine is running.");
                    setRPCTimestamp(std::chrono::system_clock::now());
                    discord::RPCManager::get().getPresence().setStatusDisplayType(discord::StatusDisplayType::State).refresh();
                })
            .onDisconnected([](int errcode, std::string_view message)
                { DBG_WARNING("disconnected discord with error code " + std::string(message)); })
            .onErrored([](int errcode, std::string_view message)
                { DBG_ERROR("discord error with code " + std::string(message)); })
            .onJoinGame([](std::string_view joinSecret) { DBG_INFO("discord join game"); })
            .onSpectateGame([](std::string_view spectateSecret) { DBG_INFO("discord spectate game"); })
            .onJoinRequest(
                [](discord::User const& user) { DBG_INFO("discord join game request from " + user.username); })
            .initialize();
    }

    EventServer::dispatch(EVENT_TYPE_INIT_FINISH);
}

Engine::~Engine()
{
    EventServer::dispatch(EVENT_TYPE_DESTROY_START);

    discord::RPCManager::get().shutdown();

    Engine::debugSelect(WeakRef<Object>());
    scene            = nullptr;
    application      = nullptr;
    next_application = nullptr;
    loaded_shaders.clear();
    loaded_materials.clear();
    loaded_textures.clear();
    loaded_3d_textures.clear();
    loaded_meshes.clear();
    premade_samplers.clear();

    InitMachine::destroy();
}

Engine* Engine::getEngine() { return engine; }

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

void Engine::updateStats(const FrameStats& stats)
{
    last_frame_stats = stats;

    smoothed_delta_time = (smoothed_delta_time * 0.9f) + (delta_time * 0.1f);
    smoothed_fps        = 1.0f / smoothed_delta_time;

    delta_time_history[history_offset] = delta_time * 1000.0f;
    history_offset                     = (history_offset + 1) % 200;
}
