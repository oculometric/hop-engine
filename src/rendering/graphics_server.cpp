#include "graphics_server.h"

#include "command_buffer.h"
#include "engine.h"
#include "framebuffer.h"
#include "material.h"
#include "mesh.h"
#include "render_graph.h"
#include "scene.h"
#include "user_interface.h"
#include "window.h"

#include <format>
#include <thread>

using namespace HopEngine;

constexpr Shader::Descriptor scene_uniform_descriptor{ 0, Shader::UNIFORM, sizeof(SceneUniforms) };
constexpr Shader::Descriptor object_uniform_descriptor{ 0, Shader::UNIFORM, sizeof(ObjectUniforms) };

Ref<UniformBlock> GraphicsServer::createSceneUniforms()
{
    return new UniformBlock(
        Shader::Layout{ getInstance()->scene_descriptor_set_layout, { scene_uniform_descriptor }, 0 });
}

Ref<UniformBlock> GraphicsServer::createObjectUniforms()
{
    return new UniformBlock(
        Shader::Layout{ getInstance()->object_descriptor_set_layout, { object_uniform_descriptor }, 1 });
}

uint32_t GraphicsServer::getFramesInFlight() { return getSwapchain()->getImageCount(); }

glm::vec2 GraphicsServer::getFramebufferSize() { return glm::vec2(getSwapchain()->getExtent()); }

void GraphicsServer::setSingleScene(const Ref<Scene>& scene)
{
    setMultiScene({
        { scene, glm::vec2{ 0, 0 }, glm::vec2{ 1, 1 } }
    });
}

void GraphicsServer::setMultiScene(const std::vector<SceneRender>& multi_scenes)
{
    GraphicsServer::waitIdle();
    getInstance()->scenes.clear();
    for (auto& scene : multi_scenes) getInstance()->scenes.emplace_back(scene);
}

FrameStats GraphicsServer::draw()
{
    if (Window::isMinimised())
    {
        std::this_thread::sleep_for(std::chrono::duration<float>(0.125f));
        return {};
    }
    if (Window::refreshSwapchain())
    {
        getInstance()->destroyImGui();
        getInstance()->initImGui();

        Input::applyCallbackBindings();
        return {};
    }

    FrameStats stats{};

    uint32_t image_index = getSwapchain()->acquireNextImage();
    if (image_index == (uint32_t)-1) return {};

    const auto record_start = std::chrono::steady_clock::now();
    auto command_buffer     = getInstance()->recordRenderCommands(image_index, stats);
    const std::chrono::duration<float> record_duration = std::chrono::steady_clock::now() - record_start;
    stats.record_time                                  = record_duration.count();

    getSwapchain()->submitCommands(command_buffer, image_index);

    command_buffer->extractTiming();

    getInstance()->tryFreeResources();

    return stats;
}

GPUHandle GraphicsServer::getRenderPass(const Framebuffer::Config& for_config)
{
    auto it = getInstance()->render_passes.find(for_config);
    if (it == getInstance()->render_passes.end())
    {
        Ref<RenderPass> new_pass                 = new RenderPass(for_config);
        getInstance()->render_passes[for_config] = new_pass;
        return new_pass->getRenderPass();
    }
    return it->second->getRenderPass();
}

Ref<Swapchain> GraphicsServer::getSwapchain() { return Window::getSwapchain(); }

GraphicsServer::GraphicsServer(const InitParams& params, bool& success)
{
    createVulkan(params.enable_api_validation);

    Window::refreshSwapchain();

    scene_descriptor_set_layout   = Shader::createDescriptorSetLayout({ scene_uniform_descriptor });
    object_descriptor_set_layout  = Shader::createDescriptorSetLayout({ object_uniform_descriptor });
    default_descriptor_set_layout = Shader::createDescriptorSetLayout({});
    default_pipeline_layout       = GraphicsServer::createPipelineLayout(default_descriptor_set_layout);

    const uint32_t default_image_data[2 * 2 * 2] = {
        0xFF000000,
        0xFFFF00FF,
        0xFFFF00FF,
        0xFF000000,
        0xFFFF00FF,
        0xFF000000,
        0xFF000000,
        0xFFFF00FF,
    };
    default_image    = new Texture({ 2, 2, 1 }, Texture::FORMAT_SRGB_8X4, default_image_data);
    default_3d_image = new Texture({ 2, 2, 2 }, Texture::FORMAT_SRGB_8X4, default_image_data);
    default_sampler  = new Sampler(Sampler::FILTER_NEAREST, Sampler::ADDRESS_REPEAT);

    quad = new Mesh(
        {
            { { -1, -1, 0, 1 }, {}, {}, {}, { 0, 0, 0 } },
            {  { 1, -1, 0, 1 }, {}, {}, {}, { 1, 0, 0 } },
            {  { -1, 1, 0, 1 }, {}, {}, {}, { 0, 1, 0 } },
            {   { 1, 1, 0, 1 }, {}, {}, {}, { 1, 1, 0 } }
    },
        { 0, 3, 1, 0, 2, 3 });
    tri = new Mesh(
        {
            { { -2, 1, 0, 1 }, {}, {}, {}, { -0.5f, 1.0f, 0 } },
            {  { 2, 1, 0, 1 }, {}, {}, {},  { 1.5f, 1.0f, 0 } },
            { { 0, -3, 0, 1 }, {}, {}, {}, { 0.5f, -1.0f, 0 } },
    },
        { 0, 1, 2 });
    skybox_cube  = Mesh::loadMesh("res://engine/meshes/skybox.obj");
    sky_sphere   = Mesh::loadMesh("res://engine/meshes/sky_sphere.obj");
    default_mesh = Mesh::loadMesh("res://engine/meshes/default_mesh.obj");
    if (!default_mesh)
    {
        success = false;
        DBG_FAULT("failed to load default mesh!");
    }

    default_material    = new Material(Engine::loadShader("res://engine/shaders/default_shader.glsl"));
    final_pass_uniforms = createSceneUniforms();
    spinner_material    = new Material(Engine::loadShader("res://engine/shaders/screen_space_image.glsl"),
        Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthWrite(false).depthTest(false),
        getSwapchain()->getFramebuffer()->getConfig());
    spinner_uniforms    = createObjectUniforms();
    ObjectUniforms* spinner = static_cast<ObjectUniforms*>(spinner_uniforms->getBuffer());
    spinner->model_to_world = glm::mat4(1);
    spinner_material->setTexture(0, Engine::loadTexture("res://engine/icon.png"));
    spinner_material->setSampler(0,
        Engine::getSampler(Sampler::FILTER_NEAREST, Sampler::ADDRESS_CLAMP_EDGE));

    createPerformanceOverlay();

    initImGui();

    DBG_VERBOSE("graphics server initialised");

    draw();
    Window::setVisible(true);

    success = true;
}

GraphicsServer::~GraphicsServer()
{
    GraphicsServer::waitIdle();

    GraphicsServer::destroyImGui();

    command_buffers.clear();
    scenes.clear();

    spinner_uniforms    = nullptr;
    spinner_material    = nullptr;
    final_pass_uniforms = nullptr;
    skybox_cube         = nullptr;
    sky_sphere          = nullptr;
    default_material    = nullptr;
    quad                = nullptr;
    tri                 = nullptr;
    default_mesh        = nullptr;
    default_image       = nullptr;
    default_3d_image    = nullptr;
    default_sampler     = nullptr;
    performance_overlay = nullptr;

    render_passes.clear();

    destroyVulkan();
}

void GraphicsServer::createPerformanceOverlay()
{
    performance_overlay = new UICanvas();
    performance_overlay->setWorldSpace(false, getSwapchain()->getFramebuffer()->getConfig());
    auto background_panel = performance_overlay->addElement<UIPanel>();
    background_panel->setScaling(UITransform::SCALING_FILL_BOTH);
    background_panel->setStyle(3);
    background_panel->setColour({ 0.8f, 0.8f, 0.8f, 0.01f });
    {
        auto panel = performance_overlay->addElement<UIPanel>();
        panel->setSize({ 156, (25 * 3) + 6 });
        panel->setColour({ 0.03f, 0.03f, 0.03f, 0.5f });
        auto parent = performance_overlay->addChild<UICanvasElement>(panel);
        parent->setPosition({ 6, 6 });
        parent->setSize({ 156 - 12, 25 * 3 });
        auto label = performance_overlay->addChild<UILabel>(parent);
        label->setText("HOP-ENGINE");
        label->setPosition({ 0, 0 });
        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("v" + HOP_ENGINE_VERSION_STRING);
        label->setPosition({ 0, 25 });
        label = performance_overlay->addChild<UILabel>(parent);
        label->setText(HOP_ENGINE_COMMIT_STRING);
        label->setPosition({ 0, 25 * 2 });
    }

    {
        system_panel = performance_overlay->addElement<UIPanel>();
        system_panel->setExternalAnchor(UITransform::ANCHOR_MIDDLE_LEFT);
        system_panel->setSize({ 200, (25 * 4) + 6 });
        system_panel->setColour({ 0.03f, 0.03f, 0.03f, 0.5f });
        auto parent = performance_overlay->addChild<UICanvasElement>(system_panel);
        parent->setPosition({ 6, 6 });
        parent->setSize({ 200 - 12, 25 * 4 });

        auto label = performance_overlay->addChild<UILabel>(parent);
        label->setText("CPU");
        label->setPosition({ 0, 0 });
        cpu_label = performance_overlay->addChild<UILabel>(parent);
        cpu_label->setText("00.0%");
        cpu_label->setPosition({ 0, 0 });
        cpu_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        cpu_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        cpu_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("CPUMEM");
        label->setPosition({ 0, 25 });
        memory_label = performance_overlay->addChild<UILabel>(parent);
        memory_label->setText("00.0MB");
        memory_label->setPosition({ 0, 25 });
        memory_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        memory_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        memory_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("GPU");
        label->setPosition({ 0, 25 * 2 });
        gpu_label = performance_overlay->addChild<UILabel>(parent);
        gpu_label->setText("00.0%");
        gpu_label->setPosition({ 0, 25 * 2 });
        gpu_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        gpu_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        gpu_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("GPUMEM");
        label->setPosition({ 0, 25 * 3 });
        gpu_memory_label = performance_overlay->addChild<UILabel>(parent);
        gpu_memory_label->setText("00.0MB");
        gpu_memory_label->setPosition({ 0, 25 * 3 });
        gpu_memory_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        gpu_memory_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        gpu_memory_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });
    }

    {
        graphics_panel = performance_overlay->addElement<UIPanel>();
        graphics_panel->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        graphics_panel->setInternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        graphics_panel->setSize({ 300, (25 * 4) + 6 });
        graphics_panel->setColour({ 0.03f, 0.03f, 0.03f, 0.5f });
        auto parent = performance_overlay->addChild<UICanvasElement>(graphics_panel);
        parent->setPosition({ 6, 6 });
        parent->setSize({ 300 - 12, 25 * 4 });

        auto label = performance_overlay->addChild<UILabel>(parent);
        label->setText("RESOLUTION");
        label->setPosition({ 0, 0 });
        window_size_label = performance_overlay->addChild<UILabel>(parent);
        window_size_label->setText("1024x1024");
        window_size_label->setPosition({ 0, 0 });
        window_size_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        window_size_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        window_size_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("PASSES");
        label->setPosition({ 0, 25 });
        render_pass_count_label = performance_overlay->addChild<UILabel>(parent);
        render_pass_count_label->setText("4");
        render_pass_count_label->setPosition({ 0, 25 });
        render_pass_count_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        render_pass_count_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        render_pass_count_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("DRAWCALLS");
        label->setPosition({ 0, 25 * 2 });
        draw_call_count_label = performance_overlay->addChild<UILabel>(parent);
        draw_call_count_label->setText("19");
        draw_call_count_label->setPosition({ 0, 25 * 2 });
        draw_call_count_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        draw_call_count_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        draw_call_count_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("TRIS");
        label->setPosition({ 0, 25 * 3 });
        tri_count_label = performance_overlay->addChild<UILabel>(parent);
        tri_count_label->setText("4557");
        tri_count_label->setPosition({ 0, 25 * 3 });
        tri_count_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        tri_count_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        tri_count_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });
    }

    {
        timing_panel = performance_overlay->addElement<UIPanel>();
        timing_panel->setExternalAnchor(UITransform::ANCHOR_MIDDLE_RIGHT);
        timing_panel->setInternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        timing_panel->setSize({ 256, (25 * 5) + 6 });
        timing_panel->setColour({ 0.03f, 0.03f, 0.03f, 0.5f });
        auto parent = performance_overlay->addChild<UICanvasElement>(timing_panel);
        parent->setPosition({ 6, 6 });
        parent->setSize({ 256 - 12, 25 * 5 });

        auto label = performance_overlay->addChild<UILabel>(parent);
        label->setText("FPS");
        label->setPosition({ 0, 0 });
        fps_label = performance_overlay->addChild<UILabel>(parent);
        fps_label->setText("352.0");
        fps_label->setPosition({ 0, 0 });
        fps_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        fps_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        fps_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("DELTA");
        label->setPosition({ 0, 25 });
        delta_time_label = performance_overlay->addChild<UILabel>(parent);
        delta_time_label->setText("2.35MS");
        delta_time_label->setPosition({ 0, 25 });
        delta_time_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        delta_time_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        delta_time_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("FRAMEFREE");
        label->setPosition({ 0, 25 * 2 });
        frame_free_label = performance_overlay->addChild<UILabel>(parent);
        frame_free_label->setText("43.0%");
        frame_free_label->setPosition({ 0, 25 * 2 });
        frame_free_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        frame_free_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        frame_free_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("TIME");
        label->setPosition({ 0, 25 * 3 });
        time_elapsed_label = performance_overlay->addChild<UILabel>(parent);
        time_elapsed_label->setText("23.5S");
        time_elapsed_label->setPosition({ 0, 25 * 3 });
        time_elapsed_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        time_elapsed_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        time_elapsed_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });

        label = performance_overlay->addChild<UILabel>(parent);
        label->setText("FRAME");
        label->setPosition({ 0, 25 * 4 });
        frame_index_label = performance_overlay->addChild<UILabel>(parent);
        frame_index_label->setText("24353");
        frame_index_label->setPosition({ 0, 25 * 4 });
        frame_index_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        frame_index_label->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
        frame_index_label->setFormatting(UIRenderer::TextFormatting{ UIRenderer::TEXT_ALIGN_RIGHT });
    }

    {
        logs_panel = performance_overlay->addElement<UIPanel>();
        logs_panel->setExternalAnchor(UITransform::ANCHOR_BOTTOM_LEFT);
        logs_panel->setInternalAnchor(UITransform::ANCHOR_BOTTOM_LEFT);
        logs_panel->setSize({ 768, (25 * 8) + 6 });
        logs_panel->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
        logs_panel->setColour({ 0.03f, 0.03f, 0.03f, 0.5f });
        auto parent = performance_overlay->addChild<UICanvasElement>(logs_panel);
        parent->setPosition({ 6, 6 });
        parent->setSize({ 768 - 16, 25 * 8 });
        parent->setScaling(UITransform::SCALING_FILL_HORIZONTAL);

        logs_label = performance_overlay->addChild<UILabel>(parent);
        logs_label->setText("line 1\nline2\nlin3\nline4\nline5\nline6\nlin7\nline8");
        logs_label->setPosition({ 0, 0 });
        UIRenderer::TextFormatting formatting;
        formatting.align = UIRenderer::TEXT_ALIGN_LEFT;
        formatting.clip = true;
        logs_label->setScaling(UITransform::SCALING_FILL_BOTH);
        logs_label->setFormatting(formatting);
        logs_label->setExternalAnchor(UITransform::ANCHOR_TOP_LEFT);
        logs_label->setInternalAnchor(UITransform::ANCHOR_TOP_LEFT);
    }
}

void GraphicsServer::updatePerformanceOverlay()
{
    {
        cpu_label->setText(std::format("{:4.1f}%", Engine::getCPUUsagePercent()));
        memory_label->setText(std::format("{:5.0f}mb", Engine::getMemoryUsageMegabytes()));
        gpu_label->setText(std::format("{:4.1f}%", Engine::getGPUUsagePercent()));
        gpu_memory_label->setText(std::format("{:5.0f}mb", Engine::getGPUMemoryUsageMegabytes()));
    }
    {
        glm::u32vec2 size = GraphicsServer::getFramebufferSize();
        window_size_label->setText(std::format("{}x{}", size.x, size.y));
        auto stats = Engine::getFrameStats();
        render_pass_count_label->setText(std::format("{}", stats.passes));
        draw_call_count_label->setText(std::format("{}", stats.draw_calls));
        tri_count_label->setText(std::format("{}", stats.triangles));
    }
    {
        fps_label->setText(std::format("{:4.1f}", Engine::getSmoothedFPS()));
        delta_time_label->setText(std::format("{:4.1f}ms", Engine::getSmoothedDeltaTime() * 1000.0f));
        frame_free_label->setText(std::format("{:3.1f}%", Engine::getFrameFreePercent()));
        time_elapsed_label->setText(std::format("{:5.1f}s", Engine::getEngineTime()));
        frame_index_label->setText(std::format("{}", Engine::getFrameCount()));
    }
    {
        auto lines = Debug::queryLines(8);
        std::string complete;
        for (const std::string& line : lines) complete = complete + line + "\n";
        logs_label->setText(complete);
    }
}

void GraphicsServer::tryFreeResources(bool force)
{
    static float last_free_time = 0.0f;

    float current_time = Engine::getEngineTime();
    if (free_list.empty()) return;
    if (!force && (current_time - last_free_time < 2.0f) && free_list.size() < 30) return;

    GraphicsServer::waitIdle();
    DBG_VERBOSE("freeing " + std::to_string(free_list.size()) + " resources");
    last_free_time = current_time;
    for (auto& item : free_list) item();
    free_list.clear();
}

WeakRef<DrawCommandBuffer> GraphicsServer::recordRenderCommands(uint32_t image_index, FrameStats& stats)
{
    SceneUniforms scene_uniforms;
    scene_uniforms.time          = Engine::getEngineTime();
    scene_uniforms.eye_position  = { 0, 0, 0 };
    scene_uniforms.viewport_size = getSwapchain()->getExtent();
    scene_uniforms.world_to_view = glm::mat4(1);
    scene_uniforms.view_to_clip  = glm::mat4(1);
    scene_uniforms.clip_to_view  = glm::mat4(1);
    scene_uniforms.view_to_world = glm::mat4(1);
    scene_uniforms.near_far      = { -1, 1 };
    memcpy(final_pass_uniforms->getBuffer(), &scene_uniforms, sizeof(SceneUniforms));

    ObjectUniforms* spinner = static_cast<ObjectUniforms*>(spinner_uniforms->getBuffer());
    spinner->model_to_world =
        glm::scale(glm::rotate(glm::translate(glm::mat4(1),
                                   glm::vec3{ glm::vec2(getSwapchain()->getExtent()) / 2.0f, 0.0f }),
                       Engine::getEngineTime(), glm::vec3{ 0, 0, 1 }),
            glm::vec3(256, 256, 1));

    Ref<DrawCommandBuffer> command_buffer = command_buffers[image_index];
    command_buffer->begin(image_index, &stats);

    for (auto& scene : scenes)
    {
        if (scene.scene)
            scene.scene->draw(command_buffer,
                glm::u32vec2(scene.size_uv * glm::vec2(getSwapchain()->getExtent())));
    }

    getSwapchain()->getFramebuffer()->bind(command_buffer, Framebuffer::Clear{
                                                               { 0.02f, 0.02f, 0.02f },
                                                               !scenes.empty()
    });

    if (scenes.empty())
    {
        final_pass_uniforms->bind(command_buffer);
        spinner_uniforms->bind(command_buffer);
        spinner_material->bind(command_buffer, false);
        quad->draw(command_buffer);
    }

    for (auto& scene : scenes)
    {
        if (scene.scene && scene.scene->render_graph)
        {
            final_pass_uniforms->bind(command_buffer);
            command_buffer->setScissorViewport(scene.start_uv, scene.size_uv);
            scene.scene->bindOutputMaterial(command_buffer);
            tri->draw(command_buffer);
        }
    }

    UIManager::draw(command_buffer);

    if (diagnostic_overlay)
    {
        updatePerformanceOverlay();
        command_buffer->setScissorViewport(glm::vec2(0.0f), glm::vec2(1.0f));
        if (performance_overlay->getSize() != GraphicsServer::getFramebufferSize())
            performance_overlay->resize(GraphicsServer::getFramebufferSize());
        performance_overlay->build();
        auto command = performance_overlay->draw();
        command.material->bind(command_buffer);
        command.mesh->draw(command_buffer);
    }

    command_buffer->drawImGui();

    command_buffer->end();

    return command_buffer;
}
