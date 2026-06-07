#include "render_server.h"

#include "command_buffer.h"
#include "engine.h"
#include "framebuffer.h"
#include "material.h"
#include "mesh.h"
#include "render_graph.h"
#include "scene.h"
#include "user_interface.h"
#include "window.h"

#include <thread>

using namespace HopEngine;

constexpr Shader::Descriptor scene_uniform_descriptor{ 0, Shader::UNIFORM, sizeof(SceneUniforms) };
constexpr Shader::Descriptor object_uniform_descriptor{ 0, Shader::UNIFORM, sizeof(ObjectUniforms) };

Ref<UniformBlock> RenderServer::createSceneUniforms()
{
    return new UniformBlock(
        Shader::Layout{ getInstance()->scene_descriptor_set_layout, { scene_uniform_descriptor }, 0 });
}

Ref<UniformBlock> RenderServer::createObjectUniforms()
{
    return new UniformBlock(
        Shader::Layout{ getInstance()->object_descriptor_set_layout, { object_uniform_descriptor }, 1 });
}

uint32_t RenderServer::getFramesInFlight() { return getSwapchain()->getImageCount(); }

glm::vec2 RenderServer::getFramebufferSize() { return glm::vec2(getSwapchain()->getExtent()); }

void RenderServer::setSingleScene(const Ref<Scene>& scene)
{
    setMultiScene({
        { scene, glm::vec2{ 0, 0 }, glm::vec2{ 1, 1 } }
    });
}

void RenderServer::setMultiScene(const std::vector<SceneRender>& multi_scenes)
{
    RenderServer::waitIdle();
    getInstance()->scenes.clear();
    for (auto& scene : multi_scenes) getInstance()->scenes.emplace_back(scene);
}

FrameStats RenderServer::draw()
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

    getInstance()->updateTextMesh();
    getInstance()->tryFreeResources();

    return stats;
}

GPUHandle RenderServer::getRenderPass(const Framebuffer::Config& for_config)
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

Ref<Swapchain> RenderServer::getSwapchain() { return Window::getSwapchain(); }

RenderServer::RenderServer(const InitParams& params, bool& success)
{
    createVulkan(params.enable_api_validation);

    Window::refreshSwapchain();

    scene_descriptor_set_layout   = Shader::createDescriptorSetLayout({ scene_uniform_descriptor });
    object_descriptor_set_layout  = Shader::createDescriptorSetLayout({ object_uniform_descriptor });
    default_descriptor_set_layout = Shader::createDescriptorSetLayout({});
    default_pipeline_layout       = RenderServer::createPipelineLayout(default_descriptor_set_layout);

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

    debug_text_font      = Font::deserialise("res://engine/NASA_worm.hfnt");
    auto debug_ui_style  = new UIStyle();
    debug_ui_style->font = debug_text_font;
    debug_text_renderer  = new UIRenderer(debug_ui_style);

    initImGui();

    DBG_VERBOSE("graphics server initialised");

    draw();
    Window::setVisible(true);

    success = true;
}

RenderServer::~RenderServer()
{
    RenderServer::waitIdle();

    RenderServer::destroyImGui();

    command_buffers.clear();
    scenes.clear();

    debug_text_font     = nullptr;
    debug_text_renderer = nullptr;

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

    render_passes.clear();

    destroyVulkan();
}

void RenderServer::updateTextMesh()
{
    if (!overlay_logs) return;
    auto lines = Debug::queryLines(32);

    debug_text_renderer->clear();
    glm::vec2 position = -glm::vec2(getSwapchain()->getExtent()) / 2.0f;
    for (const auto& line : lines)
        position.y +=
            debug_text_renderer->addText(position, 0, UIRenderer::TextFormatting(), line, { 1, 1, 1 }).y;
    debug_text_renderer->finalise();
}

void RenderServer::tryFreeResources(bool force)
{
    static float last_free_time = 0.0f;

    float current_time = Engine::getEngineTime();
    if (free_list.empty()) return;
    if (!force && (current_time - last_free_time < 2.0f) && free_list.size() < 30) return;

    RenderServer::waitIdle();
    DBG_VERBOSE("freeing " + std::to_string(free_list.size()) + " resources");
    last_free_time = current_time;
    for (auto& item : free_list) item();
    free_list.clear();
}

WeakRef<DrawCommandBuffer> RenderServer::recordRenderCommands(uint32_t image_index, FrameStats& stats)
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

    if (overlay_logs)
    {
        command_buffer->setScissorViewport({ 0, 0 }, { 1, 1 });
        auto command = debug_text_renderer->draw();
        if (command.mesh)
        {
            command.material->bind(command_buffer, false);
            command.mesh->draw(command_buffer);
        }
    }

    command_buffer->drawImGui();

    command_buffer->end();

    return command_buffer;
}
