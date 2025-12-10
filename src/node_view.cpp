#include "node_view.h"

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

constexpr glm::vec2 character_size = { 10.0f, 18.0f };
constexpr float character_padding = 1.0f;
constexpr float grid_size = 24.0f;
constexpr float layer_offset = 0.01f;
constexpr float character_top_inset = 4.0f;
constexpr size_t v_i_buffer_rounding_size = 256;

NodeView::NodeView() : Object(nullptr, nullptr)
{
    material = new Material(new Shader("res://node_shader", false), VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL, VK_FALSE, VK_FALSE);
    Ref<Sampler> sampler = new Sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    material->setTexture("node_atlas", new Texture("res://newnodes.png"));
    material->setSampler("node_atlas", sampler);
    material->setTexture("text_atlas", new Texture("res://font.bmp"));
    material->setSampler("text_atlas", sampler);
    material->setTexture("link_atlas", new Texture("res://nodelinks.png"));
    material->setSampler("link_atlas", sampler);

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

void NodeView::addQuad(glm::vec2 position, glm::vec2 size, glm::vec4 colour, glm::vec3 tint, bool clip_uv, int uv_index)
{
    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    glm::vec3 segment_size = { glm::ceil(size.x / grid_size), glm::ceil(size.y / grid_size), 0 };

    glm::vec2 tl_uv = { 0, 1 };
    glm::vec2 tr_uv = { 1, 1 };
    glm::vec2 bl_uv = { 0, 0 };
    glm::vec2 br_uv = { 1, 0 };

    if (clip_uv)
    {
        glm::vec2 slice_offset = { uv_index % 3, 2.0f - (int)(uv_index / 3) };

        tl_uv = (tl_uv + slice_offset) / 3.0f;
        tr_uv = (tr_uv + slice_offset) / 3.0f;
        bl_uv = (bl_uv + slice_offset) / 3.0f;
        br_uv = (br_uv + slice_offset) / 3.0f;

        segment_size += glm::vec3{ 2.0f, 2.0f, 0.0f };
    }

    vertices.push_back(Vertex{ { position.x, -position.y, 0 }, colour, segment_size, tint, tl_uv });
    vertices.push_back(Vertex{ { position.x + size.x, -position.y, 0 }, colour, segment_size, tint, tr_uv });
    vertices.push_back(Vertex{ { position.x, -position.y - size.y, 0 }, colour, segment_size, tint, bl_uv });
    vertices.push_back(Vertex{ { position.x + size.x, -position.y - size.y, 0 }, colour, segment_size, tint, br_uv });

    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

void NodeView::addFrame(glm::vec2 position, glm::vec2 size, glm::vec3 tint)
{
    addQuad(position, size, { 1, 0, 0, 0 }, tint, false, 0);
}

void NodeView::addBadge(glm::vec2 position, glm::vec2 size, glm::vec3 tint)
{
    addQuad(position, size, { 0, 1, 0, 0 }, tint, false, 0);
}

void NodeView::addBlock(glm::vec2 position, glm::vec2 size, glm::vec3 tint, bool outline)
{
    addQuad(position, size, { 0, 4, 0, 0 }, tint, true, 4);
    if (outline)
        addQuad(position + 2.0f, size - 4.0f, { 0, 1, 0, 0 }, background_colour, true, 4);
}

void NodeView::addPin(glm::vec2 position, glm::vec3 tint, int type, bool filled)
{
    addQuad(position, glm::vec2{ grid_size, grid_size }, { 0, 0, filled ? 10 : 1, 0 }, tint, true, type);
}

static glm::vec2 flipUV(glm::vec2 v)
{
    return { v.x, 1.0f - v.y };
}

void NodeView::addCharacter(char c, glm::vec2 position, glm::vec3 tint)
{
    float chars_horizontal = 32.0f;
    float chars_height = 320.0f / 18.0f;
    glm::vec2 uv_base = { glm::fract(c / chars_horizontal), glm::floor(c / chars_horizontal) / chars_height };
    glm::vec2 uv_size = { 1.0f / chars_horizontal, 1.0f / chars_height };

    glm::vec2 uv_bl = flipUV(uv_base + glm::vec2{ 0, uv_size.y });
    glm::vec2 uv_br = flipUV(uv_base + uv_size);
    glm::vec2 uv_tl = flipUV(uv_base);
    glm::vec2 uv_tr = flipUV(uv_base + glm::vec2{ uv_size.x, 0 });

    float top_inset = character_top_inset - character_padding;
    glm::vec3 pos_bl = { position.x, (-position.y - character_size.y) - top_inset, 0};
    glm::vec3 pos_br = { position.x + character_size.x, (-position.y - character_size.y) - top_inset, 0 };
    glm::vec3 pos_tl = { position.x, -position.y - top_inset, 0 };
    glm::vec3 pos_tr = { position.x + character_size.x, -position.y - top_inset, 0 };

    uint16_t v_off = static_cast<uint16_t>(vertices.size());
    vertices.push_back(Vertex{ pos_bl, glm::vec4(tint, 1), { 0, 0, 0.5f }, {}, uv_bl });
    vertices.push_back(Vertex{ pos_br, glm::vec4(tint, 1), { 0, 0, 0.5f }, {}, uv_br });
    vertices.push_back(Vertex{ pos_tl, glm::vec4(tint, 1), { 0, 0, 0.5f }, {}, uv_tl });
    vertices.push_back(Vertex{ pos_tr, glm::vec4(tint, 1), { 0, 0, 0.5f }, {}, uv_tr });

    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

void NodeView::addText(std::string text, glm::vec2 start, glm::vec3 tint)
{
    glm::vec2 position = start;
    for (char c : text)
    {
        addCharacter(c, position, tint);
        position.x += character_size.x;
    }
}

void NodeView::addLinkElem(glm::vec2 position, glm::vec3 tint, int type)
{
    static const glm::vec2 uv_sets[18][4] =
    {   // tl       tr        bl        br
        { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } }, // outgoing link    - 00
        { { 1, 0 }, { 0, 0 }, { 1, 1 }, { 0, 1 } }, // incoming link    - 01
        { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } }, // horizontal link  - 02
        { { 1, 1 }, { 1, 0 }, { 2, 1 }, { 2, 0 } }, // vertical link    - 03
        { { 2, 0 }, { 3, 0 }, { 2, 1 }, { 3, 1 } }, // left-down angle  - 04
        { { 2, 1 }, { 3, 1 }, { 2, 0 }, { 3, 0 } }, // left-up angle    - 05
        { { 3, 0 }, { 2, 0 }, { 3, 1 }, { 2, 1 } }, // right-down angle - 06
        { { 3, 1 }, { 2, 1 }, { 3, 0 }, { 2, 0 } }, // right-up angle   - 07
        { { 3, 0 }, { 4, 0 }, { 3, 1 }, { 4, 1 } }, // tl-br slant      - 08
        { { 3, 1 }, { 4, 1 }, { 3, 0 }, { 4, 0 } }, // tr-bl slant      - 09
        { { 0, 1 }, { 1, 1 }, { 0, 2 }, { 1, 2 } }, // left-br angle    - 10
        { { 1, 1 }, { 0, 1 }, { 1, 2 }, { 0, 2 } }, // right-bl angle   - 11
        { { 0, 2 }, { 1, 2 }, { 0, 1 }, { 1, 1 } }, // left-tr angle    - 12
        { { 1, 2 }, { 0, 2 }, { 1, 1 }, { 0, 1 } }, // right-tl angle   - 13
        { { 0, 1 }, { 0, 2 }, { 1, 1 }, { 1, 2 }, }, // top-br angle    - 14
        { { 0, 2 }, { 0, 1 }, { 1, 2 }, { 1, 1 }, }, // top-bl angle    - 15
        { { 1, 1 }, { 1, 2 }, { 0, 1 }, { 0, 2 }, }, // bottom-tr angle - 16
        { { 1, 2 }, { 1, 1 }, { 0, 2 }, { 0, 1 }, }, // bottom-tl angle - 17
    };

    {
        glm::vec2 uv_tl = uv_sets[type][0];
        glm::vec2 uv_tr = uv_sets[type][1];
        glm::vec2 uv_bl = uv_sets[type][2];
        glm::vec2 uv_br = uv_sets[type][3];

        glm::vec3 pos_tl = { position.x, -position.y, 0 };
        glm::vec3 pos_tr = { position.x + grid_size, -position.y, 0 };
        glm::vec3 pos_bl = { position.x, -position.y - grid_size, 0 };
        glm::vec3 pos_br = { position.x + grid_size, -position.y - grid_size, 0 };

        uint16_t v_off = static_cast<uint16_t>(vertices.size());
        vertices.push_back(Vertex{ pos_bl, glm::vec4(tint, 1), { 0, 0, 1 }, {}, uv_bl });
        vertices.push_back(Vertex{ pos_br, glm::vec4(tint, 1), { 0, 0, 1 }, {}, uv_br });
        vertices.push_back(Vertex{ pos_tl, glm::vec4(tint, 1), { 0, 0, 1 }, {}, uv_tl });
        vertices.push_back(Vertex{ pos_tr, glm::vec4(tint, 1), { 0, 0, 1 }, {}, uv_tr });

        indices.push_back(v_off + 0);
        indices.push_back(v_off + 3);
        indices.push_back(v_off + 1);
        indices.push_back(v_off + 0);
        indices.push_back(v_off + 2);
        indices.push_back(v_off + 3);
    }
}

void NodeView::addLink(glm::ivec2 grid_start, glm::ivec2 grid_end, glm::vec3 tint)
{
    glm::ivec2 difference = grid_end - grid_start;
    if (difference.x <= 0)
        return;
    if (difference.x <= 1 && difference.y != 0)
        return;

    glm::vec2 position = glm::vec2(grid_start) * grid_size;
    addLinkElem(position, tint, 0);
    addLinkElem(glm::vec2(grid_end) * grid_size, tint, 1);

    if (difference.y == 0)
    {
        while (difference.x > 1)
        {
            grid_start.x++;
            addLinkElem(glm::vec2(grid_start) * grid_size, tint, 2);
            difference.x--;
        }
        return;
    }

    if (difference.x < abs(difference.y) + 2)
    {
        bool positive = difference.y > 0;
        int y_delta = positive ? -1 : 1;
        if (difference.x == 2)
        {
            grid_start.x++;
            addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 4 : 5);
            difference.x--;
            while (abs(difference.y) > 1)
            {
                grid_start.y -= y_delta;
                addLinkElem(glm::vec2(grid_start) * grid_size, tint, 3);
                difference.y += y_delta;
            }
            grid_start.y = grid_end.y;
            addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 7 : 6);
            return;
        }

        grid_start.x++;
        addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 4 : 5);
        difference.x--;

        while (difference.x < abs(difference.y))
        {
            grid_start.y -= y_delta;
            addLinkElem(glm::vec2(grid_start) * grid_size, tint, 3);
            difference.y += y_delta;
        }

        grid_start.y -= y_delta;
        addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 14 : 16);
        addLinkElem((glm::vec2(grid_start) + glm::vec2{ 0.5f, -y_delta * 0.5f }) * grid_size, tint, positive ? 8 : 9);
        difference.y += y_delta;

        while (abs(difference.x) > 2)
        {
            grid_start.x++;
            grid_start.y -= y_delta;
            addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 8 : 9);
            addLinkElem((glm::vec2(grid_start) + glm::vec2{ 0.5f, -y_delta * 0.5f }) * grid_size, tint, positive ? 8 : 9);
            difference.x--;
            difference.y += y_delta;
        }

        grid_start.x++;
        grid_start.y -= y_delta;
        addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 13 : 11);
        return;
    }
    else
    {
        int horizontal_space = (difference.x - (abs(difference.y) + 2)) / 2;
        while (horizontal_space > 0)
        {
            grid_start.x++;
            addLinkElem(glm::vec2(grid_start) * grid_size, tint, 2);
            difference.x--;
            horizontal_space--;
        }

        bool positive = difference.y > 0;
        int y_delta = positive ? -1 : 1;
        grid_start.x++;
        addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 10 : 12);
        addLinkElem((glm::vec2(grid_start) + glm::vec2{ 0.5f, -y_delta * 0.5f }) * grid_size, tint, positive ? 8 : 9);
        difference.x--;
        while (abs(difference.y) > 1)
        {
            grid_start.x++;
            grid_start.y -= y_delta;
            addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 8 : 9);
            addLinkElem((glm::vec2(grid_start) + glm::vec2{ 0.5f, -y_delta * 0.5f }) * grid_size, tint, positive ? 8 : 9);
            difference.x--;
            difference.y += y_delta;
        }
        grid_start.x++;
        grid_start.y -= y_delta;
        addLinkElem(glm::vec2(grid_start) * grid_size, tint, positive ? 13 : 11);

        while (difference.x > 1)
        {
            grid_start.x++;
            addLinkElem(glm::vec2(grid_start) * grid_size, tint, 2);
            difference.x--;
        }

        return;
    }
}

// TODO: new font
void NodeView::updateMesh()
{
    glm::vec3 new_background_colour = palette.empty() ?
        glm::vec3{ 0.005f, 0.005f, 0.005f }
    : palette[0];

    if (new_background_colour != background_colour)
        material->setVec4Uniform("background_colour", glm::vec4(new_background_colour, 1));
    background_colour = new_background_colour;

    if (nodes.empty())
    {
        mesh = nullptr;
        return;
    }
    
    vertices.clear();
    indices.clear();

    // prepass to calculate node sizes
    for (Ref<Node> node : nodes)
    {
        size_t box_width = 3;
        size_t text_width = 0;
        text_width = (size_t)(character_size.x) * node->title.size();
        box_width = glm::max(box_width, ((size_t)(text_width / grid_size) + 4));
        for (const NodeElement& element : node->elements)
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

        size_t box_height_lines = node->elements.size() + 2;
        node->last_size = glm::vec2{ box_width, box_height_lines } * grid_size;
    }

    // draw links
    for (Link& link : links)
    {
        Ref<Node> start = link.start_node;
        Ref<Node> end = link.end_node;

        glm::ivec2 start_pos = glm::round(start->position) + ((start->last_size * glm::vec2{ 1, 0 }) / grid_size);
        start_pos.x--;
        int start_offset = 0;
        for (int output_num = 0; start_offset < start->elements.size(); ++start_offset)
        {
            if (start->elements[start_offset].type == ELEMENT_OUTPUT)
                ++output_num;
            if (output_num == link.start_output)
                break;
        }
        start_pos.y += (int)(start->last_size.y / grid_size) - (start_offset + 1);

        glm::ivec2 end_pos = glm::round(end->position);
        int end_offset = 0;
        for (int input_num = 0; end_offset < end->elements.size(); ++end_offset)
        {
            if (end->elements[end_offset].type == ELEMENT_INPUT)
                ++input_num;
            if (input_num == link.end_input)
                break;
        }
        end_pos.y += (int)(end->last_size.y / grid_size) - (end_offset + 3);

        glm::vec3 foreground_colour = (palette.size() < 2) ?
            glm::vec3{ 1.000f, 0.319f, 0.000f }
        : palette[glm::clamp(link.palette_index, 0, (int)palette.size() - 1)];
        addLink(start_pos, end_pos, foreground_colour);
    }

    // draw nodes
    for (auto it = nodes.rbegin(); it != nodes.rend(); it++)
    {
        Ref<Node> node = *it;
        size_t box_width = (size_t)(node->last_size.x / grid_size);
        glm::vec2 box_base = (glm::round(node->position) * grid_size);

        glm::vec3 foreground_colour = (palette.size() < 2) ?
            glm::vec3{ 1.000f, 0.319f, 0.000f }
        : palette[glm::clamp(node->palette_index, 0, (int)palette.size() - 1)];
        addBlock(box_base, node->last_size, background_colour, false);
        addFrame(box_base, node->last_size, foreground_colour);

        size_t box_height_lines = 0;
        size_t text_width = 0;
        text_width = (size_t)(character_size.x) * node->title.size();
        addBlock(box_base + glm::vec2{ grid_size - 4.0f, 0 }, glm::vec2{ ((size_t)(text_width / grid_size) + 2), 1 } * grid_size, foreground_colour, !node->highlighted);
        addText(node->title, box_base + glm::vec2{ grid_size * 1.5f, 0 }, node->highlighted ? background_colour : foreground_colour);
        box_height_lines++;

        for (const NodeElement& element : node->elements)
        {
            glm::vec2 line_pos_base = box_base + glm::vec2{ 0, box_height_lines * grid_size };
            switch (element.type)
            {
            case ELEMENT_INPUT:
                addPin(line_pos_base, foreground_colour, element.pin_type, element.pin_solid);
                addText(element.text, line_pos_base + glm::vec2{ 6.0f + grid_size, 0 }, foreground_colour);
                break;
            case ELEMENT_OUTPUT:
                addPin(line_pos_base + glm::vec2{ box_width - 1, 0 } * grid_size, foreground_colour, element.pin_type, element.pin_solid);
                text_width = (size_t)(character_size.x) * element.text.size();
                addText(element.text, line_pos_base + glm::vec2{ (box_width * grid_size) - (6.0f + grid_size + text_width), 0.0f }, foreground_colour);
                break;
            case ELEMENT_BLOCK:
                addBlock(line_pos_base + glm::vec2{ grid_size / 2.0f, 0 }, glm::vec2{ box_width - 1, 1 } * grid_size, foreground_colour, false);
                addText(element.text, line_pos_base + glm::vec2{ 6.0f + grid_size, 0 }, background_colour);
                break;
            case ELEMENT_TEXT:
                addText(element.text, line_pos_base + glm::vec2{ 6.0f + grid_size, 0 }, foreground_colour);
                break;
            }
            box_height_lines++;
        }
    }

    size_t vertices_rounded_up = ((vertices.size() / v_i_buffer_rounding_size) + 2) * v_i_buffer_rounding_size;
    size_t indices_rounded_up = ((indices.size() / v_i_buffer_rounding_size) + 2) * v_i_buffer_rounding_size;

    if (mesh.isValid())
        mesh->updateData(vertices, indices, vertices_rounded_up, indices_rounded_up);
    else
        mesh = new Mesh(vertices, indices, true);
}

Ref<NodeView::Node> NodeView::select(glm::vec2 world_position)
{
    size_t node_index = 0;
    for (size_t node_index = 0; node_index < nodes.size(); ++node_index)
    {
        Ref<Node> node = nodes[node_index];
        glm::vec2 node_position = node->position * grid_size;
        if (world_position.x < node_position.x ||
            world_position.y < node_position.y ||
            world_position.x > node_position.x + node->last_size.x ||
            world_position.y > node_position.y + node->last_size.y)
        {
            continue;
        }

        // re-order nodes list
        nodes.erase(nodes.begin() + node_index);
        nodes.insert(nodes.begin(), node);

        return node;
    }

    return nullptr;

}
