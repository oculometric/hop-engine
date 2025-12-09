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
	enum NodeElementType
	{
		ELEMENT_INPUT,
		ELEMENT_OUTPUT,
		ELEMENT_TEXT,
		ELEMENT_SPACE,
		ELEMENT_BLOCK
	};
	
	struct NodeElement
	{
		std::string text;
		NodeElementType type;
	};

	struct Node
	{
		std::string title;
		std::vector<NodeElement> elements;
		glm::vec2 position;
		int palette_index = 1;
		bool highlighted = false;
	};

public:
	std::vector<Node> nodes;
	std::vector<glm::vec3> palette;

private:
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(NodeView);

	NodeView();
	inline ~NodeView() { };

	void updateMesh();

private:
	void addQuad(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 colour, glm::vec3 tint, bool clip_uv, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices);
	void addFrame(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 tint);
	void addBadge(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 tint);
	void addBlock(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 tint);
	void addPin(glm::vec2 position, int layer, glm::vec3 tint);
	void addCharacter(char c, glm::vec2 position, int layer, glm::vec3 tint);
	void addText(std::string text, glm::vec2 start, int layer, glm::vec3 tint);
};

}
