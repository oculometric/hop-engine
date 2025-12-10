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
		int pin_type = 0;
		bool pin_solid = true;
	};

	struct Node
	{
		friend class NodeView;
	public:
		std::string title;
		std::vector<NodeElement> elements;
		glm::vec2 position;
		int palette_index = 1;
		bool highlighted = false;

		glm::vec2 last_size;
	};

	struct Link
	{
		int start_node = 0;
		int start_output = 0;
		int end_node = 0;
		int end_input = 0;
		int palette_index = 1;
	};

public:
	std::vector<Node> nodes;
	std::vector<Link> links;
	std::vector<glm::vec3> palette;
	bool use_compact = true;

private:
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	glm::vec3 background_colour;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(NodeView);

	NodeView();
	inline ~NodeView() { };

	void updateMesh();

private:
	void addQuad(glm::vec2 position, glm::vec2 size, float layer, glm::vec4 colour, glm::vec3 tint, bool clip_uv, int uv_index, std::vector<Vertex>& vertices, std::vector<uint16_t>& indices);
	void addFrame(glm::vec2 position, glm::vec2 size, float layer, glm::vec3 tint);
	void addBadge(glm::vec2 position, glm::vec2 size, float layer, glm::vec3 tint);
	void addBlock(glm::vec2 position, glm::vec2 size, float layer, glm::vec3 tint, bool outline);
	void addPin(glm::vec2 position, float layer, glm::vec3 tint, int type, bool filled);
	void addCharacter(char c, glm::vec2 position, float layer, glm::vec3 tint);
	void addText(std::string text, glm::vec2 start, float layer, glm::vec3 tint);
	void addLinkElem(glm::vec2 position, float layer, glm::vec3 tint, int type);
	void addLink(glm::ivec2 grid_start, glm::ivec2 grid_end, float layer, glm::vec3 tint);
};

}
