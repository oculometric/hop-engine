#include "uniform_block.h"

#include <stdexcept>

#include "graphics_environment.h"
#include "buffer.h"
#include "texture.h"
#include "sampler.h"

using namespace HopEngine;
using namespace std;

UniformBlock::UniformBlock(ShaderLayout layout_info)
{
    layout = layout_info;
    size = 0;
    for (const auto& binding : layout_info.bindings)
    {
        if (binding.type == UNIFORM)
            size += binding.buffer_size;
        else if (binding.type == TEXTURE)
            textures_in_use[binding.binding] = RenderServer::getDefaultTextureSampler();
    }

    uniform_buffers.resize(RenderServer::getFramesInFlight());
    for (size_t i = 0; i < uniform_buffers.size(); ++i)
        uniform_buffers[i] = new Buffer(size + 4, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vector<VkDescriptorSetLayout> set_layouts(uniform_buffers.size(), layout_info.layout);
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info{ };
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool = RenderServer::getDescriptorPool();
    descriptor_set_alloc_info.descriptorSetCount = static_cast<uint32_t>(uniform_buffers.size());
    descriptor_set_alloc_info.pSetLayouts = set_layouts.data();
    descriptor_sets.resize(uniform_buffers.size());
    if (vkAllocateDescriptorSets(RenderServer::getDevice(), &descriptor_set_alloc_info, descriptor_sets.data()) != VK_SUCCESS)
        DBG_FAULT("vkAllocateDescriptorSets failed");

    applyDescriptorBindings();

    live_uniform_buffer.resize(size);

    DBG_VERBOSE("created uniform block of buffer size " + to_string(size) + " with " + to_string(textures_in_use.size()) + " texture slots (" + to_string(layout_info.bindings.size()) + " total bindings)");
}

UniformBlock::~UniformBlock()
{
    DBG_VERBOSE("destroying uniform block " + PTR(this));
    RenderServer::waitIdle();
    vkFreeDescriptorSets(RenderServer::getDevice(), RenderServer::getDescriptorPool(), static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data());
    textures_in_use.clear();
    descriptor_sets.clear();
    uniform_buffers.clear();
    live_uniform_buffer.clear();
}

void UniformBlock::setTexture(uint32_t binding, Ref<Texture> image)
{
    textures_in_use[binding].first = image;
    applyDescriptorBindings();
}

void UniformBlock::setSampler(uint32_t binding, Ref<Sampler> sampler)
{
    textures_in_use[binding].second = sampler;
    applyDescriptorBindings();
}

void UniformBlock::pushToDescriptorSet(size_t index)
{
    memcpy(uniform_buffers[index]->mapMemory(), live_uniform_buffer.data(), live_uniform_buffer.size());
}

void UniformBlock::applyDescriptorBindings()
{
    DBG_VERBOSE("uniform block " + PTR(this) + " updating " + to_string(layout.bindings.size()) + " descriptor bindings");
    for (size_t i = 0; i < descriptor_sets.size(); ++i)
    {
        VkDeviceSize offset = 0;
        for (const DescriptorBinding& binding : layout.bindings)
        {
            VkWriteDescriptorSet descriptor_write{ };
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = descriptor_sets[i];
            descriptor_write.dstBinding = binding.binding;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorCount = 1;

            VkDescriptorBufferInfo buffer_info{ };
            VkDescriptorImageInfo image_info{ };
            if (binding.type == UNIFORM)
            {
                buffer_info.buffer = uniform_buffers[i]->getBuffer();
                buffer_info.offset = offset;
                buffer_info.range = binding.buffer_size;
                descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptor_write.pBufferInfo = &buffer_info;
                offset += binding.buffer_size;
            }
            else if (binding.type == TEXTURE)
            {
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = textures_in_use[binding.binding].first->getView();
                image_info.sampler = textures_in_use[binding.binding].second->getSampler();
                descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptor_write.pImageInfo = &image_info;
            }
            vkUpdateDescriptorSets(RenderServer::getDevice(), 1, &descriptor_write, 0, nullptr);
        }
    }
}
