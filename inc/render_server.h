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
#include "engine.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <vector>

struct GLFWwindow;

namespace HopEngine
{

/**
 * @brief encapsulates all the behaviour of actually initialising and running the rendering environment.
 */
class RenderServer final
{
public:
    /**
     * @brief defines a scene-to-be-rendered, and the portion of the window it should be rendered into.
     */
    struct SceneRender final
    {
        Ref<Scene> scene;   // reference to the scene
        glm::vec2 start_uv; // starting offset in 0-1 UV coordinates, from the top-left of the window
        glm::vec2 size_uv;  // size in 0-1 UV coordinates
    };

    /**
     * @brief describes the queue family indices available on this platform.
     */
    struct QueueFamilies final
    {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;
    };

private:
    GLFWwindow* window       = nullptr;        // main window that the user will see and interact with
    glm::u32vec2 window_size = { 1024, 1024 }; // current size of the window, and i.e. the surface
    // cached value for the size of the window before it entered fullscreen, so that we can return to it
    // when it leaves fullscreen
    glm::u32vec2 size_before_fullscreen = { 1024, 1024 };
    GPUHandle surface                   = nullptr; // surface for rendering into the attached window

    GPUHandle instance        = nullptr; // handle for the graphics API instance itself
    GPUHandle debug_messenger = nullptr; // debug messenger used to handle validation from vulkan nicely
    GPUHandle physical_device = nullptr; // selected physical device GPU handle
    GPUHandle device          = nullptr; // logical device GPU handle, for actually doing stuff
    QueueFamilies queue_families;        // queue family indices for the selected physical device
    GPUHandle graphics_queue = nullptr;  // GPU queue handle used for graphics command buffers
    GPUHandle present_queue  = nullptr;  // GPU queue handle used for presentation commands
    GPUHandle command_pool   = nullptr;  // GPU command pool out of which command buffers are allocated
    // array of graphics command buffers used for rendering, one per frame-in-flight
    std::vector<Ref<DrawCommandBuffer>> command_buffers;

    Ref<Swapchain> swapchain;          // connected to the window surface, provides images to present
    Ref<RenderPass> offscreen_pass;    // standard render pass used by scene objects and cameras
    Ref<RenderPass> final_render_pass; // render pass used to draw into the swapchain (colour only)

    GPUHandle descriptor_pool = nullptr; // pool from which descriptors and descriptor sets are allocated
    // universally used descriptor set layout for set 0, containing scene and camera information
    GPUHandle scene_descriptor_set_layout = nullptr;
    // universally used descriptor set layout for set 1, containing per-object information
    GPUHandle object_descriptor_set_layout = nullptr;
    // default descriptor set layout for set 2, with no bindings
    GPUHandle default_descriptor_set_layout = nullptr;
    // pipeline layout describing a simple 0-1-2 pipeline, used when the actual pipeline is unknown
    GPUHandle default_pipeline_layout = nullptr;

    Ref<Texture> default_image;     // default image used when none is specified
    Ref<Texture> default_3d_image;  // default image used for 3D views when none is specified
    Ref<Sampler> default_sampler;   // default sampler used when none is specified
    Ref<Material> default_material; // default material used when none is specified

    Ref<Mesh> default_mesh; // default mesh
    Ref<Mesh> skybox_cube;  // mesh used to render skyboxes
    Ref<Mesh> quad;         // mesh used to render quads
    Ref<Mesh> tri;          // mesh used to render full-screen images (NOT A QUAD)

    Ref<Font> debug_text_font;           // font used for drawing debug logs on the screen
    Ref<UIRenderer> debug_text_renderer; // ui renderer used for drawing logs on the screen

    Ref<UniformBlock> final_pass_uniforms; // scene uniforms for the final (swapchain) render pass
    Ref<Material> spinner_material;        // material used to render the loading/no-scene spinner image
    Ref<UniformBlock> spinner_uniforms;    // object uniforms used for the loading/no-scene spinner
    std::vector<SceneRender> scenes;       // list of scenes currently wanting to be rendered each frame

    bool fullscreen              = false; // if `true`, the window should be borderless fullscreen
    bool wants_fullscreen_update = false; // if `true`, there is a setFullscreen operation pending
    bool vsync                   = true;  // if `true`, image present is clamped to screen refresh rate
    bool wants_vsync_update      = false; // if `true`, there is a setVsync operation pending
    bool overlay_logs            = false; // if `true`, the last 15 lines of logs are shown on the screen

    // list of free operations which will be executed the next time the garbage collector runs
    std::vector<std::function<void()>> free_list;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(RenderServer);

    static void init(bool enable_validation);
    static void destroy();

    static GPUHandle getDevice() { return getInstance()->device; }
    /**
     * @brief blocks execution until the GPU has finished rendering.
     */
    static void waitIdle();
    static GPUHandle getPhysicalDevice() { return getInstance()->physical_device; }
    static GPUHandle getSurface() { return getInstance()->surface; }
    static GPUHandle getGraphicsQueue() { return getInstance()->graphics_queue; }
    static uint32_t getGraphicsQueueIndex()
    { return getInstance()->queue_families.graphics_family.value(); }
    /**
     * @brief queries the queue indices, deduplicated. on many platforms, graphics and present queues are
     * one and the same, and sometimes you need to know that.
     * @returns array of unique queue family indices.
     */
    static std::vector<uint32_t> getUniqueQueueIndices();
    static GPUHandle getPresentQueue() { return getInstance()->present_queue; }
    static GPUHandle getCommandPool() { return getInstance()->command_pool; }
    static GPUHandle getDescriptorPool() { return getInstance()->descriptor_pool; }
    /**
     * @brief creates a uniform buffer which maps to descriptor set 0, which holds scene and camera related
     * uniform variables.
     * @returns new uniform buffer for scene uniforms.
     */
    static Ref<UniformBlock> createSceneUniforms();
    /**
     * @brief creates a uniform buffer which maps to descriptor set 1, which holds object-specific
     * uniform variables.
     * @returns new uniform buffer for object uniforms.
     */
    static Ref<UniformBlock> createObjectUniforms();
    /**
     * @brief creates a pipeline layout from a descriptor set layout in set 2, which holds material-specific
     * uniform variables. set 0 and set 1 (scene and object) descriptor set layouts are applied
     * automatically.
     * @param layout_set_2 VkDescriptorSetLayout for the descriptor set layout in set 2.
     * @returns VkPipelineLayout which can be used for rendering meshes using the shader to which the layout
     * corresponds.
     */
    static GPUHandle createPipelineLayout(GPUHandle layout_set_2);
    static GPUHandle getDefaultPipelineLayout() { return getInstance()->default_pipeline_layout; }

    /**
     * @brief queues up a resource to be freed when the internal garbage collector runs. this should be used
     * for resources which cannot be freed during rendering, as the garbage collector running causes the CPU
     * to wait until the GPU is idle (e.g. for vkDestroyBuffer, etc). the garbage collector runs when more
     * than 30 resources are queued for destruction, or when more than 2 seconds as passed.
     * @param destructor lambda which performs code to free the resource. resources MUST be captured as
     * copies, not references, as there is no guarantee that the calling object will still be around by the
     * time the destructor is called and resource is cleaned up. if that's a problem, don't use the garbage
     * collector.
     */
    static void queueFree(std::function<void()> destructor);

    static WeakRef<RenderPass> getMainRenderPass() { return getInstance()->offscreen_pass; }
    static WeakRef<RenderPass> getFinalRenderPass() { return getInstance()->final_render_pass; }
    static WeakRef<Texture> getDefaultTexture() { return getInstance()->default_image; }
    static WeakRef<Texture> getDefault3DTexture() { return getInstance()->default_3d_image; }
    static WeakRef<Sampler> getDefaultSampler() { return getInstance()->default_sampler; }
    static WeakRef<Material> getDefaultMaterial() { return getInstance()->default_material; }
    static WeakRef<Mesh> getDefaultMesh() { return getInstance()->default_mesh; }
    static WeakRef<Mesh> getSkyboxCube() { return getInstance()->skybox_cube; }
    static WeakRef<Mesh> getQuad() { return getInstance()->quad; }

    /**
     * @brief fetches the number of frames which may be being rendered at any one time.
     * @returns number of frames which can be concurrently 'in-flight' on the GPU.
     */
    static uint32_t getFramesInFlight();
    static GLFWwindow* getWindow() { return getInstance()->window; }
    static glm::vec2 getFramebufferSize();
    /**
     * @brief checks if the GLFW window is waiting to be terminated.
     * @returns `true` if the user has just clicked the close button or otherwise closed the window,
     * otherwise `false`.
     */
    static bool getWindowShouldClose();
    /**
     * @brief updates the window title.
     * @param title new text to be displayed in the window title, if the title bar is visible.
     */
    static void setTitle(const std::string& title);
    /**
     * @brief toggles window visibility.
     * @param visible if `true`, the window will be shown, otherwise the window will be made invisible.
     */
    static void setVisible(bool visible);
    /**
     * @brief updates the window icon.
     * @param path path to the window icon file.
     */
    static void setIcon(const std::string& path);
    /**
     * @brief toggles window decoration.
     * @param borderless if `true`, then no decorations will be shown (window borders and title bar),
     * otherwise the platform-default decorations are drawn. window title bar controls may also be
     * unavailable to the user.
     */
    static void setBorderless(bool borderless);

    /**
     * @brief toggles whether image presentation to the window is tied to the screen's refresh rate.
     * generally this should be enabled unless you're trying to find out how fast she can go or you're
     * trying to flex.
     * @param enabled if `true`, images will not be presented to the screen faster than the screen's refresh
     * rate. if `false`, images will be presented to the screen as fast as they can be rendered.
     */
    static void setVsyncEnabled(bool enabled);
    static bool getVsyncEnabled();
    /**
     * @brief toggles whether the window is set in fullscreen mode or not. when fullscreened, the window
     * takes up the entire monitor, shows on top of all other windows, and lacks decorations. when the
     * window becomes unfocused in this state, it becomes entirely invisible until focused again.
     * @param enabled `true` if fullscreen mode should be used, or `false` if standard windowed mode should
     * be used.
     */
    static void setFullscreenEnabled(bool enabled);
    static bool getFullscreenEnabled();
    /**
     * @brief toggles whether the debug logs should be displayed on top of the screen. this can be useful
     * for debugging but may be removed in future. the 15 most recent calls to `Debug::write` are displayed.
     * @param `true` if debug logs should be overlaid onto the screen, `false` otherwise.
     */
    static void setOverlayLogs(bool enabled) { getInstance()->overlay_logs = enabled; }
    static bool getOverlayLogs() { return getInstance()->overlay_logs; }

    /**
     * @brief instructs the renderer to draw a single scene, filling the window, from now on. the scene will
     * be rendered automatically every frame.
     * @param scene scene which will be rendered from now on.
     */
    static void setSingleScene(const Ref<Scene>& scene);
    /**
     * @brief instructs the renderer to draw multiple scenes, restricted to their own regions, from now on.
     * the scenes will be rendered automatically every frame, and composited into a single image for
     * presentation to the window.
     * @param multi_scenes scenes which will be rendered from now on, and their desired offsets and
     * sizes.
     */
    static void setMultiScene(const std::vector<SceneRender>& multi_scenes);

    /**
     * @brief renders the next frame to the screen. acquires images, prepares materials, and records and
     * executes rendering commands.
     * @returns stuct with statistics relating to frame performance.
     */
    static FrameStats draw();

private:
    RenderServer(bool enable_validation);
    ~RenderServer();

    static RenderServer* getInstance();

    /**
     * @brief initialises GLFW and creates the window.
     */
    void createWindow();
    /**
     * @brief initialises Vulkan and constructs the necessary backend resources, including selecting the
     * physical device to use.
     * @param enable_validation if `true`, render server will attempt to start with Vulkan validation layers
     * enabled, if available.
     */
    void createVulkan(bool enable_validation);
    /**
     * @brief creates Vulkan instance.
     * @param debug if `true` attempts to enable validation layers and the debug messenger extension.
     */
    void createInstance(bool debug);
    /**
     * @brief queries and scores a physical device based on its capabilities, properties, extensions, etc.
     * devices with score of 0 should not be used, as they do not meet requirements.
     */
    static int queryPhysicalDevice(GPUHandle device, std::vector<const char*> extensions);
    /**
     * @brief selects a physical device and creates the logical device on top of it. also extracts queues.
     */
    void createDevice();
    /**
     * @brief requests information from the specified physical device about which queues are available on
     * the device, and what their indices are.
     * @param device physical device handle to query for.
     * @returns struct describing which queues are present and at what indices.
     */
    static QueueFamilies queryQueueFamilies(GPUHandle device);
    /**
     * @brief configures ImGui for displaying debug ui, and hooks it up to vulkan
     */
    void initImGui();

    /**
     * @brief checks for pending swapchain-altering operations: window resize, fullscreen toggle, and vsync
     * toggle, and resizes and recreates internal resources as needed.
     * @param force_resize if `true`, resources will be resized/recreated even if the window size does not
     * appear to have changed.
     * @returns `true` if a resize/recreation operation was actually performed, or `false` if there was
     * nothing to do.
     */
    bool resize(bool force_resize = false);
    /**
     * @brief updates the debug log overlay text.
     */
    void updateTextMesh();
    /**
     * @brief checks if the garbage collector should run, and runs it if so. the garbage collector runs when
     * more than 30 resources are queued for destruction, or when more than 2 seconds as passed (if there is
     * at least one resource queued for destruction), or when `force` is `true`. if the garbage collector
     * runs, this function will block until the GPU is idle.
     * @param force if `true`, the garbage collector will run regardles of how little time has passed or how
     * few resources are queued.
     */
    void tryFreeResources(bool force = false);
    /**
     * @brief records the actual render commands specified by the scenes and other behaviours, in order to
     * draw the net frame.
     * @param image_index index between 0 and `frames_in_flight-1` for the image from the swapchain
     * currently being rendered. used to select command buffer and (later) descriptor sets which will not be
     * in use by other potentially in-flight frames.
     * @param stats statistics struct which will be partially populated by the function.
     * @returns command buffer into which rendering commands were entered, ready to be executed.
     */
    WeakRef<DrawCommandBuffer> recordRenderCommands(uint32_t image_index, FrameStats& stats);

    /**
     * @brief destructs ImGui.
     */
    void destroyImGui();
    /**
     * @brief destroys backend resources and the vulkan instance itself. all graphics resources MUST have
     * already been released by this point.
     */
    void destroyVulkan();
    /**
     * @brief destroys the window and de-initialises GLFW.
     */
    void destroyWindow();
};

} // namespace HopEngine
