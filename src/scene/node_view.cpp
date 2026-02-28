#include "node_view.h"

#include <glm/gtc/integer.hpp>

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

constexpr size_t v_i_buffer_rounding_size = 256;
constexpr float RENDER_MODE_BOX = 0.0f;
constexpr float RENDER_MODE_TEXT = 0.1f;
constexpr float RENDER_MODE_PINS = 0.2f;
constexpr float RENDER_MODE_BACKGROUND = 0.3f;

static glm::vec2 flipUV(glm::vec2 v)
{
    return { v.x, 1.0f - v.y };
}

Ref<NodeView> NodeView::create()
{
    Ref obj = new NodeView();
    obj->self = obj.cast<Object>();
    return obj;
}

void NodeView::setStyle(Style new_style)
{
    material->setTexture("text_atlas", new_style.font->getAtlas());
    material->setTexture("node_atlas", new_style.node_atlas);
    material->setFloatUniform("grid_size", new_style.grid_size);
    material->setVec3Uniform("outline_colour", new_style.outline_colour);
    material->setIntUniform("outline_style", new_style.outline_style);
    material->setFloatUniform("outline_modulate", new_style.outline_colour_mult);
    material->setVec3Uniform("fill_colour", new_style.fill_colour);
    material->setFloatUniform("fill_modulate", new_style.fill_colour_mult);
    material->setFloatUniform("grid_dots_modulate", new_style.grid_dots_modulate);
    style = new_style;
    updateMesh();
}

void NodeView::updateMesh()
{
    if (nodes.empty())
    {
        mesh = nullptr;
        return;
    }
    
    vertices.clear();
    indices.clear();
    
    glm::vec2 half_viewport = glm::vec2{ 1000000.0f };
    addQuad(-half_viewport, half_viewport * 2.0f, { 0, 0 }, { 1, 1 }, style.grid_colour, RENDER_MODE_BACKGROUND);
    
    // draw nodes
    for (auto& node : nodes)
    {
        int node_width_tiles = 10;
        int node_height_tiles = node->elements.size() + 2;
        
        float node_fill_mode = style.fill_modulate_colour ? 2.0f : 0.0f;
        glm::vec2 box_width = glm::vec2{ node_width_tiles, 0 } * style.grid_size;

        // heading box
        // TODO: header at bottom
        addQuad(node->position, glm::vec2{ node_width_tiles, 1 } * style.grid_size,
            { 0, 0 }, { 1, 0.5f },
            node->colour, RENDER_MODE_BOX,
            { style.outline_style == HIDDEN ? 0.0f : 1.0f, style.header_fill ? 1.0f : node_fill_mode, 0.0f },
            glm::vec2{ node_width_tiles, 1.0f * 2.0f } * style.grid_size);
        glm::vec2 header_position = node->position;
        if (style.header_align > 0) header_position = node->position + box_width;
        else if (style.header_align == 0) header_position = node->position + (box_width * 0.5f);
        addText(node->title, header_position, style.text_colour, style.header_align);
        
        // TODO: line between header and main area
        
        // main box
        addQuad(node->position + glm::vec2{ 0, style.grid_size }, glm::vec2{ node_width_tiles, node_height_tiles - 1 } * style.grid_size,
            { 0, 0.5f }, { 1, 1 },
            node->colour, RENDER_MODE_BOX,
            { style.outline_style == HIDDEN ? 0.0f : 1.0f, node_fill_mode, 0.0f },
            glm::vec2{ node_width_tiles, (node_height_tiles - 1) * 2.0f } * style.grid_size);

        // elements
        glm::vec2 position = node->position - glm::vec2{ 0, style.grid_size };
        for (const auto& elem : node->elements)
        {
            switch (elem.type)
            {
            case ELEMENT_INPUT:
                addText(elem.text, position, style.text_colour);
                break;
            case ELEMENT_OUTPUT:
                addText(elem.text, position + box_width, style.text_colour, 1);
                break;
            case ELEMENT_TEXT:
                if (style.center_text_elements)
                    addText(elem.text, position + (box_width * 0.5f), style.text_colour, 0);
                else
                    addText(elem.text, position, style.text_colour);
                break;
            }
            // TODO: pins
            position -= glm::vec2{ 0, style.grid_size };
        }

        // TODO: node minimise button
        // TODO: shadows!
        
    //     for (const NodeElement& element : node->elements)
    //     {
    //         glm::vec2 line_pos_base = box_base + glm::vec2{ 0, box_height_lines * style.grid_size };
    //         switch (element.type)
    //         {
    //         case ELEMENT_INPUT:
    //             addPin(line_pos_base, foreground_colour, element.pin_type, element.pin_solid);
    //             addText(element.text, line_pos_base + glm::vec2{ style.text_left_inset + style.grid_size, 0 }, foreground_colour);
    //             break;
    //         case ELEMENT_OUTPUT:
    //             addPin(line_pos_base + glm::vec2{ box_width - 1, 0 } * style.grid_size, foreground_colour, element.pin_type, element.pin_solid);
    //             text_width = (size_t)(style.font->getGlyphSize().x) * element.text.size();
    //             addText(element.text, line_pos_base + glm::vec2{ (box_width * style.grid_size) - (style.text_left_inset + style.grid_size + text_width), 0.0f }, foreground_colour);
    //             break;
    //         case ELEMENT_TEXT:
    //             addText(element.text, line_pos_base + glm::vec2{ style.text_left_inset + style.grid_size, 0 }, foreground_colour);
    //             break;
    //         case ELEMENT_SPACE: break;
    //         }
    //         box_height_lines++;
    //     }
    }

    size_t vertices_rounded_up = ((vertices.size() / v_i_buffer_rounding_size) + 2) * v_i_buffer_rounding_size;
    size_t indices_rounded_up = ((indices.size() / v_i_buffer_rounding_size) + 2) * v_i_buffer_rounding_size;

    if (mesh)
        mesh->updateData(vertices, indices, vertices_rounded_up, indices_rounded_up);
    else
        mesh = new Mesh(vertices, indices, true);
}

NodeView::NodeView() : StaticMesh(nullptr, nullptr)
{
    // FIXME: make these paths res-relative again!
    material = new Material(new Shader("res_engine/node_shader.glsl"), PipelineBuilder().cullMode(CULL_NONE).depthTest(false).depthWrite(false));
    Ref<Sampler> sampler = Engine::makeSampler(SamplerBuilder().filter(FILTER_NEAREST));
    material->setSampler("node_atlas", sampler);
    material->setSampler("text_atlas", sampler);

    style.node_atlas = new Texture("res_engine/textures/node_atlas.png");
    style.font = new Font("res_engine/textures/font_IBM_XGA_AI_12x23.png", glm::ivec2{ 14, 25 });

    setStyle(style);
}

void NodeView::addQuad(glm::vec2 position, glm::vec2 size, glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec3 colour, float mode, glm::vec3 extra, glm::vec2 fake_size)
{
    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    
    glm::vec4 normal_value = { size.x, size.y, mode, 0.0f };
    if (fake_size.x != 0.0f)
        normal_value.x = fake_size.x;
    if (fake_size.y != 0.0f)
        normal_value.y = fake_size.y;
    glm::vec4 tangent_value = { extra, 1 };
    glm::vec4 colour_value = glm::vec4{ colour, 1 };

    // top left
    vertices.push_back(Vertex{
        { position.x, -position.y, 0, 1 },
        colour_value, normal_value, tangent_value,
        uv_tl });
    // top right
    vertices.push_back(Vertex{
        { position.x + size.x, -position.y, 0, 1 },
        colour_value, normal_value, tangent_value,
        glm::vec2{ uv_br.x, uv_tl.y } });
    // bottom left
    vertices.push_back(Vertex{
        { position.x, -position.y - size.y, 0, 1 },
        colour_value, normal_value, tangent_value,
        glm::vec2{ uv_tl.x, uv_br.y } });
    // bottom right
    vertices.push_back(Vertex{
        { position.x + size.x, -position.y - size.y, 0, 1 },
        colour_value, normal_value, tangent_value,
        uv_br });
    
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

void NodeView::addPin(glm::vec2 position, glm::vec3 tint, int type, bool filled)
{
    //addQuad(position, glm::vec2{ style.grid_size, style.grid_size }, { 0, 0, filled ? 10 : 1, 0 }, tint, true, type);
}

void NodeView::addText(const string& text, glm::vec2 _start, glm::vec3 tint, int align)
{
    const glm::vec2 uv_size = style.font->getGlyphUVSize();
    const glm::vec2 char_size = style.font->getGlyphSize();
    
    glm::vec2 start = _start;
    float width = ((text.size() + 1) * (style.font->getGlyphSize().x + style.text_spacing - 2.0f)) - (style.text_spacing - 2.0f);
    if (align > 0)
        start.x -= width;
    else if (align == 0)
        start.x -= glm::round(width / 2.0f);

    glm::vec2 position = start + style.text_offset + glm::vec2{ 0, (style.grid_size - style.font->getGlyphSize().y) };
    for (char c : text)
    {
        glm::vec2 uv_base = style.font->getGlyphUVOffset(c);

        glm::vec2 uv_br = flipUV(uv_base + uv_size);
        glm::vec2 uv_tl = flipUV(uv_base);
        glm::vec4 pos_tl = { position.x, -position.y, 0, 1 };

        addQuad(pos_tl, char_size, uv_tl, uv_br, tint, RENDER_MODE_TEXT);
        
        position.x += style.font->getGlyphSize().x + style.text_spacing - 2.0f;
    }
}

NodeView::~NodeView()
{
    style.font = nullptr;
    style.node_atlas = nullptr;
    material = nullptr;
    nodes.clear();
}
