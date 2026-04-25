/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"
#include "events.h"
#include "texture.h"

#include <chrono>
#include <map>
#include <vector>

namespace HopEngine
{

/**
 * @brief struct encapsulating various performance information about a frame which just finished.
 */
struct FrameStats final
{
    float record_time = 0.0f;      // time taken to update rendering resources and record command buffers
    float render_time = 0.0f;      // time taken to render the frame on the GPU
    float update_time = 0.0f;      // time taken to update the scene objects
    std::vector<float> pass_times; // time taken for each render pass on the GPU
    size_t draw_calls       = 0;   // number of mesh draw calls submitted
    size_t pipeline_rebinds = 0;   // number of times GPU pipelines were rebound
    size_t triangles        = 0;   // total number of triangles submitted to draw
    size_t passes           = 0;   // number of render passes executed
    size_t cameras          = 0;   // number of camera passes executed
};

class InitMachine;

/**
 * @brief encapsulates management of the overall graphics engine. provides various miscellaneous tools.
 */
class Engine final
{
    friend class InitMachine;
public:
    /**
     * @brief engine-specific event IDs which can be subscribed to via the event server.
     */
    enum Events : EventServer::TypeID
    {
        EVENT_TYPE_INIT_FINISH        = 0x10000001, // called post-initialisation
        EVENT_TYPE_DESTROY_START      = 0x10000002, // called pre-destruction
        EVENT_TYPE_FRAME_BEGIN        = 0x10000003, // called before a frame is rendered
        EVENT_TYPE_FRAME_END          = 0x10000004, // called after a frame has been rendered
        EVENT_TYPE_SCENE_CHANGE       = 0x10000005, // called when a new scene is made current
        EVENT_TYPE_APPLICATION_CHANGE = 0x10000006, // called when a new application is started
    };

    /**
     * @brief engine setup parameter struct.
     */
    struct InitParams final
    {
        bool enable_vulkan_validation = false;             // if `true` Vulkan validation layers are enabled
        Debug::Level debug_log_level  = Debug::DEBUG_INFO; // debug output level
        bool create_log_file          = true;              // if `true`, output will be copied to a file
    };

private:
    Ref<Scene> scene;             // currently active scene
    Ref<Application> application; // currently running application
    // application which will be switched to when this frame has been completed
    Ref<Application> next_application;

    // all currently managed `Ref` objects known to the engine
    std::multimap<const char*, WeakRef<void>> allocated_refs;
    // currently loaded shaders, to avoid reloading when not necessary
    std::map<std::string, Ref<Shader>> loaded_shaders;
    // currently loaded materials, to avoid reloading when not necessary
    std::map<std::string, Ref<Material>> loaded_materials;
    // currently loaded textures, to avoid reloading when not necessary
    std::map<std::string, Ref<Texture>> loaded_textures;
    std::map<std::string, Ref<Texture>> loaded_3d_textures;
    // currently loaded meshes, to avoid reloading when not necessary
    std::map<std::string, Ref<Mesh>> loaded_meshes;
    // all possible samplers are created automatically, to avoid duplication
    std::map<std::pair<Sampler::Filter, Sampler::Address>, Ref<Sampler>> premade_samplers;

    FrameStats last_frame_stats; // statistics from the last frame that was rendered
    // time point when the engine was initialised
    std::chrono::steady_clock::time_point engine_start_timestamp;
    float delta_time          = 0.0f; // time between the last frame and this frame
    float total_time          = 0.0f; // total time since the engine was initialised
    float smoothed_delta_time = 0.0f; // delta time with 90% smoothing applied
    float smoothed_fps        = 0.0f; // inverse delta time with 90% smoothing applied
    size_t frame_index        = 0;    // number of frames rendered since the engine was initalised
    float delta_time_history[200];    // last 200 frames worth of delta time values
    int history_offset = 0;           // used for displaying delta time history

    // if `true` all materials (except post-process materials) will render in wireframe view
    bool wireframe_view = false;
    // if `true`, objects may draw gizmos, for editor modes
    bool show_gizmos = false;

    // if `true` the engine will stop and exit the current application at the end of the frame
    bool stop_requested = false;
    // if `true` the mainloop is currently running, and cannot be started again
    bool start_called = false;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Engine);

    static void init(const InitParams& params);
    static void destroy();

    /**
     * @brief starts the engine mainloop with the specified application class as the client. you should
     * extend `Application` appropriately to implement your game/application code. may not be called again
     * while the mainloop is running, although you may call `stop` and then call this again.
     */
    template<class T> static void startApplication();
    /**
     * @brief switches the engine to use the specified application class as the client. may be called while
     * the mainloop is running to jump from one game/application to another.
     */
    template<class T> static void switchApplication();
    /**
     * @brief tells the engine to stop the mainloop once the current frame is finished. program control flow
     * will be returned to wherever `startApplication` was called.
     */
    static void stop();

    /**
     * @brief switches the currently active scene to the specified scene. also propagates to the render
     * server via `RenderServer::setSingleScene`.
     * @param new_scene primary scene to use for rendering and other activity from now on.
     */
    static void setScene(const Ref<Scene>& new_scene);
    static Ref<Scene> getScene();

    static float getDeltaTime() { return getEngine()->delta_time; };
    static float getEngineTime() { return getEngine()->total_time; }
    static float getSmoothedDeltaTime() { return getEngine()->smoothed_delta_time; }
    static float getSmoothedFPS() { return getEngine()->smoothed_fps; }
    static size_t getFrameCount() { return getEngine()->frame_index; }
    static FrameStats getFrameStats() { return getEngine()->last_frame_stats; }

    static bool isWireframeMode() { return getEngine()->wireframe_view; }
    /**
     * @brief toggles debug wireframe mode. when active, all non-postprocessing materials will render as
     * wireframes regardless of their normal pipeline configuration. can be useful for debugging.
     * @param value if `true`, materials will draw in wireframe mode, otherwise materials will draw as
     * normal.
     */
    static void setForceWireframe(bool value) { getEngine()->wireframe_view = value; }
    static bool getShowGizmos() { return getEngine()->show_gizmos; }
    /**
     * @brief toggles gizmo visibility mode. when active, some objects will draw editor gizmos.
     * @param value if `true`, objects can emit draw calls for gizmos.
     */
    static void setShowGizmos(bool value) { getEngine()->show_gizmos = value; }

    /**
     * @brief selects an object to be viewable in ImGui debug mode. semi-deprecated.
     * @param object object to show debug information for. debug selection should be cleared before the
     * object is destroyed.
     */
    static void debugSelect(const WeakRef<Object>& object);
    /**
     * @brief retrieves object currently flagged for debug selection (as set by `debugSelect`).
     * @returns currently selected object, or `nullptr` the selection is empty.
     */
    static WeakRef<Object> getDebugSelection();
    /**
     * @brief given a camera object, performs simple debug fly-camera movement: right-mouse drag to pan the
     * camera, WASDQE to move around, Shift to move faster. should be called for every frame you want camera
     * movement to apply.
     * @param selected_camera object on which to perform flying movement on. doesn't necessarily need to
     * actually have a camera component.
     */
    static void debugCamera(const WeakRef<Object>& selected_camera);

    /**
     * @brief loads a shader from the specified path. if the specified path has already been loaded as a
     * shader, a reference to the already-loaded shader is returned instead. this function should be used
     * instead of `Shader::Shader` in order to prevent resource duplication.
     * @param path path to the shader, either as a filesystem path relative to the working directory, or as
     * a resource path beginning `res://`.
     * @returns newly loaded shader, or a reference to the already-loaded version if available.
     */
    static Ref<Shader> loadShader(const std::string& path);
    /**
     * @brief loads a material from the specified path. if the specified path has already been loaded as a
     * material, a reference to the already-loaded material is returned instead. this function should be
     * used instead of `Material::deserialiseFile` in order to prevent resource duplication.
     * @param path path to the material, either as a filesystem path relative to the working directory, or
     * as a resource path beginning `res://`.
     * @returns newly loaded material, or a reference to the already-loaded version if available.
     */
    static Ref<Material> loadMaterial(const std::string& path);
    /**
     * @brief loads a texture from the specified path. if the specified path has already been loaded as a
     * texture, a reference to the already-loaded texture is returned instead. this function should be used
     * instead of `Texture::loadImage` in order to prevent resource duplication.
     * @param path path to the texture, either as a filesystem path relative to the working directory, or
     * as a resource path beginning `res://`.
     * @returns newly loaded texture, or a reference to the already-loaded version if available.
     */
    static Ref<Texture> loadTexture(const std::string& path);
    /**
     * @brief loads a 3D texture from the specified path. if the specified path has already been
     * loaded as a texture, a reference to the already-loaded texture is returned instead. the texture will
     * be sliced as specified by the `layers_` arguments, with slices arranged left-right, top-bottom. this
     * function should be used instead of `Texure::loadImage3D` in order to prevent resource
     * duplication.
     * @param path path to the texture, either as a filesystem path relative to the working directory, or
     * as a resource path beginning `res://`.
     * @param layers_wide number of horizontal slices in the texture.
     * @param layers_high number of vertical slices in the texture.
     * @returns newly loaded texture, or a reference to the already-loaded version if available.
     */
    static Ref<Texture> loadTexture3D(const std::string& path, int layers_wide, int layers_high);
    /**
     * @brief loads a mesh from the specified path. if the specified path has already been loaded as a
     * mesh, a reference to the already-loaded texture is returned instead. this function should be used
     * instead of `Mesh::loadMesh` in order to prevent resource duplication.
     * @param path path to the mesh, either as a filesystem path relative to the working directory, or as a
     * resource path beginning `res://`.
     * @returns newly loaded mesh, or a reference to the already-loaded version if available.
     */
    static Ref<Mesh> loadMesh(const std::string& path);
    /**
     * @brief fetches a sampler with the specified filtering mode, and default addressing mode. this
     * function should be used instead of `Sampler::Sampler` in order to prevent duplication.
     * @param filter filtering mode which the sampler should use.
     * @returns sampler with the specified characteristics.
     */
    static Ref<Sampler> getSampler(Sampler::Filter filter);
    /**
     * @brief fetches a sampler with the specified addressing mode, and default filtering mode. this
     * function should be used instead of `Sampler::Sampler` in order to prevent duplication.
     * @param address addressing mode which the sampler should use.
     * @returns sampler with the specified characteristics.
     */
    static Ref<Sampler> getSampler(Sampler::Address address);
    /**
     * @brief fetches a sampler with the specified filtering and addressing mode. this function
     * should be used instead of `Sampler::Sampler` in order to prevent duplication.
     * @param filter filtering mode which the sampler should use.
     * @param address addressing mode which the sampler should use.
     * @returns sampler with the specified characteristics.
     */
    static Ref<Sampler> getSampler(Sampler::Filter filter, Sampler::Address address);
    /**
     * @brief destroys any resources loaded by `loadShader`, `loadMaterial`, `loadTexture`, `loadTexture3D`,
     * or `loadMesh` which have no users in the code. useful to prune resources which are not needed for the
     * current scene to limit memory usage.
     * @returns number of resources which were successfully pruned.
     */
    static size_t pruneUnusedResources();

    /**
     * @brief fetches a list of all currently loaded reference counted objects which match a given type.
     * @returns list of currently loaded objects of type `T`.
     */
    template<class T> static std::vector<WeakRef<T>> getAllRefs();
    /**
     * @brief internal-use-only.
     */
    static void registerCountedRef(const char* type_name, const WeakRef<void>& reference);
    /**
     * @brief internal-use-only.
     */
    static void unregisterCountedRef(const void* ptr);

    /**
     * @brief displays a debug UI over the screen. information about the engine, framerate, resources, and
     * currently selected object/material can be accessed.
     */
    static void drawImGuiDebug();

private:
    Engine(const InitParams& params);
    ~Engine();

    /**
     * @brief starts the engine mainloop, using whatever the current application object is set to. this
     * function should only get called on the main thread, and will block until something in the program
     * calls `stop`.
     */
    static void start();
    static Engine* getEngine();
    /**
     * @brief fetches a list of all currently loaded reference counted objects which match the specified
     * type.
     * @param type_name result of `typeid(T).name()` for the target template type of the reference to match
     * for.
     * @returns list of currently loaded objects of type matching `type_name`.
     */
    static std::vector<WeakRef<void>> getRefsWithType(const char* type_name);
    /**
     * @brief updates internal frame statistics counters.
     * @param stats statistics struct for the last frame.
     */
    void updateStats(const FrameStats& stats);
    /**
     * @brief displays a summary of all currently loaded reference counted objects. useful for debugging
     * resources which are unintentionally hanging around at program exit.
     */
    static void summariseTrackedObjects();

    void _drawImGuiDebug() const;
};

/**
 * @brief base class for user-defined game/application main classes. users should override the `awake`,
 * `update`, and `drawImGui` functions to provide their own behaviour.
 */
class Application : public Destructible
{
public:
    DELETE_NOT_ALL_CONSTRUCTORS(Application);
    Application()  = default;
    ~Application() = default;

    /**
     * @brief called when the application is initialised via `Engine::startApplication` or
     * `Engine::switchApplication`. may be overriden by the user to provide startup functionality.
     */
    virtual void awake() {}
    /**
     * @brief called once per frame. may be overriden by the user to provide their own per-frame
     * functionality.
     * @param delta_time length of time in seconds which has passed since the last frame was rendered.
     */
    virtual void update(float delta_time) {}
    /**
     * @brief called once per frame. may be overriden by the user to show custom ImGui debug UI on the
     * screen.
     */
    virtual void drawImGui() {}
};

template<class T> inline void Engine::startApplication()
{
    static_assert(std::is_convertible_v<T*, Application*>, "T must be a HopEngine::Application subclass");
    if (getEngine()->start_called)
    {
        DBG_WARNING("an application is already running. did you mean to call switchApplication?");
        return;
    }
    getEngine()->application = new T();
    getEngine()->application->awake();
    EventServer::dispatch(EVENT_TYPE_APPLICATION_CHANGE);
    Engine::start();
}

template<class T> inline void Engine::switchApplication()
{
    static_assert(std::is_convertible_v<T*, Application*>, "T must be a HopEngine::Application subclass");
    getEngine()->next_application = new T();
}

template<class T> std::vector<WeakRef<T>> Engine::getAllRefs()
{
    auto refs = getRefsWithType(typeid(T).name());
    std::vector<WeakRef<T>> cast_refs;
    for (auto& r : refs) cast_refs.push_back(r.cast<T>());
    return cast_refs;
}

/**
 * @brief utility class for parsing command line arguments/options. provides an iterator-like behaviour.
 */
class CommandLineParser final
{
public:
    /**
     * @brief enumerates types of command line option. the type of argument is determined based on the
     * number of dashes in front of the argument.
     */
    enum ArgumentType
    {
        ARGUMENT_TEXT, // simple text arguments, such as `input_file.txt`
        FLAG_CHAR,     // single-character flags, such as `-a` or `-fxr`
        FLAG_TEXT      // multi-character flags, such as `--verbose`
    };

    /**
     * @brief struct describing a command line argument.
     */
    struct Argument final
    {
        ArgumentType type; // type of argument
        std::string value; // string value, with up to two preceding dashes removed
    };

    typedef std::vector<Argument>::const_iterator const_iterator;

private:
    std::string executable_path;            // path to the executable, always passed as the first argument
    std::vector<std::string> arguments;     // string versions of the remaining arguments
    std::vector<Argument> arguments_parsed; // parsed versions of the remaining arguments

public:
    CommandLineParser() = delete;
    /**
     * @brief constructs the argument parser around the incoming command line arguments. the constructor
     * extracts each argument and performs simple parsing.
     * @param nargs number of elements in `cargs`.
     * @param cargs array of arguments as C strings.
     */
    CommandLineParser(int nargs, const char** cargs);

    /**
     * @brief fetches the path to the executable currently running, always passed as the first command line
     * argument in C/C++.
     * @returns executable path.
     */
    std::string getExecutablePath() const { return executable_path; }

    const_iterator begin() const { return arguments_parsed.begin(); }
    const_iterator end() const { return arguments_parsed.end(); }
};

} // namespace HopEngine
