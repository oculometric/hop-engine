#pragma once

#include <array>
#include <string>
#include <vector>
#include <glm/glm.hpp>

#include "common.h"
#include "math_helpers.h"
#include "vulkan_typedefs.h"
#include "package.h"

namespace HopEngine
{

class Mesh final : public Destructible
{
public:
	struct Vertex final
	{
		glm::vec4 position;
		glm::vec4 colour;
		glm::vec4 normal;
		glm::vec4 tangent;
		glm::vec2 uv;
	};
	
private:
	std::string origin;
	Ref<Buffer> vertex_buffer;
	Ref<Buffer> index_buffer;
	size_t vertex_capacity = 0;
	size_t vertex_count = 0;
	size_t index_capacity = 0;
	size_t index_count = 0;
	bool accessible = false;
	BoundingBox bounding_box;

public:
	DELETE_CONSTRUCTORS(Mesh);
	Mesh(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, bool keep_accessible = false);
	~Mesh() override;
	
	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions();
	
	static Ref<Mesh> loadMesh(const std::string& path);
	static DataBlock convertToBinaryMesh(const std::string& obj_path);
	
	std::string getOrigin() const { if (this == nullptr) return "0x0"; return origin.empty() ? PTR(this) : origin; }
	size_t getVertexCount() const { return vertex_count; }
	size_t getIndexCount() const { return index_count; }
	BoundingBox getBoundingBox() const { return bounding_box; }
	void updateData(const std::vector<Vertex>& vertices, const std::vector<uint16_t>& indices, size_t vertex_alloc = 0, size_t index_alloc = 0);
	void draw(WeakRef<DrawCommandBuffer> command_buffer);
	
private:
	static DataBlock encodeBinaryMesh(std::vector<Vertex>& verts, std::vector<uint16_t>& inds);
	static bool decodeBinaryMesh(const DataBlock& data, std::vector<Vertex>& verts, std::vector<uint16_t>& inds);
	static bool readOBJ(const DataBlock& data, std::vector<Vertex>& verts, std::vector<uint16_t>& inds);
	
	void uploadFromArrays(const std::vector<Vertex>& verts, const std::vector<uint16_t>& inds);
	void recomputeBoundingBox(const std::vector<Vertex>& verts);
};

}
