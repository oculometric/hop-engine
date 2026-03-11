#include "mesh.h"

#include <vulkan/vulkan.hpp>

using namespace HopEngine;
using namespace std;

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
    attributes[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[0].offset = offsetof(Vertex, position);

    attributes[1].binding = 0;
    attributes[1].location = 1;
    attributes[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[1].offset = offsetof(Vertex, colour);

    attributes[2].binding = 0;
    attributes[2].location = 2;
    attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[2].offset = offsetof(Vertex, normal);

    attributes[3].binding = 0;
    attributes[3].location = 3;
    attributes[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes[3].offset = offsetof(Vertex, tangent);

    attributes[4].binding = 0;
    attributes[4].location = 4;
    attributes[4].format = VK_FORMAT_R32G32_SFLOAT;
    attributes[4].offset = offsetof(Vertex, uv);

    return attributes;
}