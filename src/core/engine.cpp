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

void Engine::start()
{
    if (getInstance()->start_called)
    {
        DBG_WARNING(
            "attempt to start the engine mainloop when the mainloop is already running. don't do that!");
        return;
    }
    getInstance()->start_called = true;

    auto last_frame = std::chrono::steady_clock::now();

    while (!Window::shouldClose() && !getInstance()->stop_requested)
    {
        EventServer::dispatch(EVENT_TYPE_FRAME_BEGIN);

        if (getInstance()->next_application)
        {
            getInstance()->application      = getInstance()->next_application;
            getInstance()->next_application = nullptr;
            getInstance()->application->awake();
            EventServer::dispatch(EVENT_TYPE_APPLICATION_CHANGE);
        }

        auto this_frame                          = std::chrono::steady_clock::now();
        std::chrono::duration<float> delta       = this_frame - last_frame;
        last_frame                               = this_frame;
        getInstance()->delta_time                = delta.count();
        std::chrono::duration<float> since_start = this_frame - getInstance()->engine_start_timestamp;
        getInstance()->total_time                = since_start.count();

        Input::pollInput();

        if (Input::wasKeyPressed(Input::KEY_F11)) Window::setFullscreen(!Window::isFullscreen());
        if (Input::wasKeyPressed(Input::KEY_F10)) Engine::setForceWireframe(!Engine::isWireframeMode());
        if (Input::wasKeyPressed(Input::KEY_F9))
            GraphicsServer::setOverlayLogs(!GraphicsServer::getOverlayLogs());

        auto update_start = std::chrono::steady_clock::now();
        if (getInstance()->application) getInstance()->application->update(getDeltaTime());
        std::chrono::duration<float> update_duration = std::chrono::steady_clock::now() - update_start;

        if (getInstance()->application)
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (getInstance()->application) getInstance()->application->drawImGui();

            ImGui::Render();
        }
        FrameStats stats  = GraphicsServer::draw();
        stats.update_time = update_duration.count();

        getInstance()->updateStats(stats);

        ++(getInstance()->frame_index);

        EventServer::dispatch(EVENT_TYPE_FRAME_END);
    }

    getInstance()->start_called = false;
}

void Engine::setScene(const Ref<Scene>& new_scene)
{
    Engine::debugSelect(WeakRef<Object>());
    getInstance()->scene = new_scene;
    GraphicsServer::setSingleScene(new_scene);
    EventServer::dispatch(EVENT_TYPE_SCENE_CHANGE);
}

void Engine::stop() { getInstance()->stop_requested = true; }

void Engine::reset()
{
    stop();

    Engine::debugSelect(WeakRef<Object>());
    getInstance()->scene            = nullptr;
    getInstance()->application      = nullptr;
    getInstance()->next_application = nullptr;
    getInstance()->loaded_shaders.clear();
    getInstance()->loaded_materials.clear();
    getInstance()->loaded_textures.clear();
    getInstance()->loaded_3d_textures.clear();
    getInstance()->loaded_meshes.clear();
    getInstance()->premade_samplers.clear();
}

Ref<Scene> Engine::getScene() { return getInstance()->scene; }

// all currently managed `Ref` objects known to the engine
static std::multimap<const char*, WeakRef<void>> allocated_refs;

void Engine::summariseTrackedObjects()
{
    DBG_INFO("enumerating allocated objects (" + std::to_string(allocated_refs.size()) + "):");
    for (auto& [type_name, ptr] : allocated_refs)
        DBG_INFO("object " + PTR(ptr.get()) + ", with type '" + type_name + '\'');
}

size_t Engine::countTrackedObjects() { return allocated_refs.size(); }

void HopEngine::registerCountedRef(const char* type_name, const WeakRef<void>& reference)
{ allocated_refs.insert({ type_name, reference }); }

void HopEngine::unregisterCountedRef(const void* ptr)
{
    for (auto pair = allocated_refs.begin(); pair != allocated_refs.end(); ++pair)
    {
        if (pair->second.get() == ptr)
        {
            allocated_refs.erase(pair);
            return;
        }
    }
    DBG_ERROR(
        "a reference counted object was deallocated, but its raw pointer is not allocated! you probably used a raw pointer twice. we are about to double-free that pointer. fuck you.");
}

Ref<Shader> Engine::loadShader(const std::string& path)
{
    const auto it = getInstance()->loaded_shaders.find(path);
    if (it == getInstance()->loaded_shaders.end())
    {
        Ref<Shader> thing                   = new Shader(path);
        getInstance()->loaded_shaders[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused shader '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Material> Engine::loadMaterial(const std::string& path)
{
    const auto it = getInstance()->loaded_materials.find(path);
    if (it == getInstance()->loaded_materials.end())
    {
        Ref<Material> thing                   = Material::deserialiseFile(path);
        getInstance()->loaded_materials[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused material '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Texture> Engine::loadTexture(const std::string& path)
{
    const auto it = getInstance()->loaded_textures.find(path);
    if (it == getInstance()->loaded_textures.end())
    {
        Ref<Texture> thing                   = Texture::loadImage(path);
        getInstance()->loaded_textures[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused texture '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Texture> Engine::loadTexture3D(const std::string& path, const int layers_wide, const int layers_high)
{
    const auto it = getInstance()->loaded_3d_textures.find(path);
    if (it == getInstance()->loaded_3d_textures.end())
    {
        Ref<Texture> thing                      = Texture::loadImage3D(path,
            { static_cast<uint32_t>(layers_wide), static_cast<uint32_t>(layers_high) });
        getInstance()->loaded_3d_textures[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused texture 3D '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Mesh> Engine::loadMesh(const std::string& path)
{
    const auto it = getInstance()->loaded_meshes.find(path);
    if (it == getInstance()->loaded_meshes.end())
    {
        Ref<Mesh> thing                    = Mesh::loadMesh(path);
        getInstance()->loaded_meshes[path] = thing;
        return thing;
    }
    DBG_VERBOSE("reused mesh '" + path + "' instead of duplicating");
    return it->second;
}

Ref<Sampler> Engine::getSampler(Sampler::Filter filter)
{ return getSampler(filter, Sampler::ADDRESS_REPEAT); }

Ref<Sampler> Engine::getSampler(Sampler::Address address)
{ return getSampler(Sampler::FILTER_NEAREST, address); }

Ref<Sampler> Engine::getSampler(Sampler::Filter filter, Sampler::Address address)
{
    if (getInstance()->premade_samplers.find({ filter, address }) != getInstance()->premade_samplers.end())
        return getInstance()->premade_samplers[{ filter, address }];
    else
    {
        Ref<Sampler> s                                       = new Sampler(filter, address);
        getInstance()->premade_samplers[{ filter, address }] = s;
        return s;
    }
}

size_t Engine::pruneUnusedResources()
{
    DBG_INFO("pruning currently loaded unused resources...");
    size_t pruned_refs = 0;
    auto mesh_it       = getInstance()->loaded_meshes.begin();
    while (mesh_it != getInstance()->loaded_meshes.end())
    {
        if (mesh_it->second.getCount() == 1)
        {
            auto next = mesh_it;
            ++next;
            getInstance()->loaded_meshes.erase(mesh_it);
            ++pruned_refs;
            mesh_it = next;
        }
        else
            ++mesh_it;
    }
    auto mat_it = getInstance()->loaded_materials.begin();
    while (mat_it != getInstance()->loaded_materials.end())
    {
        if (mat_it->second.getCount() == 1)
        {
            auto next = mat_it;
            ++next;
            getInstance()->loaded_materials.erase(mat_it);
            ++pruned_refs;
            mat_it = next;
        }
        else
            ++mat_it;
    }
    auto shr_it = getInstance()->loaded_shaders.begin();
    while (shr_it != getInstance()->loaded_shaders.end())
    {
        if (shr_it->second.getCount() == 1)
        {
            auto next = shr_it;
            ++next;
            getInstance()->loaded_shaders.erase(shr_it);
            ++pruned_refs;
            shr_it = next;
        }
        else
            ++shr_it;
    }
    auto tex_it = getInstance()->loaded_textures.begin();
    while (tex_it != getInstance()->loaded_textures.end())
    {
        if (tex_it->second.getCount() == 1)
        {
            auto next = tex_it;
            ++next;
            getInstance()->loaded_textures.erase(tex_it);
            ++pruned_refs;
            tex_it = next;
        }
        else
            ++tex_it;
    }
    DBG_INFO("pruned " + std::to_string(pruned_refs) + " total reference counted objects");
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

size_t chronoToTime(std::chrono::system_clock::time_point time_point)
{ return std::chrono::system_clock::to_time_t(time_point); }

void Engine::setRPCTimestamp(std::chrono::system_clock::time_point start_time)
{
    discord::RPCManager::get()
        .getPresence()
        .setStartTimestamp(chronoToTime(start_time))
        .setEndTimestamp(0)
        .refresh();
}

void Engine::setRPCTimestamp(std::chrono::system_clock::time_point start_time,
    std::chrono::system_clock::time_point end_time)
{
    discord::RPCManager::get()
        .getPresence()
        .setStartTimestamp(chronoToTime(start_time))
        .setEndTimestamp(chronoToTime(end_time))
        .refresh();
}

void Engine::clearRPCActivity() { discord::RPCManager::get().clearPresence(); }

void Engine::drawImGuiDebug() { getInstance()->_drawImGuiDebug(); }

Engine::Engine(const InitParams& params, bool& success)
{
    getInstance()->engine_start_timestamp = std::chrono::steady_clock::now();

    auto discord_application_id_data = Package::load("discord_appid.txt");
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
                    discord::RPCManager::get()
                        .getPresence()
                        .setStatusDisplayType(discord::StatusDisplayType::State)
                        .refresh();
                })
            .onDisconnected([](int errcode, std::string_view message)
                { DBG_WARNING("disconnected discord with error code " + std::string(message)); })
            .onErrored([](int errcode, std::string_view message)
                { DBG_ERROR("discord error with code " + std::string(message)); })
            .onJoinGame([](std::string_view joinSecret) { DBG_INFO("discord join game"); })
            .onSpectateGame([](std::string_view spectateSecret) { DBG_INFO("discord spectate game"); })
            .onJoinRequest([](discord::User const& user)
                { DBG_INFO("discord join game request from " + user.username); })
            .initialize();
    }

    success = true;
}

Engine::~Engine()
{
    discord::RPCManager::get().shutdown();
    reset();
}

std::vector<WeakRef<void>> Engine::getRefsWithType(const char* type_name)
{
    std::vector<WeakRef<void>> refs;
    auto [range_start, range_end] = allocated_refs.equal_range(type_name);
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

    frame_free_percent =
        glm::clamp(1.0f - (glm::max(stats.record_time + stats.update_time, stats.render_time) / delta_time),
            0.0f, 1.0f) *
        100.0f;
}
