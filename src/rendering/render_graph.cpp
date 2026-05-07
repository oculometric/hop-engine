#include "render_graph.h"

#include "command_buffer.h"
#include "engine.h"
#include "framebuffer.h"
#include "material.h"
#include "mesh.h"
#include "render_server.h"
#include "scene.h"
#include "texture.h"

#include <execution>

using namespace HopEngine;

RenderGraph::Step::~Step() {}

RenderGraph::Builder& RenderGraph::Builder::addCamera(const size_t slot)
{ return addCamera(slot, Framebuffer::getDefaultConfig()); }

RenderGraph::Builder& RenderGraph::Builder::addCamera(const size_t slot,
    const Framebuffer::Config& render_pass_config, float size_factor, glm::u32vec2 custom_extent)
{
    Step step;
    step.is_camera        = true;
    step.camera_slot      = slot;
    step.resolution_scale = size_factor;
    step.custom_extent    = custom_extent;
    const glm::vec2 size  = RenderServer::getFramebufferSize();
    if (size_factor > 0.0f)
        step.framebuffer = new Framebuffer(
            { static_cast<uint32_t>(size.x * size_factor), static_cast<uint32_t>(size.y * size_factor) },
            render_pass_config);
    else
        step.framebuffer =
            new Framebuffer({ custom_extent.x ? custom_extent.x : static_cast<uint32_t>(size.x),
                                custom_extent.y ? custom_extent.y : static_cast<uint32_t>(size.y) },
                render_pass_config);
    execution_steps.push_back(step);
    return *this;
}

RenderGraph::Builder& RenderGraph::Builder::addCamera(const size_t slot, const float size_factor,
    const glm::u32vec2 custom_extent)
{ return addCamera(slot, Framebuffer::getDefaultConfig(), size_factor, custom_extent); }

RenderGraph::Builder& RenderGraph::Builder::addPostProcess(const Ref<Shader>& shader,
    const std::map<uint32_t, AttachmentBinding>& texture_bindings)
{ return addPostProcess(shader, texture_bindings, Framebuffer::getCanvasConfig()); }

RenderGraph::Builder& RenderGraph::Builder::addPostProcess(const Ref<Shader>& shader,
    const std::map<uint32_t, AttachmentBinding>& texture_bindings,
    const Framebuffer::Config& render_pass_config, const float size_factor,
    const glm::u32vec2 custom_extent)
{
    Step step;
    step.is_camera        = false;
    step.resolution_scale = size_factor;
    step.custom_extent    = custom_extent;
    const glm::vec2 size  = RenderServer::getFramebufferSize();
    if (size_factor > 0.0f)
        step.framebuffer = new Framebuffer(
            { static_cast<uint32_t>(size.x * size_factor), static_cast<uint32_t>(size.y * size_factor) },
            render_pass_config);
    else
        step.framebuffer =
            new Framebuffer({ custom_extent.x ? custom_extent.x : static_cast<uint32_t>(size.x),
                                custom_extent.y ? custom_extent.y : static_cast<uint32_t>(size.y) },
                render_pass_config);
    step.material         = new Material(shader,
        Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthTest(false).depthWrite(false),
        render_pass_config);
    step.texture_bindings = texture_bindings;
    step.scene_uniforms   = RenderServer::createSceneUniforms();
    execution_steps.push_back(step);
    return *this;
}

RenderGraph::Builder& RenderGraph::Builder::addPostProcess(const Ref<Shader>& shader,
    const std::map<uint32_t, AttachmentBinding>& texture_bindings, const float size_factor,
    const glm::u32vec2 custom_extent)
{
    return addPostProcess(shader, texture_bindings, Framebuffer::getCanvasConfig(), size_factor,
        custom_extent);
}

RenderGraph::RenderGraph(const Builder& config)
{
    passthrough = new Material(Engine::loadShader("res://engine/shaders/passthrough.glsl"),
        Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthWrite(false).depthTest(false), { 0, true });
    passthrough->setSampler(0, Engine::getSampler(config.screen_filtering, Sampler::ADDRESS_CLAMP_EDGE));
    execution_steps = config.execution_steps;
    if (!config.execution_steps.empty())
        expected_extent = config.execution_steps[execution_steps.size() - 1].framebuffer->getExtent();

    rebuildBindings();
}

RenderGraph::~RenderGraph() { DBG_VERBOSE("destroying render graph " + PTR(this)); }

WeakRef<Material> RenderGraph::getMaterialForStep(const size_t step)
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to read material from step " + std::to_string(step) + " of render graph " +
                  PTR(this) + ", but there is no such step");
        return nullptr;
    }

    if (execution_steps[step].is_camera)
    {
        DBG_ERROR("attempt to read material from step " + std::to_string(step) + " of render graph " +
                  PTR(this) + ", but it is not a post-process (material) step");
        return nullptr;
    }

    return execution_steps[step].material;
}

WeakRef<Material> RenderGraph::getMaterialForStep(const std::string& name)
{ return getMaterialForStep(findStep(name)); }

WeakRef<Texture> RenderGraph::getFinalImage() const
{
    if (execution_steps.empty()) return nullptr;
    size_t step_index = output_step % execution_steps.size();
    if (output_step == -1)
    {
        step_index = execution_steps.size() - 1;
        while (step_index > 0 && execution_steps[step_index].skipped) --step_index;
    }

    auto& step              = execution_steps[step_index];
    const size_t attachment = output_image;
    return step.framebuffer->getImage(attachment);
}

bool RenderGraph::getSkipStep(const size_t step) const
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to get skipped for step " + std::to_string(step) + " of render graph " +
                  PTR(this) + ", but there is no such step");
        return false;
    }

    return execution_steps[step].skipped;
}

bool RenderGraph::getSkipStep(const std::string& name) const { return getSkipStep(findStep(name)); }

void RenderGraph::setSkipStep(const size_t step, const bool skip)
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to skip step " + std::to_string(step) + " of render graph " + PTR(this) +
                  ", but there is no such step");
        return;
    }

    execution_steps[step].skipped = skip;
    rebuildBindings();
}

void RenderGraph::setSkipStep(const std::string& name, const bool skip)
{ setSkipStep(findStep(name), skip); }

void RenderGraph::resizeBuffers(glm::u32vec2 new_extent)
{
    if (expected_extent == new_extent) return;
    DBG_VERBOSE("resizing render graph");
    for (Step& step : execution_steps)
    {
        if (step.resolution_scale > 0.0f)
            step.framebuffer->resize(
                { static_cast<uint32_t>(static_cast<float>(new_extent.x) * step.resolution_scale),
                    static_cast<uint32_t>(static_cast<float>(new_extent.y) * step.resolution_scale) });
        else
            step.framebuffer->resize({ step.custom_extent.x ? step.custom_extent.x : new_extent.x,
                step.custom_extent.y ? step.custom_extent.y : new_extent.y });

        if (step.is_camera) continue;

        for (const auto& [texture_index, binding] : step.texture_bindings)
            step.material->setTexture(texture_index,
                execution_steps[binding.step_index].framebuffer->getImage(binding.output_index));
    }
    expected_extent = new_extent;
}

void RenderGraph::draw(WeakRef<DrawCommandBuffer> command_buffer,
    const std::vector<DrawCommand>& draw_commands,
    const std::map<size_t, std::pair<WeakRef<UniformBlock>, glm::vec4>>& cameras)
{
    WeakRef<UniformBlock> last_uniforms = cameras.at(0).first;
    for (const Step& step : execution_steps)
    {
        if (!step.is_camera)
        {
            SceneUniforms* uniforms = reinterpret_cast<SceneUniforms*>(step.scene_uniforms->getBuffer());
            auto& first_input       = execution_steps[(*step.texture_bindings.begin()).first];
            if (first_input.is_camera) last_uniforms = cameras.at(first_input.camera_slot).first;
            else
                last_uniforms = first_input.scene_uniforms;
            if (last_uniforms)
            {
                SceneUniforms* last_uniforms_data =
                    reinterpret_cast<SceneUniforms*>(last_uniforms->getBuffer());
                memcpy(uniforms, last_uniforms_data, sizeof(SceneUniforms));
            }
            uniforms->time          = Engine::getEngineTime();
            uniforms->viewport_size = step.framebuffer->getExtent();
        }
    }

    auto final_image_info                = getFinalImage();
    WeakRef<Texture> new_passthrough_tex = final_image_info;
    if (!new_passthrough_tex) new_passthrough_tex = RenderServer::getDefaultTexture();
    if (new_passthrough_tex != passthrough_texture)
    {
        passthrough->setTexture(0, new_passthrough_tex.strong());
        passthrough_texture = new_passthrough_tex;
        if (new_passthrough_tex->getFormat() == Texture::FORMAT_DEPTH)
            passthrough->setIntUniform("display_depth", 1);
        else
            passthrough->setIntUniform("display_depth", 0);
    }

    std::vector<std::vector<DrawCommand>> step_commands(execution_steps.size());
    for (auto cmd : draw_commands)
    {
        for (size_t i = 0; i < execution_steps.size(); ++i)
        {
            const Step& step = execution_steps[i];
            if (!step.is_camera) continue;
            if (!cmd.material) cmd.material = RenderServer::getDefaultMaterial();
            if (step.framebuffer->isCompatible(cmd.material) && (cmd.camera_mask & (1 << step.camera_slot)))
            {
                step_commands[i].emplace_back(cmd);
            }
        }
    }

    rebuildBindings();

    for (size_t i = 0; i < execution_steps.size(); ++i)
    {
        if (execution_steps[i].skipped) continue;
        if (execution_steps[i].is_camera)
        {
            if (!cameras.contains(execution_steps[i].camera_slot)) continue;
            auto camera = cameras.at(execution_steps[i].camera_slot);
            std::sort(step_commands[i].begin(), step_commands[i].end(),
                [](const DrawCommand& a, const DrawCommand& b) { return DrawCommand::compare(a, b); });
            recordCameraStep(command_buffer, camera.first, camera.second, execution_steps[i].framebuffer,
                step_commands[i]);
        }
        else
            recordPostProcessStep(command_buffer, execution_steps[i].material,
                execution_steps[i].scene_uniforms, execution_steps[i].framebuffer);
    }
}

void RenderGraph::bindOutputMaterial(WeakRef<DrawCommandBuffer> command_buffer)
{ passthrough->bind(command_buffer, false); }

std::map<size_t, glm::u32vec2> RenderGraph::getCameraSlots()
{
    std::map<size_t, glm::u32vec2> slots;
    for (const auto& step : execution_steps)
    {
        if (step.is_camera) slots[step.camera_slot] = step.framebuffer->getExtent();
    }
    return slots;
}

size_t RenderGraph::findStep(const std::string& name) const
{
    size_t index = 0;
    for (auto& step : execution_steps)
    {
        if (step.name == name) return index;
        ++index;
    }

    DBG_ERROR(
        "attempt to find step " + name + " of render graph " + PTR(this) + ", but there is no such step");
    return 0;
}

void RenderGraph::rebuildBindings()
{
    for (Step& step : execution_steps)
    {
        if (step.skipped) continue;
        if (step.is_camera) continue;

        for (const auto& [texture_index, binding] : step.texture_bindings)
        {
            Step binding_step    = execution_steps[binding.step_index];
            Ref<Texture> texture = binding_step.framebuffer->getImage(binding.output_index);
            while (binding_step.skipped)
            {
                if (binding_step.is_camera || binding_step.texture_bindings.empty())
                {
                    texture = RenderServer::getDefaultTexture().strong();
                    break;
                }
                binding_step = execution_steps[binding_step.texture_bindings[0].step_index];
                texture      = binding_step.framebuffer->getImage(0);
            }
            step.material->setTexture(texture_index, texture);
            step.material->setSampler(texture_index,
                Engine::getSampler(binding.filter_mode, binding.address_mode));
        }
    }
}

void RenderGraph::recordCameraStep(WeakRef<DrawCommandBuffer> command_buffer,
    const WeakRef<UniformBlock>& camera, glm::vec4 clear_colour, const WeakRef<Framebuffer>& pass,
    const std::vector<DrawCommand>& commands)
{
    pass->bind(command_buffer, Framebuffer::Clear{ clear_colour });

    for (DrawCommand command : commands)
    {
        auto material = command.material;
        auto mesh     = command.mesh;
        if (!material) material = RenderServer::getDefaultMaterial();
        if (!mesh) mesh = RenderServer::getDefaultMesh();

        // material must be bound before we can start binding uniforms at all
        material->bind(command_buffer);

        camera->bind(command_buffer);
        if (command.uniforms) command.uniforms->bind(command_buffer);
        mesh->draw(command_buffer);
    }
}

void RenderGraph::recordPostProcessStep(WeakRef<DrawCommandBuffer> command_buffer,
    const WeakRef<Material>& material, const WeakRef<UniformBlock>& scene_descriptor_set, const WeakRef<Framebuffer>& pass)
{
    pass->bind(command_buffer, Framebuffer::Clear{ { 0, 0, 0 } });

    // material must be bound before we can start binding uniforms at all
    material->bind(command_buffer, false);
    scene_descriptor_set->bind(command_buffer);

    WeakRef<Mesh> quad = RenderServer::getQuad();
    quad->draw(command_buffer);
}
