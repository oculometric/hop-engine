#include "uniform_block.h"

#include <vulkan/vulkan.hpp>

#include "render_server.h"
#include "buffer.h"
#include "command_buffer.h"
#include "texture.h"

using namespace HopEngine;
using namespace std;

UniformBlock::UniformBlock(const Shader::Layout& layout_info)
{
    layout = layout_info;
    set_index = layout_info.set_index;
    // calculate the total size of the required buffer for the uniforms
    // we use one big buffer regardless of if there are multiple uniform blocks,
    // and just map sections of the buffer when we apply the descriptor set
    // bindings/writes
    size = 0;
    for (const auto& binding : layout_info.bindings)
    {
        if (binding.type == Shader::UNIFORM)
            size += binding.buffer_size;
        else if (binding.type == Shader::TEXTURE)
        {
            textures_in_use[binding.binding] =
            {
                binding.texture_is_3d ? RenderServer::getDefault3DTexture().strong() : RenderServer::getDefaultTexture().strong(),
                RenderServer::getDefaultSampler().strong()
            };
        }
    }

    // create uniform buffers for each frame-in-flight to avoid updating
    // a buffer currently being used by the GPU
    uniform_buffers.resize(1);
    for (auto& uniform_buffer : uniform_buffers)
        uniform_buffer = new Buffer(size + 4, Buffer::BUFFER_USAGE_UNIFORM, MEMORY_PROPERTY_HOST_VISIBLE | MEMORY_PROPERTY_HOST_COHERENT);

    // allocate descriptor sets from the pool
    const vector<VkDescriptorSetLayout> set_layouts(uniform_buffers.size(), layout_info.layout);
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info{ };
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool = RenderServer::getDescriptorPool();
    descriptor_set_alloc_info.descriptorSetCount = static_cast<uint32_t>(uniform_buffers.size());
    descriptor_set_alloc_info.pSetLayouts = set_layouts.data();
    descriptor_sets.resize(uniform_buffers.size());
    if (vkAllocateDescriptorSets(RenderServer::getDevice(), &descriptor_set_alloc_info, descriptor_sets.data()) != VK_SUCCESS)
        DBG_FAULT("vkAllocateDescriptorSets failed");

    // apply descriptor writes/bindings
    applyDescriptorBindings();

    // size the uniform buffer to match the on-GPU buffers
    live_uniform_buffer.resize(size);

    DBG_VERBOSE("created uniform block of buffer size " + ::to_string(size) + " with " + ::to_string(textures_in_use.size()) + " texture slots (" + ::to_string(layout_info.bindings.size()) + " total bindings)");
}

UniformBlock::~UniformBlock()
{
    DBG_VERBOSE("destroying uniform block " + PTR(this));
    for (VkDescriptorSet set : descriptor_sets)
        RenderServer::free(set);
}

void UniformBlock::bind(WeakRef<DrawCommandBuffer> command_buffer) const
{
    memcpy(uniform_buffers[0]->mapMemory(), live_uniform_buffer.data(), live_uniform_buffer.size());
    command_buffer->bindDescriptorSetInternal(set_index, descriptor_sets[0]);
}

void UniformBlock::setTexture(const uint32_t binding, Ref<Texture> image)
{
    // if the texture is already bound, skip rebinding it
    if (textures_in_use[binding].texture == image)
        return;
    // update the binding
    if (!image)
        textures_in_use[binding].texture = layout.bindings[binding].texture_is_3d ? RenderServer::getDefault3DTexture().strong() : RenderServer::getDefaultTexture().strong();
    else
        textures_in_use[binding].texture = image;
    applyDescriptorBindings();
}

void UniformBlock::setSampler(const uint32_t binding, Ref<Sampler> sampler)
{
    // if same sampler, skip rebinding
    if (textures_in_use[binding].sampler == sampler)
        return;
    // update sampler binding; use default engine sampler if null
    if (!sampler)
        textures_in_use[binding].sampler = RenderServer::getDefaultSampler().strong();
    else
        textures_in_use[binding].sampler = sampler;
    applyDescriptorBindings();
}

void UniformBlock::applyDescriptorBindings()
{
    // updating the descriptor set bindings so that they correctly connect
    // to our specified textures, and our uniform buffers
    DBG_BABBLE("uniform block " + PTR(this) + " updating " + ::to_string(layout.bindings.size()) + " descriptor bindings");
    for (size_t i = 0; i < descriptor_sets.size(); ++i)
    {
        VkDeviceSize offset = 0;
        for (const Shader::DescriptorBinding& binding : layout.bindings)
        {
            // standard write command for our specified descriptor set and binding
            VkWriteDescriptorSet descriptor_write{ };
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = descriptor_sets[i];
            descriptor_write.dstBinding = binding.binding;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorCount = 1;

            VkDescriptorBufferInfo buffer_info{ };
            VkDescriptorImageInfo image_info{ };
            if (binding.type == Shader::UNIFORM)
            {
                // if the binding is a uniform buffer, point it to a section
                // of the corresponding GPU buffer. offset is incremented
                // according to the buffer size of this particular uniform
                // block
                buffer_info.buffer = uniform_buffers[i]->getHandle();
                buffer_info.offset = offset;
                buffer_info.range = binding.buffer_size;
                descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_write.pBufferInfo = &buffer_info;
                offset += binding.buffer_size;
            }
            else if (binding.type == Shader::TEXTURE)
            {
                // if the binding is a texture-sampler, give it the image view
                // and sampler specified in the texture map
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                auto& texture = textures_in_use[binding.binding].texture;
                if (texture->is3D() != binding.texture_is_3d)
                {
                    DBG_ERROR("uniform attempted to bind an incompatible texture dimension. texture will be reset to default.");
                    textures_in_use[binding.binding].texture = binding.texture_is_3d ? RenderServer::getDefault3DTexture().strong() : RenderServer::getDefaultTexture().strong();
                    texture = textures_in_use[binding.binding].texture;
                }
                image_info.imageView = texture->getView();
                image_info.sampler = textures_in_use[binding.binding].sampler->getSampler();
                descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor_write.pImageInfo = &image_info;
            }
            // this could potentially be more efficient, since we could group all the
            // write commands into a sensible array and issue one big vkUpdateDescriptorSets,
            // but it's annoying to corral all the secondary structures involved
            vkUpdateDescriptorSets(RenderServer::getDevice(), 1, &descriptor_write, 0, nullptr);
        }
    }
}
