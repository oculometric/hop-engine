#include "node_view.h"

#include <glm/gtc/integer.hpp>

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

constexpr size_t v_i_buffer_rounding_size = 256;

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
    style = new_style;
    updateMesh();
}

// TODO: new font
void NodeView::updateMesh()
{
    if (nodes.empty())
    {
        mesh = nullptr;
        return;
    }
    
    vertices.clear();
    indices.clear();

    // prepass to calculate node sizes
    // for (Ref<Node> node : nodes)
    // {
    //     size_t box_width = 3;
    //     size_t text_width = 0;
    //     text_width = (size_t)(style.font->getGlyphSize().x) * node->title.size();
    //     box_width = glm::max(box_width, ((size_t)(text_width / style.grid_size) + 4));
    //     for (const NodeElement& element : node->elements)
    //     {
    //         switch (element.type)
    //         {
    //         case ELEMENT_TEXT:
    //         case ELEMENT_BLOCK:
    //         case ELEMENT_OUTPUT:
    //         case ELEMENT_INPUT:
    //             text_width = (size_t)(style.font->getGlyphSize().x) * element.text.size();
    //             box_width = glm::max(box_width, ((size_t)(text_width / style.grid_size) + 4));
    //             break;
    //         case ELEMENT_SPACE: break;
    //         }
    //     }
    //
    //     size_t box_height_lines = node->elements.size() + 2;
    //     node->last_size = glm::vec2{ box_width, box_height_lines } * style.grid_size;
    // }

    // draw links
    // for (Link& link : links)
    // {
    //     Ref<Node> start = link.start_node;
    //     Ref<Node> end = link.end_node;
    //
    //     glm::ivec2 start_pos = glm::round(start->position);
    //     start_pos.x += (int)(start->last_size.x / style.grid_size) - 1;
    //     int start_offset = 0;
    //     for (int output_num = 0; start_offset < (int)start->elements.size(); ++start_offset)
    //     {
    //         if (start->elements[start_offset].type == ELEMENT_OUTPUT)
    //         {
    //             if (output_num == link.start_output)
    //                 break;
    //             ++output_num;
    //         }
    //     }
    //     start_pos.y += start_offset + 1;
    //
    //     glm::ivec2 end_pos = glm::round(end->position);
    //     int end_offset = 0;
    //     for (int input_num = 0; end_offset < (int)end->elements.size(); ++end_offset)
    //     {
    //         if (end->elements[end_offset].type == ELEMENT_INPUT)
    //         {
    //             if (input_num == link.end_input)
    //                 break;
    //             ++input_num;
    //         }
    //     }
    //     end_pos.y += end_offset + 1;
    //
    //     
    //     addLink(start_pos, end_pos, getForegroundColour(link.palette_index));
    // }
    //
    
    glm::vec2 half_viewport = glm::vec2{1000000.0f};//glm::vec2(getScene()->getViewportSize()) / 2.0f;
    glm::vec4 grid_colour = glm::vec4{ style.grid_colour, 0 };
    glm::vec4 normal_value = glm::vec4{ 0, 0, 0.2f, 0 };
    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    
    // top left
    vertices.push_back(Vertex{ { -half_viewport.x, half_viewport.y, 0, 1 }, grid_colour, normal_value, { }, glm::vec2{ 0, 0 } });
    // top right
    vertices.push_back(Vertex{ { half_viewport.x, half_viewport.y, 0, 1 }, grid_colour, normal_value, { }, glm::vec2{ 1, 0 } });
    // bottom left
    vertices.push_back(Vertex{ { -half_viewport.x, -half_viewport.y, 0, 1 }, grid_colour, normal_value, { }, glm::vec2{ 0, 1 } });
    // bottom right
    vertices.push_back(Vertex{ { half_viewport.x, -half_viewport.y, 0, 1 }, grid_colour, normal_value, { }, glm::vec2{ 1, 1 } });
    
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
    
    // draw nodes
    for (auto& node : nodes)
    {
        int node_width_tiles = 10;
        int node_height_tiles = 16;
        // TODO: ability to have just the fill (no frame)
        // TODO: header text align
        addFrame(node->position, glm::vec2{ node_width_tiles, node_height_tiles } * style.grid_size, node->colour, false);
        
        glm::vec2 header_box_pos = node->position - glm::vec2{ 0, style.header_offset * style.grid_size };
        glm::vec2 header_box_size = glm::vec2{ node_width_tiles, style.header_height } * style.grid_size;
        if (!style.header_at_top)
            header_box_pos = node->position + glm::vec2{ 0, node_height_tiles * style.grid_size } + glm::vec2{ 0, style.header_offset * style.grid_size } - glm::vec2{ 0, header_box_size.y };
        
        if (style.header_outline) // TODO: a third section of the node atlas which has header elements
            addFrame(header_box_pos, header_box_size, node->colour * style.outline_colour_mult, false);
        if (style.header_fill)
            addFrame(header_box_pos, header_box_size, node->colour * style.outline_colour_mult, true);
        addText("test title", header_box_pos + glm::round(glm::vec2{ 0, static_cast<float>(style.header_height - 1) * 0.5f * style.grid_size }), style.text_colour);
        
        
        //     Ref<Node> node = *it;
    //     size_t box_width = (size_t)(node->last_size.x / style.grid_size);
    //     glm::vec2 box_base = (glm::round(node->position) * style.grid_size);
    //     glm::vec3 foreground_colour = getForegroundColour(node->palette_index);
    //     glm::vec3 background_colour = getBackgroundColour(foreground_colour);
    //
    //     addBlock(box_base, node->last_size, background_colour, false);
    //     addFrame(box_base, node->last_size, foreground_colour);
    //
    //     size_t box_height_lines = 0;
    //     size_t text_width = 0;
    //     text_width = (size_t)(style.font->getGlyphSize().x) * node->title.size();
    //     addBlock(box_base + glm::vec2{ style.grid_size - 4.0f, 0 }, glm::vec2{ ((size_t)(text_width / style.grid_size) + 2), 1 } *style.grid_size, foreground_colour, !node->highlighted);
    //     addText(node->title, box_base + (style.title_offset * style.grid_size), node->highlighted ? background_colour : foreground_colour);
    //     box_height_lines++;
    //
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
    style.font = new Font("res://engine/font.bmp", glm::ivec2{ 10, 18 });

    setStyle(style);
}
//
// void NodeView::addQuad(glm::vec2 position, glm::vec2 size, glm::vec4 colour, glm::vec3 tint, bool clip_uv, int uv_index)
// {
//     uint16_t v_off = static_cast<uint16_t>(vertices.size());
//     glm::vec4 segment_size = { glm::ceil(size.x / style.grid_size), glm::ceil(size.y / style.grid_size), 0, 0 };
//
//     glm::vec2 tl_uv = { 0, 1 };
//     glm::vec2 tr_uv = { 1, 1 };
//     glm::vec2 bl_uv = { 0, 0 };
//     glm::vec2 br_uv = { 1, 0 };
//
//     if (clip_uv)
//     {
//         glm::vec2 slice_offset = { uv_index % 3, 2.0f - (int)(uv_index / 3) };
//
//         tl_uv = (tl_uv + slice_offset) / 3.0f;
//         tr_uv = (tr_uv + slice_offset) / 3.0f;
//         bl_uv = (bl_uv + slice_offset) / 3.0f;
//         br_uv = (br_uv + slice_offset) / 3.0f;
//
//         segment_size += glm::vec4{ 2.0f, 2.0f, 0.0f, 0.0f };
//     }
//
//     vertices.push_back(Vertex{ { position.x, -position.y, 0, 1 }, colour, segment_size, glm::vec4(tint, 0), tl_uv });
//     vertices.push_back(Vertex{ { position.x + size.x, -position.y, 0, 1 }, colour, segment_size, glm::vec4(tint, 0), tr_uv });
//     vertices.push_back(Vertex{ { position.x, -position.y - size.y, 0, 1 }, colour, segment_size, glm::vec4(tint, 0), bl_uv });
//     vertices.push_back(Vertex{ { position.x + size.x, -position.y - size.y, 0, 1 }, colour, segment_size, glm::vec4(tint, 0), br_uv });
//
//     indices.push_back(v_off + 0);
//     indices.push_back(v_off + 3);
//     indices.push_back(v_off + 1);
//     indices.push_back(v_off + 0);
//     indices.push_back(v_off + 2);
//     indices.push_back(v_off + 3);
// }

void NodeView::addFrame(glm::vec2 position, glm::vec2 size, glm::vec3 tint, bool filled)
{
    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    
    glm::vec4 normal_value = { size.x, size.y, filled ? 0.1f : 0.0f, 0.0f };
    
    // top left
    vertices.push_back(Vertex{ { position.x, -position.y, 0, 1 }, glm::vec4{ tint, 0 }, normal_value, { 0, 0, 0, 1}, glm::vec2{ 0, 0 } });
    // top right
    vertices.push_back(Vertex{ { position.x + size.x, -position.y, 0, 1 }, glm::vec4{ tint, 0 }, normal_value, { 0, 0, 0, 1}, glm::vec2{ 1, 0 } });
    // bottom left
    vertices.push_back(Vertex{ { position.x, -position.y - size.y, 0, 1 }, glm::vec4{ tint, 0 }, normal_value, { 0, 0, 0, 1}, glm::vec2{ 0, 1 } });
    // bottom right
    vertices.push_back(Vertex{ { position.x + size.x, -position.y - size.y, 0, 1 }, glm::vec4{ tint, 0 }, normal_value, { 0, 0, 0, 1}, glm::vec2{ 1, 1 } });
    
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

void NodeView::addText(const string& text, glm::vec2 start, glm::vec3 tint)
{
    glm::vec2 position = start + style.text_offset + glm::vec2{ 0, (style.grid_size - style.font->getGlyphSize().y) };
    for (char c : text)
    {
        glm::vec2 uv_base = style.font->getGlyphUVOffset(c);
        glm::vec2 uv_size = style.font->getGlyphUVSize();

        glm::vec2 uv_bl = flipUV(uv_base + glm::vec2{ 0, uv_size.y });
        glm::vec2 uv_br = flipUV(uv_base + uv_size);
        glm::vec2 uv_tl = flipUV(uv_base);
        glm::vec2 uv_tr = flipUV(uv_base + glm::vec2{ uv_size.x, 0 });

        glm::vec2 char_size = style.font->getGlyphSize();
        glm::vec4 pos_bl = { position.x, (-position.y - char_size.y), 0, 1 };
        glm::vec4 pos_br = { position.x + char_size.x, (-position.y - char_size.y), 0, 1 };
        glm::vec4 pos_tl = { position.x, -position.y, 0, 1 };
        glm::vec4 pos_tr = { position.x + char_size.x, -position.y, 0, 1 };

        uint16_t v_off = static_cast<uint16_t>(vertices.size());
        vertices.push_back(Vertex{ pos_bl, glm::vec4(tint, 1), { 0, 0, 0.5f, 0 }, {}, uv_bl });
        vertices.push_back(Vertex{ pos_br, glm::vec4(tint, 1), { 0, 0, 0.5f, 0 }, {}, uv_br });
        vertices.push_back(Vertex{ pos_tl, glm::vec4(tint, 1), { 0, 0, 0.5f, 0 }, {}, uv_tl });
        vertices.push_back(Vertex{ pos_tr, glm::vec4(tint, 1), { 0, 0, 0.5f, 0 }, {}, uv_tr });

        indices.push_back(v_off + 1);
        indices.push_back(v_off + 3);
        indices.push_back(v_off + 0);
        indices.push_back(v_off + 3);
        indices.push_back(v_off + 2);
        indices.push_back(v_off + 0);
        
        position.x += style.font->getGlyphSize().x + style.text_spacing;
    }
}

NodeView::~NodeView()
{
    style.font = nullptr;
    style.node_atlas = nullptr;
    material = nullptr;
    nodes.clear();
}
