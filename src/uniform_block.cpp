#include "uniform_block.h"

#include <stdexcept>

#include "graphics_environment.h"
#include "buffer.h"

using namespace HopEngine;
using namespace std;

UniformBlock::UniformBlock(VkDescriptorSetLayout layout, VkDeviceSize buffer_size)
{
    size = buffer_size;

    uniform_buffers.resize(GraphicsEnvironment::get()->getFramesInFlight());
    for (size_t i = 0; i < uniform_buffers.size(); ++i)
        uniform_buffers[i] = new Buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vector<VkDescriptorSetLayout> set_layouts(uniform_buffers.size(), layout);
    VkDescriptorSetAllocateInfo descriptor_set_alloc_info{ };
    descriptor_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_alloc_info.descriptorPool = GraphicsEnvironment::get()->getDescriptorPool();
    descriptor_set_alloc_info.descriptorSetCount = static_cast<uint32_t>(uniform_buffers.size());
    descriptor_set_alloc_info.pSetLayouts = set_layouts.data();
    descriptor_sets.resize(uniform_buffers.size());
    if (vkAllocateDescriptorSets(GraphicsEnvironment::get()->getDevice(), &descriptor_set_alloc_info, descriptor_sets.data()) != VK_SUCCESS)
        throw runtime_error("vkAllocateDescriptorSets failed");

    vector<VkDescriptorBufferInfo> buffer_infos(descriptor_sets.size());
    vector<VkWriteDescriptorSet> descriptor_writes(descriptor_sets.size());
    for (size_t i = 0; i < descriptor_sets.size(); ++i)
    {
        // TODO: improve this to allow more complex descriptor set management (i.e. with more than one descriptor, textures, etc)
        buffer_infos[i].buffer = uniform_buffers[i]->getBuffer();
        buffer_infos[i].offset = 0;
        buffer_infos[i].range = buffer_size;

        descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[i].dstSet = descriptor_sets[i];
        descriptor_writes[i].dstBinding = 0;
        descriptor_writes[i].dstArrayElement = 0;
        descriptor_writes[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_writes[i].descriptorCount = 1;
        descriptor_writes[i].pBufferInfo = &(buffer_infos[i]);
    }
    vkUpdateDescriptorSets(GraphicsEnvironment::get()->getDevice(), static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);

    live_uniform_buffer.resize(size);
}

UniformBlock::~UniformBlock()
{
    if (vkFreeDescriptorSets(GraphicsEnvironment::get()->getDevice(), GraphicsEnvironment::get()->getDescriptorPool(), static_cast<uint32_t>(descriptor_sets.size()), descriptor_sets.data()) != VK_SUCCESS)
        throw runtime_error("vkFreeDescriptorSets failed");
    descriptor_sets.clear();
    uniform_buffers.clear();
    live_uniform_buffer.clear();
}

void UniformBlock::pushToDescriptorSet(size_t index)
{
    memcpy(uniform_buffers[index]->mapMemory(), live_uniform_buffer.data(), live_uniform_buffer.size());
}
