#include "render_graph.h"

#include <execution>

#include "render_server.h"
#include "render_pass.h"
#include "material.h"
#include "uniform_block.h"
#include "engine.h"
#include "scene.h"
#include "mesh.h"
#include "texture.h"
#include "command_buffer.h"

using namespace HopEngine;
using namespace std;

RenderStep::~RenderStep()
{
    DBG_BABBLE("destroying render step " + PTR(this));
}

RenderGraphBuilder& RenderGraphBuilder::addCamera(const size_t slot)
{
    return addCamera(slot, RenderServer::getMainRenderPass()->getOutputConfig());
}

RenderGraphBuilder& RenderGraphBuilder::addCamera(const size_t slot, const RenderOutput& render_pass_config, float size_factor, glm::u32vec2 custom_extent)
{
    RenderStep step;
    step.is_camera = true;
    step.camera_slot = slot;
    step.resolution_scale = size_factor;
    step.custom_extent = custom_extent;
    const glm::vec2 size = RenderServer::getFramebufferSize();
    if (size_factor > 0.0f)
        step.render_pass = new RenderPass(static_cast<uint32_t>(size.x * size_factor), static_cast<uint32_t>(size.y * size_factor), render_pass_config);
    else
        step.render_pass = new RenderPass(custom_extent.x ? custom_extent.x : static_cast<uint32_t>(size.x), custom_extent.y ? custom_extent.y : static_cast<uint32_t>(size.y), render_pass_config);
    execution_steps.push_back(step);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addCamera(const size_t slot, const float size_factor, const glm::u32vec2 custom_extent)
{
    return addCamera(slot, RenderServer::getMainRenderPass()->getOutputConfig(), size_factor, custom_extent);
}

RenderGraphBuilder& RenderGraphBuilder::addPostProcess(const Ref<Shader>& shader, const map<uint32_t, RenderTextureBinding>& texture_bindings)
{
    return addPostProcess(shader, texture_bindings, RenderOutput{ 0, false });
}

RenderGraphBuilder& RenderGraphBuilder::addPostProcess(const Ref<Shader>& shader, const map<uint32_t, RenderTextureBinding>& texture_bindings, const RenderOutput& render_pass_config, const float size_factor, const glm::u32vec2 custom_extent)
{
    RenderStep step;
    step.is_camera = false;
    step.resolution_scale = size_factor;
    step.custom_extent = custom_extent;
    const glm::vec2 size = RenderServer::getFramebufferSize();
    if (size_factor > 0.0f)
        step.render_pass = new RenderPass(static_cast<uint32_t>(size.x * size_factor), static_cast<uint32_t>(size.y * size_factor), render_pass_config);
    else
        step.render_pass = new RenderPass(custom_extent.x ? custom_extent.x : static_cast<uint32_t>(size.x), custom_extent.y ? custom_extent.y : static_cast<uint32_t>(size.y), render_pass_config);
    step.material = new Material(shader, Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthTest(false).depthWrite(false), step.render_pass);
    step.texture_bindings = texture_bindings;
    step.scene_uniforms = RenderServer::createSceneUniforms();
    execution_steps.push_back(step);
    return *this;
}

RenderGraphBuilder& RenderGraphBuilder::addPostProcess(const Ref<Shader>& shader, const map<uint32_t, RenderTextureBinding>& texture_bindings, const float size_factor, const glm::u32vec2 custom_extent)
{
    return addPostProcess(shader, texture_bindings, RenderOutput{ 0, false }, size_factor, custom_extent);
}

RenderGraph::RenderGraph(const RenderGraphBuilder& config)
{
    skybox_material = new Material(new Shader("res://engine/shaders/skybox.glsl"), Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthWrite(false).depthTest(false));
    passthrough = new Material(Engine::loadShader("res://engine/shaders/passthrough.glsl"), Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthWrite(false).depthTest(false), RenderServer::getFinalRenderPass());
    passthrough->setSampler(0, Engine::makeSampler(Sampler::Builder().filter(config.screen_filtering).address(Sampler::ADDRESS_CLAMP_EDGE)));
    execution_steps = config.execution_steps;
    if (!config.execution_steps.empty())
        expected_extent = config.execution_steps[0].render_pass->getExtent();

    rebuildBindings();
}

RenderGraph::~RenderGraph()
{
    DBG_VERBOSE("destroying render graph " + PTR(this));
}

Ref<Material> RenderGraph::getMaterialForStep(const size_t step)
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to read material from step " + ::to_string(step) + " of render graph " + PTR(this) + ", but there is no such step");
        return nullptr;
    }

    if (execution_steps[step].is_camera)
    {
        DBG_ERROR("attempt to read material from step " + ::to_string(step) + " of render graph " + PTR(this) + ", but it is not a post-process (material) step");
        return nullptr;
    }

    return execution_steps[step].material;
}

Ref<Material> RenderGraph::getMaterialForStep(const string& name)
{ return getMaterialForStep(findStep(name)); }

pair<Ref<Texture>, bool> RenderGraph::getFinalImage() const
{
    if (execution_steps.empty())
        return { nullptr, false };
    size_t step_index = output_step % execution_steps.size();
    if (output_step == -1)
    {
        step_index = execution_steps.size() - 1;
        while (step_index > 0 && execution_steps[step_index].skipped)
            --step_index;
    }
    
    auto& step = execution_steps[step_index];
    const size_t attachment = output_image;
    const auto [additional_attachments, has_depth_attachment] = step.render_pass->getOutputConfig();
    const size_t max_attachments = additional_attachments + (has_depth_attachment ? 2 : 1);
    bool is_stencil = false;
    if (attachment == max_attachments)
        is_stencil = true;
    return { step.render_pass->getImage(is_stencil ? attachment - 1 : attachment), is_stencil };
}

bool RenderGraph::getSkipStep(const size_t step) const
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to get skipped for step " + ::to_string(step) + " of render graph " + PTR(this) + ", but there is no such step");
        return false;
    }
    
    return execution_steps[step].skipped;
}

bool RenderGraph::getSkipStep(const string& name) const
{ return getSkipStep(findStep(name)); }

void RenderGraph::setSkipStep(const size_t step, const bool skip)
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to skip step " + ::to_string(step) + " of render graph " + PTR(this) + ", but there is no such step");
        return;
    }
    
    execution_steps[step].skipped = skip;
    rebuildBindings();
}

void RenderGraph::setSkipStep(const string& name, const bool skip)
{
    setSkipStep(findStep(name), skip);
}

void RenderGraph::resizeBuffers(const uint32_t width, const uint32_t height)
{
    for (RenderStep& step : execution_steps)
    {
        if (step.resolution_scale > 0.0f)
            step.render_pass->resize(static_cast<uint32_t>(static_cast<float>(width) * step.resolution_scale), static_cast<uint32_t>(static_cast<float>(height) * step.resolution_scale));
        else
            step.render_pass->resize(step.custom_extent.x ? step.custom_extent.x : width, step.custom_extent.y ? step.custom_extent.y : height);

        if (step.is_camera)
            continue;

        for (const auto& [texture_index, binding] : step.texture_bindings)
            step.material->setTexture(texture_index, execution_steps[binding.step_index].render_pass->getImage(binding.output_index));
    }
    expected_extent = { width, height };
}


void RenderGraph::recordCommandBuffer(Ref<DrawCommandBuffer> command_buffer, WeakRef<Scene> scene, FrameStats& stats, glm::u32vec2 viewport_size)
{
    for (const RenderStep& step : execution_steps)
    {
        if (step.is_camera)
        {
            glm::u32vec2 extent = step.render_pass->getExtent();
        }
        else
        {
            glm::u32vec2 extent = execution_steps[0].render_pass->getExtent();
            // FIXME: what the hell is happening here
            SceneUniforms uniforms = scene->getCamera(execution_steps[0].camera_slot)->getSceneUniforms({ extent.x, extent.y }, scene->getLightParams(), glm::vec4(scene->ambient_colour, 0));
            uniforms.viewport_size = { step.render_pass->getExtent().x, step.render_pass->getExtent().y };
            memcpy(step.scene_uniforms->getBuffer(), &uniforms, sizeof(SceneUniforms));
        }
    }
    
    if (scene->skybox && current_skybox != scene->skybox)
    {
        skybox_material->setTexture("tex", scene->skybox);
        current_skybox = scene->skybox;
    }
    
    auto final_image_info = getFinalImage();
    WeakRef<Texture> new_passthrough_tex = final_image_info.first;
    if (!new_passthrough_tex)
        new_passthrough_tex = RenderServer::getDefaultTextureSampler().first;
    static bool in_stencil_mode = false;
    if (new_passthrough_tex != passthrough_texture || final_image_info.second != in_stencil_mode)
    {
        passthrough->setTexture(0, new_passthrough_tex.strong());
        passthrough_texture = new_passthrough_tex;
        in_stencil_mode = final_image_info.second;
        if (in_stencil_mode) passthrough->setTexture(1, new_passthrough_tex.strong(), true);
        else passthrough->setTexture(1, nullptr);
        if (new_passthrough_tex->getFormat() == Texture::getDepthFormat())
            passthrough->setIntUniform("display_depth", in_stencil_mode ? 2 : 1);
        else
            passthrough->setIntUniform("display_depth", 0);
    }

    vector<multiset<DrawCommand, DrawCommand>> step_commands(execution_steps.size());
    auto scene_commands = scene->getDrawCommands(viewport_size);
    for (const auto& cmd : scene_commands)
    {
        for (size_t i = 0; i < execution_steps.size(); ++i)
        {
            const RenderStep& step = execution_steps[i];
            if (!step.is_camera)
                continue;
            if (cmd.material->getRenderPass()->isCompatible(step.render_pass) && (cmd.camera_mask & (1 << step.camera_slot)))
                step_commands[i].insert(cmd);
        }
    }

    if (scene->skybox)
    {
        for (size_t i = 0; i < execution_steps.size(); ++i)
        {
            if (!execution_steps[i].is_camera)
                continue;
            step_commands[i].insert(DrawCommand(skybox_material, RenderServer::getSkyboxCube().weak()).priority(1000));
        }
    }

    for (size_t i = 0; i < execution_steps.size(); ++i)
    {
        if (execution_steps[i].skipped)
            continue;
        if (execution_steps[i].is_camera)
        {
            recordCameraStep(command_buffer, scene->getCamera(execution_steps[i].camera_slot), execution_steps[i].render_pass, step_commands[i],
                scene->getLightParams(), glm::vec4(scene->ambient_colour, 0));
            ++stats.cameras;
        }
        else
            recordPostProcessStep(command_buffer, execution_steps[i].material, execution_steps[i].scene_uniforms);
    }
}

void RenderGraph::bind(Ref<DrawCommandBuffer> command_buffer)
{
    passthrough->bind(command_buffer, false);
}

size_t RenderGraph::findStep(const string& name) const
{
    size_t index = 0;
    for (auto& step : execution_steps)
    {
        if (step.name == name)
            return index;
        ++index;
    }
    
    DBG_ERROR("attempt to find step " + name + " of render graph " + PTR(this) + ", but there is no such step");
    return 0;
}

void RenderGraph::rebuildBindings()
{
    for (RenderStep& step : execution_steps)
    {
        if (step.skipped)
            continue;
        if (step.is_camera)
            continue;

        for (const auto& [texture_index, binding] : step.texture_bindings)
        {
            RenderStep binding_step = execution_steps[binding.step_index];
            Ref<Texture> texture = binding_step.render_pass->getImage(binding.output_index);
            while (binding_step.skipped)
            {
                if (binding_step.is_camera || binding_step.texture_bindings.empty())
                {
                    texture = RenderServer::getDefaultTextureSampler().first;
                    break;                    
                }
                binding_step = execution_steps[binding_step.texture_bindings[0].step_index];
                texture = binding_step.render_pass->getImage(0);
            }
            step.material->setTexture(texture_index, texture);
            step.material->setSampler(texture_index, Engine::makeSampler(Sampler::Builder().filter(binding.filter_mode).address(binding.address_mode)));
        }
    }
}

void RenderGraph::recordCameraStep(const Ref<DrawCommandBuffer> command_buffer, const Ref<Camera>& camera, const Ref<RenderPass>& pass, const multiset<DrawCommand, DrawCommand>& commands, const vector<LightParams>& lights, glm::vec4 ambient_colour)
{
    pass->begin(command_buffer, camera->clear_colour);

    for (DrawCommand command : commands)
    {
        if (!command.material || !command.mesh || !command.mesh->isRenderable())
        {
            DBG_WARNING("skipping draw command with invalid mesh or material");
            continue;
        }
        
        command.material->bind(command_buffer);

        camera->bind(command_buffer, pass->getExtent(), lights, ambient_colour);
        if (command.uniforms)
            command.uniforms->bind(command_buffer, 1);
        command.mesh->draw(command_buffer);
    }
}

void RenderGraph::recordPostProcessStep(Ref<DrawCommandBuffer> command_buffer, const Ref<Material>& material, const Ref<UniformBlock>& scene_descriptor_set)
{
    material->getRenderPass()->begin(command_buffer, { 0, 0, 0 });
    
    material->bind(command_buffer, false);
    scene_descriptor_set->bind(command_buffer, 0);
    
    WeakRef<Mesh> quad = RenderServer::getQuad();
    quad->draw(command_buffer);
}
