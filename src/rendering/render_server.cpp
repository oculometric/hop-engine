#include "render_server.h"

#include "material.h"
#include "uniform_block.h"
#include "render_graph.h"
#include "swapchain.h"
#include "engine.h"
#include "mesh.h"
#include "scene.h"
#include "command_buffer.h"
#include "font.h"

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

glm::vec2 RenderServer::getFramebufferSize()
{ return glm::vec2(server->swapchain->getExtent()); }

void RenderServer::setVsyncEnabled(bool enabled)
{
    server->vsync = enabled;
    server->wants_vsync_update = true;
}

bool RenderServer::getVsyncEnabled()
{ return server->vsync; }

void RenderServer::setFullscreenEnabled(bool enabled)
{
    server->fullscreen = enabled;
    server->wants_fullscreen_update = true;
}

bool RenderServer::getFullscreenEnabled()
{ return server->fullscreen; }

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

    swapchain = new Swapchain(window_size);

    final_render_pass = new RenderPass(swapchain, { 0, true });
    offscreen_pass = new RenderPass({ 1, 1 }, { 3, true });

    uint8_t default_image_data[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
    default_image = new Texture({ 1, 1, 1 }, Texture::FORMAT_SRGB_8X4, default_image_data);
    uint8_t default_3d_image_data[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    default_3d_image = new Texture({ 1, 1, 2 }, Texture::FORMAT_SRGB_8X4, default_3d_image_data);
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

    debug_text_font = new Font("res://engine/textures/font_IBM_XGA_AI_12x23.png", { 14, 25 });
    debug_text_mesh = new Mesh({}, {}, true);
    debug_text_material = new Material(new Shader("res://engine/shaders/screen_space_text.glsl"),
        Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthTest(false).depthTest(false), final_render_pass);
    debug_text_material->setTexture(0, debug_text_font->getAtlas());
    debug_text_material->setSampler(0, Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_LINEAR)));

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

    debug_text_font = nullptr;
    debug_text_mesh = nullptr;
    debug_text_material = nullptr;

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

void RenderServer::updateTextMesh()
{
    if (!overlay_logs)
        return;
    auto lines = Debug::queryLines(32);
    const glm::vec2 uv_size   = debug_text_font->getGlyphUVSize();
    const glm::vec2 char_size = debug_text_font->getGlyphSize();
    glm::vec2 position = { 0, 0 };
    vector<Mesh::Vertex> vertices;
    vector<uint16_t> indices;
    uint16_t v_off = 0;

    glm::vec4 normal_value  = {};
    glm::vec4 colour_value  = glm::vec4{ 1, 1, 1, 1 };

    for (const auto& line : lines)
    {
        for (char c : line)
        {
            const glm::vec2 uv_base = debug_text_font->getGlyphUVOffset(c);
            glm::vec2 uv_br  = uv_base + uv_size; uv_br.y = 1.0f - uv_br.y;
            glm::vec2 uv_tl  = uv_base;           uv_tl.y = 1.0f - uv_tl.y;
            glm::vec2 pos_tl = position;
            glm::vec2 pos_br = pos_tl + char_size;
            
            vertices.push_back(Mesh::Vertex{
                { pos_tl.x, pos_tl.y, 0, 1 },
                colour_value, normal_value, normal_value,
                uv_tl
            });
            vertices.push_back(Mesh::Vertex{
                { pos_br.x, pos_tl.y, 0, 1 },
                colour_value, normal_value, normal_value,
                { uv_br.x, uv_tl.y }
            });
            vertices.push_back(Mesh::Vertex{
                { pos_tl.x, pos_br.y, 0, 1 },
                colour_value, normal_value, normal_value,
                { uv_tl.x, uv_br.y }
            });
            vertices.push_back(Mesh::Vertex{
                { pos_br.x, pos_br.y, 0, 1 },
                colour_value, normal_value, normal_value,
                uv_br
            });

            indices.push_back(v_off + 0);
            indices.push_back(v_off + 3);
            indices.push_back(v_off + 1);
            indices.push_back(v_off + 0);
            indices.push_back(v_off + 2);
            indices.push_back(v_off + 3);
            v_off += 4;

            position.x += char_size.x - 1;
        }
        position.x = 0;
        position.y += char_size.y;
    }
    debug_text_mesh->updateData(vertices, indices, (vertices.size() / 512) * 512, (indices.size() / 512) * 512);
}

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
    
    if (overlay_logs)
    {
        command_buffer->setScissorViewport({ 0, 0 }, { 1, 1 }, swapchain->getExtent());
        debug_text_material->bind(command_buffer, false);
        debug_text_mesh->draw(command_buffer);
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
