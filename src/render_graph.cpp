#include "render_graph.h"

#include <execution>
#include <string>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

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

void RenderGraph::updateUniforms(uint32_t image_index, float time_since_start, Ref<Scene> scene)
{
    for (const RenderStep& step : execution_steps)
    {
        if (step.is_camera)
        {
            VkExtent2D extent = step.render_pass->getExtent();
            scene->getCamera(step.camera_slot)->pushToCameraDescriptorSet(image_index, { extent.width, extent.height }, time_since_start, scene->getLightParams(), glm::vec4(scene->ambient_colour, 0));
        }
        else
        {
            VkExtent2D extent = execution_steps[0].render_pass->getExtent();
            step.material->pushToDescriptorSet(image_index);
            SceneUniforms uniforms = scene->getCamera(execution_steps[0].camera_slot)->getSceneUniforms({ extent.width, extent.height }, time_since_start, scene->getLightParams(), glm::vec4(scene->ambient_colour, 0));
            uniforms.viewport_size = { step.render_pass->getExtent().width, step.render_pass->getExtent().height };
            memcpy(step.scene_uniforms->getBuffer(), &uniforms, sizeof(SceneUniforms));
            step.scene_uniforms->pushToDescriptorSet(image_index);
        }
    }
    
    auto final_image_info = getFinalImage();
    WeakRef<Texture> new_passthrough_tex = final_image_info.first;
    if (!new_passthrough_tex)
        new_passthrough_tex = RenderServer::getDefaultTextureSampler().first;
    static bool in_stencil_mode = false;
    if (new_passthrough_tex != passthrough_texture || final_image_info.second != in_stencil_mode)
    {
        passthrough->setTexture(0, new_passthrough_tex);
        passthrough_texture = new_passthrough_tex;
        in_stencil_mode = final_image_info.second;
        if (in_stencil_mode) passthrough->setTexture(1, new_passthrough_tex, true);
        else passthrough->setTexture(1, nullptr);
        if (new_passthrough_tex->getFormat() == Texture::depth_format)
            passthrough->setIntUniform("display_depth", in_stencil_mode ? 2 : 1);
        else
            passthrough->setIntUniform("display_depth", 0);
    }
    passthrough->pushToDescriptorSet(image_index);
}

void RenderGraph::recordCommandBuffer(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Scene> scene, FrameStats& stats, Ref<RenderPass> final_render_pass) const
{
    vector<multiset<DrawCommand, DrawCommand>> step_commands(execution_steps.size() + 1);
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
        if (cmd.material->getRenderPass()->isCompatible(final_render_pass) && cmd.camera_mask & 0xF0000000)
            step_commands[execution_steps.size()].insert(cmd);
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
        if (step.skipped)
            continue;
        if (step.is_camera)
            recordCameraStep(command_buffer, image_index, scene->getCamera(step.camera_slot), step.render_pass, step_commands[i], stats);
        else
            recordPostProcessStep(command_buffer, image_index, step.material, step.scene_uniforms->getDescriptorSet(image_index), stats);
    }
    
    VkRenderPassBeginInfo render_pass_begin_info{ };
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = final_render_pass->getRenderPass();
    render_pass_begin_info.framebuffer = final_render_pass->getFramebuffer(image_index);
    render_pass_begin_info.renderArea.offset = { 0, 0 };
    render_pass_begin_info.renderArea.extent = final_render_pass->getExtent();
    vector<VkClearValue> clear_values = final_render_pass->getClearValues();
    clear_values[0].color = { 0.02f, 0.02f, 0.02f };
    render_pass_begin_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_begin_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
    RenderServer::writeTimestamp(image_index, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    stats.passes++;

    VkRect2D scissor{ };
    scissor.offset = { 0, 0 };
    scissor.extent = final_render_pass->getExtent();
    VkViewport viewport{ };
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(scissor.extent.width);
    viewport.height = static_cast<float>(scissor.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    VkDescriptorSet descriptor_sets[3] =
    {
        scene->getCamera(execution_steps[0].camera_slot)->getDescriptorSet(image_index),
        VK_NULL_HANDLE,
        VK_NULL_HANDLE
    };
    
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passthrough->getPipeline());
    stats.pipeline_rebinds++;

    descriptor_sets[2] = passthrough->getDescriptorSet(image_index);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passthrough->getPipelineLayout(), 0, 1, descriptor_sets, 0, nullptr);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, passthrough->getPipelineLayout(), 2, 1, descriptor_sets + 2, 0, nullptr);
    
    WeakRef<Mesh> quad = RenderServer::getQuad();
    VkBuffer vertex_buffers[] = { quad->getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, quad->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(quad->getIndexCount()), 1, 0, 0, 0);
    stats.draw_calls++;
    stats.triangles += 2;
    stats.vertices += 4;
    
    auto final_step = step_commands[execution_steps.size()];
    for (DrawCommand command : final_step)
    {
        descriptor_sets[1] = command.uniforms->getDescriptorSet(image_index);
        descriptor_sets[2] = command.material->getDescriptorSet(image_index);
        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command.material->getPipeline());
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, command.material->getPipelineLayout(), 0, 3, descriptor_sets, 0, nullptr);
        VkBuffer vertex_buffers[] = { command.mesh->getVertexBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(command_buffer, command.mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(command.mesh->getIndexCount()), 1, 0, 0, 0);
        stats.draw_calls++;
        stats.triangles += command.mesh->getIndexCount() / 3;
        stats.vertices += command.mesh->getVertexCount();
    }

    ImDrawData* draw_data = ImGui::GetDrawData();
    if (draw_data) ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);

    RenderServer::writeTimestamp(image_index, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    vkCmdEndRenderPass(command_buffer);
}

void RenderGraph::resizeBuffers(uint32_t width, uint32_t height)
{
    for (RenderStep& step : execution_steps)
    {
        if (step.resolution_scale > 0.0f)
            step.render_pass->resize((uint32_t)(width * step.resolution_scale), (uint32_t)(height * step.resolution_scale));
        else
            step.render_pass->resize(step.custom_extent.width ? step.custom_extent.width : (uint32_t)width, step.custom_extent.height ? step.custom_extent.height : height);

        if (step.is_camera)
            continue;

        for (const auto& pair : step.texture_bindings)
            step.material->setTexture(pair.first, execution_steps[pair.second.step_index].render_pass->getImage(pair.second.output_index));
    }
    expected_extent = { width, height };
}

pair<Ref<Texture>, bool> RenderGraph::getFinalImage() const
{
    if (execution_steps.empty())
        return { nullptr, false };
    size_t step_index = output_step % execution_steps.size();
    if (output_step == (size_t)-1)
    {
        step_index = execution_steps.size() - 1;
        while (step_index > 0 && execution_steps[step_index].skipped)
            --step_index;
    }
    
    auto& step = execution_steps[step_index];
    size_t attachment = output_image;
    auto config = step.render_pass->getOutputConfig();
    size_t max_attachments = config.additional_attachments + (config.has_depth_attachment ? 2 : 1);
    bool is_stencil = false;
    if (output_image == max_attachments)
        is_stencil = true;
    return { step.render_pass->getImage(is_stencil ? output_image - 1 : output_image), is_stencil };
}

Ref<Material> RenderGraph::getMaterialForStep(size_t step)
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to read material from step " + to_string(step) + " of render graph " + PTR(this) + ", but there is no such step");
        return nullptr;
    }

    if (execution_steps[step].is_camera)
    {
        DBG_ERROR("attempt to read material from step " + to_string(step) + " of render graph " + PTR(this) + ", but it is not a post-process (material) step");
        return nullptr;
    }

    return execution_steps[step].material;
}

Ref<Material> RenderGraph::getMaterialForStep(const string& name)
{
    return getMaterialForStep(findStep(name));
}

void RenderGraph::setSkipStep(size_t step, bool skip)
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to skip step " + to_string(step) + " of render graph " + PTR(this) + ", but there is no such step");
        return;
    }
    
    execution_steps[step].skipped = skip;
    rebuildBindings();
}

void RenderGraph::setSkipStep(const string& name, bool skip)
{
    setSkipStep(findStep(name), skip);
}

bool RenderGraph::getSkipStep(const std::string& name) const
{
    return getSkipStep(findStep(name));
}

bool RenderGraph::getSkipStep(size_t step) const
{
    if (step >= execution_steps.size())
    {
        DBG_ERROR("attempt to get skipped for step " + to_string(step) + " of render graph " + PTR(this) + ", but there is no such step");
        return false;
    }
    
    return execution_steps[step].skipped;
}

RenderGraph::RenderGraph(RenderGraphBuilder config)
{
    passthrough = new Material(Engine::loadShader("res://engine/shaders/passthrough"), PipelineBuilder().cullMode(VK_CULL_MODE_NONE).depthWrite(VK_FALSE).depthTest(VK_FALSE), RenderServer::getFinalRenderPass());
    execution_steps = config.execution_steps;
    if (!config.execution_steps.empty())
        expected_extent = config.execution_steps[0].render_pass->getExtent();

    rebuildBindings();
}

RenderGraph::~RenderGraph()
{
    DBG_INFO("destroying render graph " + PTR(this));
    execution_steps.clear();
    passthrough = nullptr;
}

size_t RenderGraph::findStep(const std::string& name) const
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

        for (const auto& pair : step.texture_bindings)
        {
            RenderStep binding_step = execution_steps[pair.second.step_index];
            Ref<Texture> texture = binding_step.render_pass->getImage(pair.second.output_index);
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
            step.material->setTexture(pair.first, texture);
            step.material->setSampler(pair.first, new Sampler(SamplerBuilder().filter(pair.second.filter_mode).address(pair.second.address_mode)));
        }
    }
}

void RenderGraph::recordCameraStep(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Camera> camera, Ref<RenderPass> pass, std::multiset<DrawCommand, DrawCommand> commands, FrameStats& stats) const
{
    stats.cameras++;

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
    RenderServer::writeTimestamp(image_index, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    stats.passes++;

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
        {
            vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                Engine::isWireframeMode() ? last_used_material->getDebugPipeline() : last_used_material->getPipeline());
            stats.pipeline_rebinds++;
        }
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
        stats.draw_calls++;
        stats.triangles += last_used_mesh->getIndexCount() / 3;
        stats.vertices += last_used_mesh->getVertexCount();
    }

    RenderServer::writeTimestamp(image_index, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    vkCmdEndRenderPass(command_buffer);
}

void RenderGraph::recordPostProcessStep(VkCommandBuffer command_buffer, uint32_t image_index, Ref<Material> material, VkDescriptorSet scene_descriptor_set, FrameStats& stats) const
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
    RenderServer::writeTimestamp(image_index, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
    stats.passes++;

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
    stats.pipeline_rebinds++;

    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(), 0, 1, &scene_descriptor_set, 0, nullptr);
    VkDescriptorSet material_descriptor_set = material->getDescriptorSet(image_index);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material->getPipelineLayout(), 2, 1, &material_descriptor_set, 0, nullptr);

    WeakRef<Mesh> quad = RenderServer::getQuad();
    VkBuffer vertex_buffers[] = { quad->getVertexBuffer()};
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
    vkCmdBindIndexBuffer(command_buffer, quad->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(command_buffer, static_cast<uint32_t>(quad->getIndexCount()), 1, 0, 0, 0);
    stats.draw_calls++;
    stats.triangles += 2;
    stats.vertices += 4;

    RenderServer::writeTimestamp(image_index, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    vkCmdEndRenderPass(command_buffer);
}

RenderGraphBuilder RenderGraphBuilder::addCamera(size_t slot)
{
    return addCamera(slot, RenderServer::getMainRenderPass()->getOutputConfig());
}

RenderGraphBuilder RenderGraphBuilder::addCamera(size_t slot, RenderOutput render_pass_config, float size_factor, VkExtent2D custom_extent)
{
    RenderStep step;
    step.is_camera = true;
    step.camera_slot = slot;
    step.resolution_scale = size_factor;
    step.custom_extent = custom_extent;
    glm::vec2 size = RenderServer::getFramebufferSize();
    if (size_factor > 0.0f)
        step.render_pass = new RenderPass((uint32_t)(size.x * size_factor), (uint32_t)(size.y * size_factor), render_pass_config);
    else
        step.render_pass = new RenderPass(custom_extent.width ? custom_extent.width : (uint32_t)size.x, custom_extent.height ? custom_extent.height : (uint32_t)size.y, render_pass_config);
    execution_steps.push_back(step);
    return *this;
}

RenderGraphBuilder RenderGraphBuilder::addCamera(size_t slot, float size_factor, VkExtent2D custom_extent)
{
    return addCamera(slot, RenderServer::getMainRenderPass()->getOutputConfig(), size_factor, custom_extent);
}

RenderGraphBuilder RenderGraphBuilder::addPostProcess(Ref<Shader> shader, map<uint32_t, RenderTextureBinding> texture_bindings)
{
    return addPostProcess(shader, texture_bindings, RenderOutput{ 0, false });
}

RenderGraphBuilder RenderGraphBuilder::addPostProcess(Ref<Shader> shader, map<uint32_t, RenderTextureBinding> texture_bindings, RenderOutput render_pass_config, float size_factor, VkExtent2D custom_extent)
{
    RenderStep step;
    step.is_camera = false;
    step.resolution_scale = size_factor;
    step.custom_extent = custom_extent;
    glm::vec2 size = RenderServer::getFramebufferSize();
    if (size_factor > 0.0f)
        step.render_pass = new RenderPass((uint32_t)(size.x * size_factor), (uint32_t)(size.y * size_factor), render_pass_config);
    else
        step.render_pass = new RenderPass(custom_extent.width ? custom_extent.width : (uint32_t)size.x, custom_extent.height ? custom_extent.height : (uint32_t)size.y, render_pass_config);
    step.material = new Material(shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE).depthTest(VK_FALSE).depthWrite(VK_FALSE), step.render_pass);
    step.texture_bindings = texture_bindings;
    step.scene_uniforms = new UniformBlock(ShaderLayout{ RenderServer::getSceneDescriptorSetLayout(), {{ 0, UNIFORM, sizeof(SceneUniforms) }} });
    execution_steps.push_back(step);
    return *this;
}

RenderGraphBuilder RenderGraphBuilder::addPostProcess(Ref<Shader> shader, map<uint32_t, RenderTextureBinding> texture_bindings, float size_factor, VkExtent2D custom_extent)
{
    return addPostProcess(shader, texture_bindings, RenderOutput{ 0, false }, size_factor, custom_extent);
}

RenderStep::~RenderStep()
{
    DBG_INFO("destroying render step " + PTR(this));
}