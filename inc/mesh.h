#pragma once

#include <array>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "common.h"
#include "math_helpers.h"
#include "vulkan_typedefs.h"

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
	std::string origin;
	Ref<Buffer> vertex_buffer;
	Ref<Buffer> index_buffer;
	size_t vertex_space = 0;
	size_t index_space = 0;
	size_t vertex_count = 0;
	size_t index_count = 0;
	bool accessible = false;
	BoundingBox bounding_box;
	bool is_renderable = false;

public:
	DELETE_CONSTRUCTORS(Mesh);
	Mesh(const std::string& path);
	Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, bool keep_accessible = false);
	~Mesh() override;
	
	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();
	static std::vector<uint8_t> encodeBinaryMesh(const std::string& path);
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	size_t getVertexCount() const { return vertex_count; }
	size_t getIndexCount() const { return index_count; }
	BoundingBox getBoundingBox() const { return bounding_box; }
	bool isRenderable() const { return is_renderable; }
	void updateData(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, size_t vertex_alloc = 0, size_t index_alloc = 0);
	void draw(WeakRef<DrawCommandBuffer> command_buffer);
	
private:
	static bool decodeBinaryMesh(const std::vector<uint8_t>& data, std::vector<Vertex>& verts, std::vector<uint16_t>& inds);
	static bool readFileToArrays(const std::string& path, std::vector<Vertex>& verts, std::vector<uint16_t>& inds);
	void createFromArrays(const std::vector<Vertex>& verts, const std::vector<uint16_t>& inds);
	void recomputeBoundingBox(const std::vector<Vertex>& verts);
};

}
