#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"
#include "command_buffer.h"
#include "scene.h"
#include "basic_components.h"
#include "mesh.h"

namespace HopEngine
{

class NodeView final : public StaticMeshComponent
{
public:
	enum NodeElementType : uint8_t
	{
		ELEMENT_INPUT,
		ELEMENT_OUTPUT,
		ELEMENT_TEXT,
		ELEMENT_SPACE
	};
	
	struct NodeElement final
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

	struct Node final
	{
        friend class NodeView;
    private:
        WeakRef<Node> self;
		std::vector<std::pair<WeakRef<Node>, size_t>> incoming_links;
    public:
		std::string title = "node";
		std::vector<NodeElement> elements;
		glm::vec2 position = { 0, 0 };
		glm::vec2 size = { 8, 1 };
		glm::vec3 colour = { 1.0f, 0.44f, 0.0f };
		bool highlighted = false;
		bool minimised = false;

        WeakRef<Node> input(const std::string& text) { elements.push_back(NodeElement(text, ELEMENT_INPUT)); return self; }
        WeakRef<Node> output(const std::string& text) { elements.push_back(NodeElement(text, ELEMENT_OUTPUT)); return self; }
        WeakRef<Node> text(const std::string& text)  { elements.push_back(NodeElement(text, ELEMENT_TEXT)); return self; }
        WeakRef<Node> space() { elements.push_back(NodeElement("", ELEMENT_SPACE)); return self; }

    private:
        DELETE_NOT_ALL_CONSTRUCTORS(Node);
        Node() = default;
	};

	enum OutlineStyle
	{
		HIDDEN,
		PRESET_COLOUR,
		NODE_COLOUR,
		MODULATE_NODE_COLOUR
	};

	struct Style final
	{
		Ref<Font> font = nullptr;
		
		Ref<Texture> node_atlas = nullptr;
		Ref<Texture> extra_atlas = nullptr;
		Ref<Texture> ui_atlas = nullptr;
		
		float grid_size = 32.0f;
		
		int header_align = -1;
		bool header_at_top = true;
		bool header_fill = true;
		int after_header_spacing = 0;

		glm::vec2 text_offset = { 6.0f, 2.0f };
		glm::vec3 text_colour = { 0.020f, 0.020f, 0.038f };
		float text_spacing = -1.0f;
		
		OutlineStyle outline_style = PRESET_COLOUR;
		glm::vec3 outline_colour = { 0.020f, 0.020f, 0.038f };
		glm::vec3 outline_colour_highlight = { 0.75f, 0.087f, 0.0f };
		float outline_colour_mult = 0.7f;
		
		bool fill_modulate_colour = false;
		glm::vec3 fill_colour = { 0.9f, 0.85f, 0.81f };
		float fill_colour_mult = 0.1f;
		
		glm::vec3 background_colour = { 0.94f, 0.94f, 0.94f };
		bool show_grid = true;
		int grid_scale = 4;
		glm::vec3 grid_colour = { 0.70f, 0.70f, 0.70f };
		float grid_dots_modulate = 0.001f;

		float pin_offset = 2.0f;
		bool reverse_element_order = false;
		bool center_text_elements = true;
		int after_elements_spacing = 0;

		bool shadows = true;
		glm::vec2 shadow_offset = { 4.0f, 4.0f };
		glm::vec3 shadow_colour = { 0.14f, 0.14f, 0.29f };

        static Ref<Style> deserialise(const std::string& path);
	};

public:

private:
	std::vector<Ref<Node>> nodes;
	std::vector<Mesh::Vertex> vertices;
	std::vector<uint16_t> indices;
	Ref<Style> style;
    glm::vec2 temp_link_start;
    glm::vec2 temp_link_end;
    bool draw_temp_link = false;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(NodeView);
	NodeView() = default;
	~NodeView() override;
	
	void awake() override;

	Ref<Style> getStyle() { return style; }
	void setStyle(Ref<Style> new_style);
	void updateMesh();
	bool checkInput(glm::ivec2 rect_min, glm::ivec2 rect_size);
    WeakRef<Node> makeNode(const std::string& title, glm::vec3 colour, int tiles_wide);
    void makeLink(WeakRef<Node> sender_node, size_t output_index, WeakRef<Node> receiver_node, size_t input_index);
    void drawImGuiDebug();

private:
    void addQuadRaw(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2 norm_xy, glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec3 colour, float mode, glm::vec3 extra);
	void addQuad(glm::vec2 position, glm::vec2 size, glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec3 colour, float mode, glm::vec3 extra = { 0.0f, 0.0f, 15.0f });
	void addPin(glm::vec2 position, glm::vec3 tint, int type, bool filled);
	void addText(const std::string& text, glm::vec2 _start, glm::vec3 tint, int align = -1);
	void addLink(glm::vec2 link_start, glm::vec2 link_end, bool translucent = false);
    void drawNode(WeakRef<Node> node, int pass);
};

}
