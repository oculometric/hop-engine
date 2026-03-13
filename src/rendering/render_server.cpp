#include "render_server.h"

#include "material.h"
#include "uniform_block.h"
#include "render_graph.h"
#include "swapchain.h"
#include "engine.h"
#include "mesh.h"
#include "scene.h"
#include "command_buffer.h"

using namespace HopEngine;
using namespace std;

static RenderServer* server = nullptr;

void RenderServer::init()
{
    DBG_INFO("initialising graphics server");
    if (server == nullptr)
        server = new RenderServer();
}

void RenderServer::destroy()
{
    DBG_INFO("destroying graphics server");
    if (server != nullptr)
    {
        delete server;
        server = nullptr;
    }
}

Ref<UniformBlock> RenderServer::createSceneUniforms()
{
    return new UniformBlock(Shader::Layout{
        server->scene_descriptor_set_layout, 
        {{ 0, Shader::UNIFORM, sizeof(SceneUniforms) } },
        0
    });
}

Ref<UniformBlock> RenderServer::createObjectUniforms()
{
    return new UniformBlock(Shader::Layout{
        server->object_descriptor_set_layout, 
        {{ 0, Shader::UNIFORM, sizeof(ObjectUniforms) } },
        1
    });
}

Ref<RenderPass> RenderServer::getMainRenderPass()
{ return server->offscreen_pass; }

Ref<RenderPass> RenderServer::getFinalRenderPass()
{ return server->final_render_pass; }

Ref<Texture> RenderServer::getDefaultTexture()
{ return server->default_image; }

Ref<Texture> RenderServer::getDefault3DTexture()
{ return server->default_3d_image; }

Ref<Sampler> RenderServer::getDefaultSampler()
{ return server->default_sampler; }

Ref<Mesh> RenderServer::getSkyboxCube()
{ return server->skybox_cube; }

Ref<Mesh> RenderServer::getQuad()
{ return server->quad; }

glm::vec2 RenderServer::getFramebufferSize()
{ return glm::vec2(server->swapchain->getExtent()); }

void RenderServer::setVsyncEnabled(bool enabled)
{
    server->swapchain->setVsync(enabled);
    server->final_render_pass->resize();
}

bool RenderServer::getVsyncEnabled()
{ return server->swapchain->getVsync(); }

void RenderServer::setSingleScene(const Ref<Scene>& scene)
{
    setMultiScene({ { scene, glm::vec2{ 0, 0 }, glm::vec2{ 1, 1 } } });
}

void RenderServer::setMultiScene(const vector<SceneRender>& multi_scenes)
{
    RenderServer::waitIdle();
    server->scenes.clear();
    for (auto& scene : multi_scenes)
        server->scenes.emplace_back(scene);
}

RenderServer::RenderServer()
{
    server = this;

    createWindow();

    createVulkan();

    swapchain = new Swapchain(window_size.x, window_size.y, surface);

    final_render_pass = new RenderPass(swapchain, { 0, true });
    offscreen_pass = new RenderPass(1, 1, { 3, true });

    uint8_t default_image_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    default_image = new Texture(1, 1, Texture::FORMAT_R8G8B8A8_SRGB, Texture::Builder().data(default_image_data));
    uint8_t default_3d_image_data[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    default_3d_image = new Texture(2, 1, Texture::FORMAT_R8G8B8A8_SRGB, Texture::Builder().layers({ 2, 1 }).data(default_3d_image_data));
    default_sampler = new Sampler(Sampler::Builder());

    // quad = new Mesh({
    //                     { { -1, -1, 0, 1 }, {}, {}, {}, { 0, 0 } },
    //                     { { 1, -1, 0, 1 }, {}, {}, {}, { 1, 0 } },
    //                     { { -1, 1, 0, 1 }, {}, {}, {}, { 0, 1 } },
    //                     { { 1, 1, 0, 1 }, {}, {}, {}, { 1, 1 } }
    //                 }, { 0, 3, 1, 0, 2, 3 });
    quad = new Mesh({
        { { -2,  1, 0, 1 }, {}, {}, {}, { -0.5f, 1.0f } },
        { {  2,  1, 0, 1 }, {}, {}, {}, {  1.5f, 1.0f } },
        { {  0, -3, 0, 1 }, {}, {}, {}, {  0.5f, -1.0f } },
    }, { 0, 1, 2 });
    skybox_cube = Mesh::loadMesh("res://engine/meshes/skybox.obj");

    default_material = new Material(new Shader("res://engine/shaders/default_shader.glsl"));
    final_pass_uniforms = createSceneUniforms();
    initImGui();

    DBG_VERBOSE("graphics server initialised");

    draw();
    setVisible(true);
}

RenderServer::~RenderServer()
{
    RenderServer::waitIdle();

    RenderServer::destroyImGui();

    command_buffers.clear();
    scenes.clear();
    final_pass_uniforms = nullptr;
    skybox_cube = nullptr;
    default_material = nullptr;
    quad = nullptr;
    default_image = nullptr;
    default_3d_image = nullptr;
    default_sampler = nullptr;

    offscreen_pass = nullptr;
    final_render_pass = nullptr;
    swapchain = nullptr;

    destroyVulkan();

    destroyWindow();
}

RenderServer* RenderServer::getInstance()
{ return server; }

void RenderServer::recordRenderCommands(uint32_t image_index, FrameStats& stats)
{
    Ref<DrawCommandBuffer> command_buffer = command_buffers[image_index];
    command_buffer->begin(image_index, &stats);
    
    for (auto& scene : scenes)
    {
        if (scene.scene)
            scene.scene->draw(command_buffer, glm::u32vec2(scene.size_uv * glm::vec2(swapchain->getExtent())));
    }
    
    final_render_pass->begin(command_buffer, glm::vec3{ 0.02f, 0.02f, 0.02f });
    
    for (auto& scene : scenes)
    {
        if (scene.scene && scene.scene->render_graph)
        {
            scene.scene->bindOutputMaterial(command_buffer);
            final_pass_uniforms->bind(command_buffer);
        
            command_buffer->setScissorViewport(scene.start_uv, scene.size_uv, swapchain->getExtent());
            quad->draw(command_buffer);
        }
    }
    
    command_buffer->drawImGui();

    command_buffer->end();
}

bool DrawCommand::operator()(const DrawCommand& a, const DrawCommand& b) const
{
    if (a.draw_priority <= b.draw_priority)
        return false;
    if (a.material->getShader() > b.material->getShader())
        return false;
    if (a.material > b.material)
        return false;
    if (a.uniforms > b.uniforms)
        return false;
    if (a.mesh > b.mesh)
        return false;
    return true;
}
