#pragma once

#include <array>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

#include "common.h"
#include "math_helpers.h"

namespace HopEngine
{

struct Vertex
{
	glm::vec4 position;
	glm::vec4 colour;
	glm::vec4 normal;
	glm::vec4 tangent;
	glm::vec2 uv;
};

class Mesh : public Destructible
{
private:
	Ref<Buffer> vertex_buffer;
	Ref<Buffer> index_buffer;
	size_t vertex_space = 0;
	size_t index_space = 0;
	size_t vertex_count = 0;
	size_t index_count = 0;
	bool accessible = false;
	std::string origin;
	BoundingBox bounding_box;

public:
	DELETE_CONSTRUCTORS(Mesh);

	VkBuffer getVertexBuffer() const;
	VkBuffer getIndexBuffer() const;
	inline size_t getVertexCount() const { return vertex_count; }
	inline size_t getIndexCount() const { return index_count; }
	void updateData(std::vector<Vertex> vertices, std::vector<uint16_t> indices, size_t vertex_alloc = 0, size_t index_alloc = 0);
	inline std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	inline BoundingBox getBoundingBox() const { return bounding_box; }
	
	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();
	
	Mesh(std::string path);
	Mesh(std::vector<Vertex> vertices, std::vector<uint16_t> indices, bool keep_accessible = false);
	~Mesh() override;

private:
	bool readFileToArrays(std::string path, std::vector<Vertex>& verts, std::vector<uint16_t>& inds);
	void createFromArrays(std::vector<Vertex> verts, std::vector<uint16_t> inds);
	void recomputeBoundingBox(std::vector<Vertex> verts);
};

}
