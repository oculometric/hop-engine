#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "object.h"
#include "mesh.h"

namespace HopEngine
{

class NodeView : public Object
{
public:
	struct Node
	{
		std::string title;
		std::string description;
		glm::ivec2 position;
		glm::ivec2 size;
	};

public:
	std::vector<Node> boxes;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(NodeView);

	NodeView();
	inline ~NodeView() { };

	void updateMesh();

private:
	void addFrame(glm::vec2 position, glm::vec2 size, int layer, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices);
	void addCharacter(char c, glm::vec2 position, glm::vec2 size, int layer, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices);
};

}
