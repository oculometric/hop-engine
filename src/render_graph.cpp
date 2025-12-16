#include "render_graph.h"

#include "graphics_environment.h"
#include "render_pass.h"
#include "material.h"
#include "uniform_block.h"
#include "pbr.h"
#include "engine.h"
#include "scene.h"
#include "mesh.h"
#include "texture.h"
#include "sampler.h"

using namespace HopEngine;
using namespace std;

RenderGraph::RenderGraph(RenderGraphBuilder config)
{
    execution_steps = config.execution_steps;
    if (!config.execution_steps.empty())
        expected_extent = config.execution_steps[0].render_pass->getExtent();

    for (RenderStep& step : execution_steps)
    {
        if (step.is_camera)
            continue;

        for (const auto& pair : step.texture_bindings)
        {
            step.material->setTexture(pair.first, execution_steps[pair.second.step_index].render_pass->getImage(pair.second.output_index));
            step.material->setSampler(pair.first, new Sampler(SamplerBuilder().filter(pair.second.filter_mode).address(pair.second.address_mode)));
        }
    }
    // TODO: checks for duplicate post process material use, and duplicate render pass use by those materials
}

void RenderGraph::updateUniforms(uint32_t image_index, float time_since_start, Ref<Scene> scene)
{
    for (const RenderStep& step : execution_steps)
    {
        if (step.is_camera)
        {
            VkExtent2D extent = step.render_pass->getExtent();
            scene->getCamera(step.camera_slot)->pushToCameraDescriptorSet(image_index, { extent.width, extent.height}, time_since_start, scene->getLightParams(), glm::vec4(scene->ambient_colour, 0));
        }
        else
            step.material->pushToDescriptorSet(image_index);
    }
}

void RenderGraph::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Scene> scene) const
{
    vector<multiset<DrawCommand, DrawCommand>> step_commands(execution_steps.size());
    auto scene_commands = scene->getDrawCommands();
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
            step_commands[i].insert(DrawCommand(RenderServer::getSkyboxMaterial(), RenderServer::getSkyboxCube()).priority(1000));
        }
    }

    for (size_t i = 0; i < execution_steps.size(); ++i)
    {
        const RenderStep& step = execution_steps[i];
        if (step.is_camera)
            recordCameraStep(command_buffer, image_index, scene->getCamera(step.camera_slot), step.render_pass, step_commands[i]);
        else
            recordPostProcessStep(command_buffer, image_index, step.material, step.scene_uniforms->getDescriptorSet(image_index));
    }
}

void RenderGraph::resizeBuffers(uint32_t width, uint32_t height)
{
    for (RenderStep& step : execution_steps)
    {
        step.render_pass->resize(width, height);
        if (step.is_camera)
            continue;

        for (const auto& pair : step.texture_bindings)
            step.material->setTexture(pair.first, execution_steps[pair.second.step_index].render_pass->getImage(pair.second.output_index));
    }
    expected_extent = { width, height };
}

Ref<Texture> RenderGraph::getFinalImage() const
{
    if (execution_steps.empty())
        return nullptr;
    return execution_steps[output_step % execution_steps.size()].render_pass->getImage(output_image);
}

void RenderGraph::recordCameraStep(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Camera> camera, Ref<RenderPass> pass, std::multiset<DrawCommand, DrawCommand> commands) const
{
    VkRenderPassBeginInfo render_pass_begin_info{ };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = pass->getRenderPass();
    render_pass_begin_info.framebuffer = pass->getFramebuffer(image_index);
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = pass->getExtent();
    vector<VkClearValue> clear_values = pass->getClearValues();
    clear_values[0].color = { camera->clear_colour.r, camera->clear_colour.g, camera->clear_colour.b };
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkRect2D scissor{ };
    scissor.offset = { 0, 0 };
    scissor.extent = pass->getExtent();
    VkViewport viewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(scissor.extent.width);
    viewport.height = static_cast<float>(scissor.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    Ref<Shader> last_used_shader;
    Ref<Material> last_used_material;
    Ref<Mesh> last_used_mesh;
    Ref<UniformBlock> last_used_uniforms;

    VkDescriptorSet descriptor_sets[3] =
    {
        camera->getDescriptorSet(image_index),
        VK_NULL_HANDLE,
        VK_NULL_HANDLE
    };

    for (DrawCommand command : commands)
    {
        if (!command.material || !command.mesh)
        {
            DBG_WARNING("skipping draw command with invalid mesh or material");
            continue;
        }

        bool rebind_material = false;
        bool rebind_object = false;
        bool rebind_layout = false;
        bool rebind_mesh = false;
        if (command.material->getShader() != last_used_shader)
        {
            last_used_shader = command.material->getShader();
            rebind_layout = true;
        }
        if (command.material != last_used_material)
        {
            last_used_material = command.material;
            rebind_material = true;
        }
        if (command.mesh != last_used_mesh)
        {
            last_used_mesh = command.mesh;
            rebind_mesh = true;
        }
        if (command.uniforms != last_used_uniforms)
        {
            last_used_uniforms = command.uniforms;
            rebind_object = true;
        }

        if (rebind_material || rebind_layout)
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, last_used_material->getPipeline());
        if (rebind_layout || rebind_material)
            descriptor_sets[2] = last_used_material->getDescriptorSet(image_index);
        if ((rebind_layout || rebind_object) && last_used_uniforms)
            descriptor_sets[1] = last_used_uniforms->getDescriptorSet(image_index);
        if (rebind_layout || rebind_material || rebind_object)
        {
            if (last_used_uniforms)
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, last_used_material->getPipelineLayout(), 0, 3, descriptor_sets, 0, nullptr);
            else
            {
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, last_used_material->getPipelineLayout(), 0, 1, descriptor_sets, 0, nullptr);
                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, last_used_material->getPipelineLayout(), 2, 1, descriptor_sets + 2, 0, nullptr);
            }
        }

        if (rebind_mesh)
        {
            VkBuffer vertex_buffers[] = { last_used_mesh->getVertexBuffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
            vkCmdBindIndexBuffer(command_buffer, last_used_mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
        }
        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(last_used_mesh->getIndexCount()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(command_buffer);
}

void RenderGraph::recordPostProcessStep(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Material> material, VkDescriptorSet scene_descriptor_set) const
{
    VkRenderPassBeginInfo render_pass_begin_info{ };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = material->getRenderPass()->getRenderPass();
    render_pass_begin_info.framebuffer = material->getRenderPass()->getFramebuffer(image_index);
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = material->getRenderPass()->getExtent();
    vector<VkClearValue> clear_values = material->getRenderPass()->getClearValues();
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkRect2D scissor{ };
    scissor.offset = { 0, 0 };
    scissor.extent = material->getRenderPass()->getExtent();
    VkViewport viewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(scissor.extent.width);
    viewport.height = static_cast<float>(scissor.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipeline());

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(), 0, 1, &scene_descriptor_set, 0, nullptr);
    VkDescriptorSet material_descriptor_set = material->getDescriptorSet(image_index);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(), 2, 1, &material_descriptor_set, 0, nullptr);

    WeakRef<Mesh> quad = RenderServer::getQuad();
    VkBuffer vertex_buffers[] = { quad->getVertexBuffer()};
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, quad->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(quad->getIndexCount()), 1, 0, 0, 0);

    vkCmdEndRenderPass(command_buffer);
}

RenderGraphBuilder RenderGraphBuilder::addCamera(size_t slot)
{
    return addCamera(slot, RenderServer::getMainRenderPass()->getOutputConfig());
}

RenderGraphBuilder RenderGraphBuilder::addCamera(size_t slot, RenderOutput render_pass_config)
{
    RenderStep step;
    step.is_camera = true;
    step.camera_slot = slot;
    glm::vec2 size = RenderServer::getFramebufferSize();
    step.render_pass = new RenderPass((uint32_t)size.x, (uint32_t)size.y, render_pass_config);
    execution_steps.push_back(step);
    return *this;
}

RenderGraphBuilder RenderGraphBuilder::addPostProcess(Ref<Shader> shader, std::map<uint32_t, RenderTextureBinding> texture_bindings)
{
    return addPostProcess(shader, RenderOutput{ 0, true }, texture_bindings);
}

RenderGraphBuilder RenderGraphBuilder::addPostProcess(Ref<Shader> shader, RenderOutput render_pass_config, std::map<uint32_t, RenderTextureBinding> texture_bindings)
{
    RenderStep step;
    step.is_camera = false;
    glm::vec2 size = RenderServer::getFramebufferSize();
    step.render_pass = new RenderPass((uint32_t)size.x, (uint32_t)size.y, render_pass_config);
    step.material = new Material(shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE).depthTest(VK_FALSE).depthWrite(VK_FALSE), step.render_pass);
    step.texture_bindings = texture_bindings;
    step.scene_uniforms = new UniformBlock(ShaderLayout{ RenderServer::getSceneDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(SceneUniforms) }} });
    execution_steps.push_back(step);
    return *this;
}

RenderStep::~RenderStep()
{ }
