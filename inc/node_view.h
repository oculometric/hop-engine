#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "command_buffer.h"
#include "command_buffer.h"
#include "common.h"
#include "object.h"
#include "mesh.h"

namespace HopEngine
{

class NodeView : public StaticMesh
{
public:
	enum NodeElementType : uint8_t
	{
		ELEMENT_INPUT,
		ELEMENT_OUTPUT,
		ELEMENT_TEXT,
		ELEMENT_SPACE
	};
	
	struct NodeElement
	{
		std::string text;
		NodeElementType type;
		uint8_t pin_type = 0;
		bool pin_solid = true;
		
		NodeElement(const std::string& _text, const NodeElementType _type, const int _pin_type = 0, const bool _pin_solid = true)
			: text(_text), type(_type), pin_type(_pin_type), pin_solid(_pin_solid) { }
		NodeElement()
			: text("text"), type(ELEMENT_INPUT), pin_type(0), pin_solid(true) { }
	};

	struct Node
	{
		std::string title = "node";
		std::vector<NodeElement> elements;
		glm::vec2 position;
		glm::vec3 colour = { 1.0f, 0.44f, 0.0f };
		bool highlighted = false;
	};

	struct Style
	{
		Ref<Font> font = nullptr;
		
		Ref<Texture> node_atlas = nullptr;
		
		float grid_size = 24.0f;
		
		int header_align = -1;
		bool header_at_top = true;
		bool header_fill = true;
		bool header_outline = false;
		
		glm::vec2 text_offset = { 6.0f, 0.0f };
		glm::vec3 text_colour = { 1.0f, 1.0f, 1.0f };
		float text_spacing = 1.0f;
		
		bool outline_show = true;
		bool outline_modulate_colour = false;
		glm::vec3 outline_colour = { 0.05f, 0.05f, 0.05f };
		float outline_colour_mult = 0.7f;
		
		bool fill_modulate_colour = false;
		glm::vec3 fill_colour = { 0.9f, 0.85f, 0.81f };
		float fill_colour_mult = 0.1f;
		
		glm::vec3 grid_colour = { 0.01f, 0.01f, 0.01f };
	};

public:
	std::vector<Ref<Node>> nodes;

private:
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	Style style;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(NodeView);
	static Ref<NodeView> create();
	~NodeView() override;
	
	Style getStyle() { return style; }
	void setStyle(Style new_style);
	void updateMesh();
	void checkInput();
	
protected:
	NodeView();

private:
	void addQuad(glm::vec2 position, glm::vec2 size, glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec3 colour, float mode);
	void addFrame(glm::vec2 position, glm::vec2 size, glm::vec3 tint, bool filled);
	void addPin(glm::vec2 position, glm::vec3 tint, int type, bool filled);
	void addText(const std::string& text, glm::vec2 start, glm::vec3 tint);
};

}
