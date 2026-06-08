#include "buffer.h"
#include "command_buffer.h"
#include "material.h"
#include "graphics_server.h"
#include "texture.h"
#include "vulkan_helpers.h"

#include <vulkan/vulkan.hpp>

using namespace HopEngine;

UniformBlock::UniformBlock(const Shader::Layout& layout_info)
{
    layout    = layout_info;
    set_index = layout_info.set_index;
    // calculate the total size of the required buffer for the uniforms
    // we use one big buffer regardless of if there are multiple uniform blocks,
    // and just map sections of the buffer when we apply the descriptor set
    // bindings/writes
    size = 0;
    for (const auto& binding : layout_info.bindings)
    {
        if (binding.type == Shader::UNIFORM) size += binding.buffer_size;
        else if (binding.type == Shader::TEXTURE)
        {
            textures_in_use[binding.binding] = { binding.texture_is_3d
                                                     ? GraphicsServer::getDefault3DTexture().strong()
                                                     : GraphicsServer::getDefaultTexture().strong(),
                GraphicsServer::getDefaultSampler().strong() };
        }
    }

    // create uniform buffers for each frame-in-flight to avoid updating
    // a buffer currently being used by the GPU
    uniform_buffers.resize(GraphicsServer::getFramesInFlight());
    for (auto& uniform_buffer : uniform_buffers)
        uniform_buffer = new Buffer(size + 4, Buffer::BUFFER_USAGE_UNIFORM,
            MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);

    // allocate descriptor sets from the pool
    const std::vector<VkDescriptorSetLayout> set_layouts(uniform_buffers.size(),
        static_cast<VkDescriptorSetLayout>(layout_info.layout));
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info{};
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool =
        static_cast<VkDescriptorPool>(GraphicsServer::getDescriptorPool());
    descriptor_set_alloc_info.descriptorSetCount = static_cast<uint32_t>(uniform_buffers.size());
    descriptor_set_alloc_info.pSetLayouts        = set_layouts.data();
    descriptor_sets.resize(uniform_buffers.size());
    CHECK_RESULT(vkAllocateDescriptorSets,
        (static_cast<VkDevice>(GraphicsServer::getDevice()), &descriptor_set_alloc_info,
            reinterpret_cast<VkDescriptorSet*>(descriptor_sets.data())),
        FAULT,
        ;);

    // apply descriptor writes/bindings
    applyDescriptorBindings();

    // size the uniform buffer to match the on-GPU buffers
    live_uniform_buffer.resize(size);

    DBG_VERBOSE("created uniform block of buffer size " + std::to_string(size) + " with " +
                std::to_string(textures_in_use.size()) + " texture slots (" +
                std::to_string(layout_info.bindings.size()) + " total bindings)");
}

UniformBlock::~UniformBlock()
{
    DBG_VERBOSE("destroying uniform block " + PTR(this));
    for (GPUHandle& set : descriptor_sets)
    {
        auto _temp_ = set;
        GraphicsServer::queueFree(
            [_temp_]()
            {
                vkFreeDescriptorSets(static_cast<VkDevice>(GraphicsServer::getDevice()),
                    static_cast<VkDescriptorPool>(GraphicsServer::getDescriptorPool()), 1,
                    reinterpret_cast<const VkDescriptorSet*>(&_temp_));
            });
        set = nullptr;
    }
}

void UniformBlock::bind(WeakRef<DrawCommandBuffer> command_buffer)
{
    if (rebind_needed)
    {
        applyDescriptorBindings();
        rebind_needed = false;
    }
    uint32_t index = command_buffer->getImageIndex() % uniform_buffers.size();
    memcpy(uniform_buffers[index]->mapMemory(), live_uniform_buffer.data(), live_uniform_buffer.size());
    command_buffer->bindDescriptorSetInternal(set_index, descriptor_sets[index]);
}

void UniformBlock::setTexture(const uint32_t binding, Ref<Texture> texture)
{
    // if the texture is already bound, skip rebinding it
    if (std::get<0>(textures_in_use[binding]) == texture) return;
    // update the binding
    if (!texture)
        std::get<0>(textures_in_use[binding]) = layout.bindings[binding].texture_is_3d
                                                    ? GraphicsServer::getDefault3DTexture().strong()
                                                    : GraphicsServer::getDefaultTexture().strong();
    else
        std::get<0>(textures_in_use[binding]) = texture;
    rebind_needed = true;
}

void UniformBlock::setSampler(const uint32_t binding, Ref<Sampler> sampler)
{
    // if same sampler, skip rebinding
    if (std::get<1>(textures_in_use[binding]) == sampler) return;
    // update sampler binding; use default engine sampler if null
    if (!sampler) std::get<1>(textures_in_use[binding]) = GraphicsServer::getDefaultSampler().strong();
    else
        std::get<1>(textures_in_use[binding]) = sampler;
    rebind_needed = true;
}

void UniformBlock::setTextureSampler(uint32_t binding, Ref<Texture> texture, Ref<Sampler> sampler)
{
    setTexture(binding, texture);
    setSampler(binding, sampler);
}

void UniformBlock::applyDescriptorBindings()
{
    // updating the descriptor set bindings so that they correctly connect
    // to our specified textures, and our uniform buffers
    DBG_VERBOSE("uniform block " + PTR(this) + " updating " + std::to_string(layout.bindings.size()) +
                " descriptor bindings");
    for (size_t i = 0; i < descriptor_sets.size(); ++i)
    {
        VkDeviceSize offset = 0;
        for (const Shader::Descriptor& binding : layout.bindings)
        {
            // standard write command for our specified descriptor set and binding
            VkWriteDescriptorSet descriptor_write{};
            descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet          = static_cast<VkDescriptorSet>(descriptor_sets[i]);
            descriptor_write.dstBinding      = binding.binding;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorCount = 1;

            VkDescriptorBufferInfo buffer_info{};
            VkDescriptorImageInfo image_info{};
            if (binding.type == Shader::UNIFORM)
            {
                // if the binding is a uniform buffer, point it to a section
                // of the corresponding GPU buffer. offset is incremented
                // according to the buffer size of this particular uniform
                // block
                buffer_info.buffer              = static_cast<VkBuffer>(uniform_buffers[i]->getHandle());
                buffer_info.offset              = offset;
                buffer_info.range               = binding.buffer_size;
                descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_write.pBufferInfo    = &buffer_info;
                offset += binding.buffer_size;
            }
            else if (binding.type == Shader::TEXTURE)
            {
                // if the binding is a texture-sampler, give it the image view
                // and sampler specified in the texture map
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                auto& texture          = std::get<0>(textures_in_use[binding.binding]);
                if (texture->is3D() != binding.texture_is_3d)
                {
                    DBG_ERROR(
                        "uniform attempted to bind an incompatible texture dimension. texture will be reset to default.");
                    std::get<0>(textures_in_use[binding.binding]) =
                        binding.texture_is_3d ? GraphicsServer::getDefault3DTexture().strong()
                                              : GraphicsServer::getDefaultTexture().strong();
                    texture = std::get<0>(textures_in_use[binding.binding]);
                }
                image_info.imageView = static_cast<VkImageView>(texture->getView());
                image_info.sampler =
                    static_cast<VkSampler>(std::get<1>(textures_in_use[binding.binding])->getSampler());
                descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor_write.pImageInfo     = &image_info;
            }
            // this could potentially be more efficient, since we could group all the
            // write commands into a sensible array and issue one big vkUpdateDescriptorSets,
            // but it's annoying to corral all the secondary structures involved
            vkUpdateDescriptorSets(static_cast<VkDevice>(GraphicsServer::getDevice()), 1, &descriptor_write,
                0, nullptr);
        }
    }
}
