#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "object.h"
#include "mesh.h"

namespace HopEngine
{

class Font;

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
		Ref<Node> start_node = 0;
		int start_output = 0;
		Ref<Node> end_node = nullptr;
		int end_input = 0;
		int palette_index = 1;
	};

	struct Style
	{
		std::vector<glm::vec3> palette;
		bool use_dynamic_background = false;
		float background_factor = 0.02f;
		//size_t scale_factor = 2; // TODO:
		Ref<Font> font = nullptr;
		// TODO: other style textures
	};

public:
	// TODO: eliminate reference counting and make this internally managed/allocated?
	std::vector<Ref<Node>> nodes;
	std::vector<Link> links;

private:
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	Style style;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(NodeView);

	NodeView();
	inline ~NodeView() { };

	inline Style getStyle() { return style; }
	void setStyle(Style new_style);
	void updateMesh();
	Ref<Node> select(glm::vec2 world_position);

private:
	void addQuad(glm::vec2 position, glm::vec2 size, glm::vec4 colour, glm::vec3 tint, bool clip_uv, int uv_index);
	void addFrame(glm::vec2 position, glm::vec2 size, glm::vec3 tint);
	void addBadge(glm::vec2 position, glm::vec2 size, glm::vec3 tint);
	void addBlock(glm::vec2 position, glm::vec2 size, glm::vec3 tint, bool outline);
	void addPin(glm::vec2 position, glm::vec3 tint, int type, bool filled);
	void addCharacter(char c, glm::vec2 position, glm::vec3 tint);
	void addText(std::string text, glm::vec2 start, glm::vec3 tint);
	void addLinkElem(glm::vec2 position, glm::vec3 tint, int type);
	void addLink(glm::ivec2 grid_start, glm::ivec2 grid_end, glm::vec3 tint);
	glm::vec3 getBackgroundColour(glm::vec3 fg_col);
};

}
