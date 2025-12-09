#include "node_view.h"

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

constexpr glm::vec2 character_size = { 10.0f, 18.0f };
constexpr float character_padding = 1.0f;
constexpr float grid_size = 24.0f;
constexpr float layer_offset = 0.1f;
constexpr float character_top_inset = 4.0f;

NodeView::NodeView() : Object(nullptr, nullptr)
{
    material = new Material(new Shader("res://node_shader", false), VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL);
    material->setTexture("sliced_texture", new Texture("res://newnodes.png"));
    material->setSampler("sliced_texture", new Sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT));
    material->setTexture("text_atlas", new Texture("res://font.bmp"));
    material->setSampler("text_atlas", new Sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT));

    palette =
    {
        { 0.005f, 0.005f, 0.005f },
        { 1.000f, 0.319f, 0.000f },
        { 1.000f, 0.133f, 0.000f },
        { 0.624f, 0.027f, 0.012f },
        { 0.127f, 0.027f, 0.304f },
        { 0.175f, 0.451f, 0.005f },
        { 0.292f, 0.041f, 0.117f }
    };

    updateMesh();
}

void NodeView::addQuad(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 colour, glm::vec3 tint, bool clip_uv, vector<Vertex>& vertices, vector<uint16_t>& indices)
{
    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    glm::vec3 segment_size = { size.x / grid_size, size.y / grid_size, 0 };
    float z_height = layer * layer_offset;

    glm::vec2 tl_uv = { 0, 1 };
    glm::vec2 tr_uv = { 1, 1 };
    glm::vec2 bl_uv = { 0, 0 };
    glm::vec2 br_uv = { 1, 0 };

    if (clip_uv)
    {
        tl_uv = (tl_uv + 1.0f) / 3.0f;
        tr_uv = (tr_uv + 1.0f) / 3.0f;
        bl_uv = (bl_uv + 1.0f) / 3.0f;
        br_uv = (br_uv + 1.0f) / 3.0f;
    }

    vertices.push_back(Vertex{ { position.x, -position.y, z_height }, colour, segment_size, tint, tl_uv });
    vertices.push_back(Vertex{ { position.x + size.x, -position.y, z_height }, colour, segment_size, tint, tr_uv });
    vertices.push_back(Vertex{ { position.x, -position.y - size.y, z_height }, colour, segment_size, tint, bl_uv });
    vertices.push_back(Vertex{ { position.x + size.x, -position.y - size.y, z_height }, colour, segment_size, tint, br_uv });

    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

void NodeView::addFrame(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 tint)
{
    addQuad(position, size, layer, { 1, 0, 0 }, tint, false, vertices, indices);
}

void NodeView::addBadge(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 tint)
{
    addQuad(position, size, layer, { 0, 1, 0 }, tint, false, vertices, indices);
}

void NodeView::addBlock(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 tint)
{
    addQuad(position, size, layer, { 0, 1, 0 }, tint, true, vertices, indices);
}

void NodeView::addPin(glm::vec2 position, int layer, glm::vec3 tint)
{
    addQuad(position, glm::vec2{ grid_size, grid_size }, layer, { 0, 0, 1 }, tint, false, vertices, indices);
}

glm::vec2 flipUV(glm::vec2 v)
{
    return { v.x, 1.0f - v.y };
}

void NodeView::addCharacter(char c, glm::vec2 position, int layer, glm::vec3 tint)
{
    float chars_horizontal = 32.0f;
    float chars_height = 320.0f / 18.0f;
    glm::vec2 uv_base = { glm::fract(c / chars_horizontal), glm::floor(c / chars_horizontal) / chars_height };
    glm::vec2 uv_size = { 1.0f / chars_horizontal, 1.0f / chars_height };

    glm::vec2 uv_bl = flipUV(uv_base + glm::vec2{ 0, uv_size.y });
    glm::vec2 uv_br = flipUV(uv_base + uv_size);
    glm::vec2 uv_tl = flipUV(uv_base);
    glm::vec2 uv_tr = flipUV(uv_base + glm::vec2{ uv_size.x, 0 });

    float z_height = layer * layer_offset;
    float top_inset = character_top_inset - character_padding;
    glm::vec3 pos_bl = { position.x, (-position.y - character_size.y) - top_inset, z_height};
    glm::vec3 pos_br = { position.x + character_size.x, (-position.y - character_size.y) - top_inset, z_height };
    glm::vec3 pos_tl = { position.x, -position.y - top_inset, z_height };
    glm::vec3 pos_tr = { position.x + character_size.x, -position.y - top_inset, z_height };

    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    vertices.push_back(Vertex{ pos_bl, tint, { 0, 0, 1 }, {}, uv_bl });
    vertices.push_back(Vertex{ pos_br, tint, { 0, 0, 1 }, {}, uv_br });
    vertices.push_back(Vertex{ pos_tl, tint, { 0, 0, 1 }, {}, uv_tl });
    vertices.push_back(Vertex{ pos_tr, tint, { 0, 0, 1 }, {}, uv_tr });

    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

void NodeView::addText(std::string text, glm::vec2 start, int layer, glm::vec3 tint)
{
    glm::vec2 position = start;
    for (char c : text)
    {
        addCharacter(c, position, layer, tint);
        position.x += character_size.x;
    }
}

// TODO: connections between nodes
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
    for (Node& box : nodes)
    {
        size_t box_width = 3;
        size_t text_width = 0;
        text_width = (size_t)(character_size.x) * box.title.size();
        box_width = glm::max(box_width, ((size_t)(text_width / grid_size) + 4));
        for (const NodeElement& element : box.elements)
        {
            switch (element.type)
            {
            case ELEMENT_TEXT:
            case ELEMENT_BLOCK:
            case ELEMENT_OUTPUT:
            case ELEMENT_INPUT:
                text_width = (size_t)(character_size.x) * element.text.size();
                box_width = glm::max(box_width, ((size_t)(text_width / grid_size) + 4));
                break;
            }
        }

        glm::vec3 foreground_colour = (palette.size() < 2) ? 
              glm::vec3{ 1.000f, 0.319f, 0.000f }
            : palette[glm::clamp(box.palette_index, 0, (int)palette.size() - 1)];
        glm::vec3 background_colour = palette.empty() ?
              glm::vec3{ 0.005f, 0.005f, 0.005f }
            : palette[0];

        size_t box_height_lines = 0;
        glm::vec2 box_base = (glm::round(box.position) * grid_size);
        text_width = (size_t)(character_size.x) * box.title.size();
        addText(box.title, box_base + glm::vec2{ grid_size * 1.5f, 0 }, 2, box.highlighted ? background_colour : foreground_colour);
        addBlock(box_base + glm::vec2{ grid_size - 4.0f, 0 }, glm::vec2{ ((size_t)(text_width / grid_size) + 2), 1 } *grid_size, 1, box.highlighted ? foreground_colour : background_colour);
        //addBadge(box_base - glm::vec2{ 0, grid_size }, glm::vec2{ ((size_t)(text_width / grid_size) + 4), 3 } * grid_size, 1, box.highlighted ? foreground_colour : background_colour);
        box_height_lines += 1;

        for (const NodeElement& element : box.elements)
        {
            glm::vec2 line_pos_base = box_base + glm::vec2{0, box_height_lines * grid_size};
            switch (element.type)
            {
            case ELEMENT_INPUT:
                addPin(line_pos_base, 1, foreground_colour);
                addText(element.text, line_pos_base + glm::vec2{ 6.0f + grid_size, 0 }, 2, foreground_colour);
                break;
            case ELEMENT_OUTPUT:
                addPin(line_pos_base + glm::vec2{ box_width - 1, 0 } * grid_size, 1, foreground_colour);
                text_width = (size_t)(character_size.x) * element.text.size();
                addText(element.text, line_pos_base + glm::vec2{ (box_width * grid_size) - (6.0f + grid_size + text_width), 0.0f }, 2, foreground_colour);
                break;
            case ELEMENT_BLOCK:
                addBlock(line_pos_base + glm::vec2{ grid_size / 2.0f, 0 }, glm::vec2{ box_width - 1, 1 } * grid_size, 1, foreground_colour);
                addText(element.text, line_pos_base + glm::vec2{ 6.0f + grid_size, 0 }, 2, background_colour);
                break;
            case ELEMENT_TEXT:
                addText(element.text, line_pos_base + glm::vec2{ 6.0f + grid_size, 0 }, 2, foreground_colour);
                break;
            }

            box_height_lines++;
        }

        box_height_lines++;
        addFrame(box_base, glm::vec2{ box_width, box_height_lines + 1 } * grid_size, 0, foreground_colour);
        addBlock(box_base, glm::vec2{box_width, box_height_lines + 1} * grid_size, -1, background_colour);
    }

    if (mesh.isValid())
        mesh->updateData(vertices, indices);
    else
        mesh = new Mesh(vertices, indices, true);
}
