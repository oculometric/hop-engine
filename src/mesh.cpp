#include "mesh.h"

#include <stdexcept>

#include "graphics_environment.h"
#include "buffer.h"

using namespace HopEngine;
using namespace std;

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices)
{
    vertex_buffer = new Buffer(sizeof(Vertex) * vertices.size(),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(vertex_buffer->mapMemory(), vertices.data(), vertex_buffer->getSize());
    vertex_buffer->unmapMemory();

    index_buffer = new Buffer(sizeof(uint16_t) * indices.size(),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    memcpy(index_buffer->mapMemory(), indices.data(), index_buffer->getSize());
    index_buffer->unmapMemory();

    index_count = indices.size();
}

Mesh::~Mesh()
{
    delete vertex_buffer;
    delete index_buffer;
}

VkBuffer Mesh::getVertexBuffer()
{
    return vertex_buffer->getBuffer();
}

VkBuffer HopEngine::Mesh::getIndexBuffer()
{
    return index_buffer->getBuffer();
}

VkVertexInputBindingDescription Mesh::getBindingDescription()
{
    VkVertexInputBindingDescription binding_description{ };
    binding_description.binding = 0;
    binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    binding_description.stride = sizeof(Vertex);

    return binding_description;
}

array<VkVertexInputAttributeDescription, 5> Mesh::getAttributeDescriptions()
{
    array<VkVertexInputAttributeDescription, 5> attributes;
    attributes[0].binding = 0;
    attributes[0].location = 0;
    attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[0].offset = offsetof(Vertex, position);

    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[1].offset = offsetof(Vertex, colour);

    attributes[2].binding = 0;
    attributes[2].location = 2;
    attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[2].offset = offsetof(Vertex, normal);

    attributes[3].binding = 0;
    attributes[3].location = 3;
    attributes[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributes[3].offset = offsetof(Vertex, tangent);

    attributes[4].binding = 0;
    attributes[4].location = 4;
    attributes[4].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[4].offset = offsetof(Vertex, uv);

    return attributes;
}
