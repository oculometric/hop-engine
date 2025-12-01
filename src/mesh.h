#pragma once

#include <array>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "common.h"

namespace HopEngine
{

struct Vertex
{
	glm::vec3 position;
	glm::vec3 colour;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 uv;
};

class Buffer;

class Mesh
{
private:
	Buffer* vertex_buffer = nullptr;
	Buffer* index_buffer = nullptr;
	size_t index_count = 0;

public:
	DELETE_CONSTRUCTORS(Mesh);

	Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices);
	~Mesh();

	VkBuffer getVertexBuffer();
	VkBuffer getIndexBuffer();
	inline size_t getIndexCount() { return index_count; }

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();
};

}
