#pragma once

#include <array>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include "common.h"

namespace HopEngine
{

struct Vertex
{
	glm::vec3 position;
	glm::vec4 colour;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 uv;
};

class Buffer;

class Mesh
{
private:
	Ref<Buffer> vertex_buffer = nullptr;
	Ref<Buffer> index_buffer = nullptr;
	size_t vertex_space = 0;
	size_t index_space = 0;
	size_t index_count = 0;
	bool accessible = false;

public:
	DELETE_CONSTRUCTORS(Mesh);

	Mesh(std::string path);
	Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices, bool keep_accessible = false);
	~Mesh();

	VkBuffer getVertexBuffer();
	VkBuffer getIndexBuffer();
	inline size_t getIndexCount() { return index_count; }
	void updateData(std::vector<Vertex> vertices, std::vector<uint16_t> indices, size_t vertex_alloc = 0, size_t index_alloc = 0);

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();

private:
	bool readFileToArrays(std::string path, std::vector<Vertex>& verts, std::vector<uint16_t>& inds);
	void createFromArrays(std::vector<Vertex> verts, std::vector<uint16_t> inds);
};

}
