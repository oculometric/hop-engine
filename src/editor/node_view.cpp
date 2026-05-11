#include "node_view.h"

#include "engine.h"
#include "user_interface.h"

#include <glm/gtc/integer.hpp>

using namespace HopEngine;
using namespace std;

constexpr size_t v_i_buffer_rounding_size = 256;
constexpr float RENDER_MODE_BOX           = 0.0f;
constexpr float RENDER_MODE_TEXT          = 1.0f;
constexpr float RENDER_MODE_PINS          = 2.0f;
constexpr float RENDER_MODE_BACKGROUND    = 3.0f;
constexpr float RENDER_MODE_UI            = 4.0f;

static glm::vec2 flipUV(glm::vec2 v) { return { v.x, 1.0f - v.y }; }

void NodeView::setStyle(Ref<Style> new_style)
{
    material->setTexture("text_atlas", new_style->font->getAtlas().strong());
    material->setTexture("node_atlas", new_style->node_atlas);
    material->setTexture("extra_atlas", new_style->extra_atlas);
    material->setTexture("ui_atlas", new_style->ui_atlas);

    material->setFloatUniform("grid_size", new_style->grid_size);
    material->setVec3Uniform("outline_colour", new_style->outline_colour);
    material->setIntUniform("outline_style", new_style->outline_style);
    material->setFloatUniform("outline_modulate", new_style->outline_colour_mult);
    material->setVec3Uniform("fill_colour", new_style->fill_colour);
    material->setFloatUniform("fill_modulate", new_style->fill_colour_mult);
    material->setFloatUniform("grid_dots_modulate", new_style->grid_dots_modulate);
    material->setIntUniform("grid_scale", new_style->grid_scale);
    material->setVec3Uniform("outline_colour_highlight", new_style->outline_colour_highlight);
    material->setVec3Uniform("background_colour", new_style->background_colour);

    style = new_style;
    updateMesh();
}

float encodeFourBits(bool a, bool b, bool c, bool d)
{
    int i = (a << 0) | (b << 1) | (c << 2) | (d << 3);
    return static_cast<float>(i);
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

    if (style->show_grid)
    {
        const glm::vec2 half_viewport = glm::vec2{ 1000000.0f };
        addQuad(-half_viewport, half_viewport * 2.0f, { 0, 0 }, { 1, 1 }, style->grid_colour,
            RENDER_MODE_BACKGROUND);
    }

    // draw nodes
    for (auto& node : nodes)
        drawNode(node, 1);      // shadow pass
    for (auto& node : nodes)
        drawNode(node, 2);      // link pass
    for (auto& node : nodes)
        drawNode(node, 0);      // main pass

    if (draw_temp_link)
        addLink(temp_link_start * style->grid_size, temp_link_end * style->grid_size);

    size_t vertices_rounded_up =
        ((vertices.size() / v_i_buffer_rounding_size) + 2) * v_i_buffer_rounding_size;
    size_t indices_rounded_up =
        ((indices.size() / v_i_buffer_rounding_size) + 2) * v_i_buffer_rounding_size;

    if (mesh) mesh->updateData(vertices, indices, vertices_rounded_up, indices_rounded_up);
    else
        mesh = new Mesh(vertices, indices, true);
}

void NodeView::awake()
{
    StaticMeshComponent::awake();
    material             = new Material(new Shader("res://engine/shaders/node_shader.glsl"),
                    Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthTest(false).depthWrite(false));
    Ref<Sampler> sampler = Engine::getSampler(Sampler::FILTER_NEAREST);
    material->setSampler("node_atlas", sampler);
    material->setSampler("text_atlas", sampler);
    material->setSampler("extra_atlas", sampler);
    material->setSampler("ui_atlas", sampler);

    style = Style::deserialise("res://engine/colourful_nodes.hsty");
    if (!style->font)
        style->font = new Font("res://engine/textures/font_IBM_XGA_AI_12x23.png", { 14, 25 });
    if (!style->node_atlas)
        style->node_atlas = Engine::loadTexture("res://engine/textures/node_atlas.png");
    if (!style->extra_atlas)
        style->extra_atlas = Engine::loadTexture("res://engine/textures/extra_atlas.png");
    if (!style->ui_atlas)
        style->ui_atlas = Engine::loadTexture("res://engine/textures/ui_atlas.png");
    setStyle(style);
}

void NodeView::addQuadRaw(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2 norm_xy,
    glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec3 colour, float mode, glm::vec3 extra)
{
    uint16_t v_off = static_cast<uint16_t>(vertices.size());

    glm::vec4 normal_value  = { norm_xy, mode, 0.0f };
    glm::vec4 tangent_value = { extra, 1 };
    glm::vec4 colour_value  = glm::vec4{ colour, 1 };

    // top left
    vertices.push_back(Mesh::Vertex{
        { p1.x, -p1.y, 0, 1 },
        colour_value, normal_value, tangent_value, { uv_tl, 0 }
    });
    // top right
    vertices.push_back(Mesh::Vertex{
        { p2.x, -p2.y, 0, 1 },
        colour_value, normal_value, tangent_value,
        { uv_br.x, uv_tl.y, 0 }
    });
    // bottom left
    vertices.push_back(Mesh::Vertex{
        { p3.x, -p3.y, 0, 1 },
        colour_value, normal_value, tangent_value,
        { uv_tl.x, uv_br.y, 0 }
    });
    // bottom right
    vertices.push_back(Mesh::Vertex{
        { p4.x, -p4.y, 0, 1 },
        colour_value, normal_value, tangent_value, { uv_br, 0 }
    });

    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

void NodeView::addQuad(glm::vec2 position, glm::vec2 size, glm::vec2 uv_tl, glm::vec2 uv_br,
    glm::vec3 colour, float mode, glm::vec3 extra)
{
    addQuadRaw({ position.x, position.y }, { position.x + size.x, position.y },
        { position.x, position.y + size.y }, { position.x + size.x, position.y + size.y }, size, uv_tl,
        uv_br, colour, mode, extra);
}

void NodeView::addPin(glm::vec2 position, glm::vec3 tint, int type, bool filled)
{
    glm::vec2 seg_size = glm::vec2(style->extra_atlas->getSize()) / 3.0f;
    glm::vec2 uv_size  = glm::vec2{ 1.0f / 3.0f };
    glm::vec2 uv_base  = glm::vec2{ uv_size.x * (type % 3), uv_size.y * (type / 3) };
    addQuad(position + glm::round((style->grid_size - seg_size) * 0.5f), seg_size, flipUV(uv_base),
        flipUV(uv_base + uv_size), tint, RENDER_MODE_PINS, { filled ? 1 : 0, 0, 0 });
}

void NodeView::addText(const string& text, glm::vec2 _start, glm::vec3 tint, int align)
{
    const glm::vec2 uv_size   = style->font->getGlyphUVSize();
    const glm::vec2 char_size = style->font->getGlyphSize();

    glm::vec2 start = _start;
    float width = ((text.size() + 1) * (style->font->getGlyphSize().x + style->text_spacing - 2.0f)) -
                  (style->text_spacing - 2.0f);
    if (align > 0) start.x -= width;
    else if (align == 0)
        start.x -= glm::round(width / 2.0f);

    glm::vec2 position = start + style->text_offset +
                         glm::vec2{ 0, (style->grid_size - style->font->getGlyphSize().y) * 0.5f };
    for (char c : text)
    {
        glm::vec2 uv_base = style->font->getGlyphUVOffset(c);

        glm::vec2 uv_br  = flipUV(uv_base + uv_size);
        glm::vec2 uv_tl  = flipUV(uv_base);
        glm::vec4 pos_tl = { position.x, position.y, 0, 1 };

        addQuad(pos_tl, char_size, uv_tl, uv_br, tint, RENDER_MODE_TEXT);

        position.x += style->font->getGlyphSize().x + style->text_spacing - 2.0f;
    }
}

void NodeView::addLink(glm::vec2 link_start, glm::vec2 link_end, bool translucent)
{
    float cos_a = glm::dot(glm::normalize(link_end - link_start), { 1, 0 });
    float sin_a = glm::dot(glm::normalize(link_end - link_start), { 0, 1 });
    float cps   = cos_a + sin_a;
    float cms   = cos_a - sin_a;
    float smc   = -cms;

    float radius = 2.0f;
    addQuadRaw(
        { link_start + glm::vec2{ -radius * cms, -radius * cps } },
        { link_start + glm::vec2{ -radius * cps, -radius * smc } },
        { link_end + glm::vec2{ radius * cps, radius * smc } },
        { link_end + glm::vec2{ radius * cms, radius * cps } },
        { 1, 1 }, { 0, 0 }, { 1, 1 },
        translucent ? glm::vec3{ 0.01f, 0.01f, 0.01f } : glm::vec3{ 0, 0, 0 },
        RENDER_MODE_BOX, { 0, translucent ? 3 : 1, 0 });
}

void NodeView::drawNode(WeakRef<Node> node, int pass)
{
    const glm::vec2 position = node->position * style->grid_size;
    float node_width_tiles     = node->size.x;
    int node_height_tiles = static_cast<int>(node->elements.size()) + style->after_header_spacing +
                            style->after_elements_spacing;
    node->size.y = static_cast<float>(node_height_tiles);

    float node_fill_mode    = style->fill_modulate_colour ? 2.0f : 0.0f;
    float node_outline_mode = style->outline_style == HIDDEN ? 0.0f : 1.0f;
    if (node->highlighted) node_outline_mode = 2.0f;
    const glm::vec2 box_width       = glm::vec2{ node_width_tiles, 0 } * style->grid_size;
    const glm::vec2 half_tile_width = glm::vec2{ style->grid_size * 0.5f, 0 };
    const glm::vec2 pin_offset      = glm::vec2{ style->pin_offset, 0 };
    glm::vec2 header_position = style->header_at_top ? 
        position :
        position + glm::vec2{ 0, node_height_tiles * style->grid_size };

    if (pass == 0)
    {
        // heading box
        addQuad(header_position, glm::vec2{ node_width_tiles, 1 } * style->grid_size, { 0, 0 },
            { 1, 1 }, node->colour, RENDER_MODE_BOX,
            { node_outline_mode, style->header_fill ? 1.0f : node_fill_mode,
                encodeFourBits(true, node->minimised, true, true) });
        if (style->header_align > 0) header_position = header_position + box_width;
        else if (style->header_align == 0)
            header_position = header_position + (box_width * 0.5f);
        addText(node->title, header_position, style->text_colour, style->header_align);

        // minimise button
        glm::vec2 seg_size = glm::vec2(style->ui_atlas->getSize()) / 4.0f;
        glm::vec2 uv_size  = glm::vec2(1.0f / 4.0f);
        int texture_part   = node->minimised ? 1 : 0;
        glm::vec2 uv_base =
            glm::vec2{ uv_size.x * (texture_part % 4), uv_size.y * (texture_part / 4) };
        addQuad(header_position + glm::round((style->grid_size - seg_size) * 0.5f) +
                    (glm::vec2{ node_width_tiles - 1, 0 } * style->grid_size),
            seg_size, flipUV(uv_base), flipUV(uv_base + uv_size), style->outline_colour,
            RENDER_MODE_UI, style->fill_colour);

        if (node->minimised) return;

        // TODO: line between header and main area

        // main box
        glm::vec2 box_position = position + glm::vec2{ 0, style->grid_size };
        {
            if (!style->header_at_top) { box_position -= glm::vec2{ 0, style->grid_size }; }
            addQuad(box_position, glm::vec2{ node_width_tiles, node_height_tiles } * style->grid_size,
                { 0, 0 }, { 1, 1 }, node->colour, RENDER_MODE_BOX,
                { node_outline_mode, node_fill_mode, encodeFourBits(false, true, true, true) });
        }

        // elements
        glm::vec2 elem_position = box_position;
        if (style->header_at_top)
            elem_position += glm::vec2{ 0, style->after_header_spacing * style->grid_size };
        else
            elem_position += glm::vec2{ 0, style->after_elements_spacing * style->grid_size };
        auto it     = node->elements.begin();
        auto end_it = node->elements.end() - 1;
        if (style->reverse_element_order)
        {
            it     = node->elements.end() - 1;
            end_it = node->elements.begin();
        }
        while (!node->elements.empty())
        {
            const auto& elem = *it;
            switch (elem.type)
            {
            case ELEMENT_INPUT:
                addText(elem.text, elem_position + half_tile_width, style->text_colour);
                addPin(elem_position + pin_offset - half_tile_width, style->outline_colour,
                    elem.pin_type, elem.pin_solid);
                break;
            case ELEMENT_OUTPUT:
                addText(elem.text, elem_position + box_width - half_tile_width, style->text_colour, 1);
                addPin(elem_position + box_width - (half_tile_width + pin_offset),
                    style->outline_colour, elem.pin_type, elem.pin_solid);
                break;
            case ELEMENT_TEXT:
                if (style->center_text_elements)
                    addText(elem.text, elem_position + (box_width * 0.5f), style->text_colour, 0);
                else
                    addText(elem.text, elem_position, style->text_colour);
                break;
            case ELEMENT_SPACE: break;
            }
            if (it == end_it) break;
            elem_position += glm::vec2{ 0, style->grid_size };
            if (style->reverse_element_order) --it;
            else
                ++it;
        }
    }

    // shadows
    if (pass == 1)
    {
        if (style->shadows)
        {
            addQuad(position + style->shadow_offset,
                glm::vec2{ node_width_tiles, 1 + (node->minimised ? 0 : node_height_tiles) } *
                    style->grid_size,
                glm::vec2{ 0, 0 }, glm::vec2{ 1, 1 }, style->shadow_colour, RENDER_MODE_BOX,
                { 0, 1, 15.0f });
        }
    }

    // links
    if (pass == 2)
    {
        int out_rows_down = -1;
        for (auto& link : node->incoming_links)
        {
            ++out_rows_down;
            while (static_cast<size_t>(out_rows_down) < node->elements.size() &&
                    node->elements[out_rows_down].type != ELEMENT_INPUT)
                ++out_rows_down;
            if (static_cast<size_t>(out_rows_down) >= node->elements.size()) break;
            if (!link.first) continue;
            auto target = link.first;

            int in_rows_down   = 0;
            int in_inputs_seen = 0;
            while (static_cast<size_t>(in_rows_down) < target->elements.size() &&
                    !(static_cast<size_t>(in_inputs_seen) == link.second &&
                        target->elements[in_rows_down].type == ELEMENT_OUTPUT))
            {
                if (target->elements[in_rows_down].type == ELEMENT_OUTPUT) ++in_inputs_seen;
                ++in_rows_down;
            }
            if (static_cast<size_t>(in_rows_down) >= target->elements.size()) break;

            glm::vec2 link_start = node->minimised ? 
                header_position + glm::vec2{ 0, 0.5f * style->grid_size } :
                header_position + glm::vec2{ 0, ((float)out_rows_down + 1.5f) * style->grid_size };
            glm::vec2 link_end = target->minimised ?
                (target->position + glm::vec2{ target->size.x, 0.5f }) * style->grid_size :
                (target->position + glm::vec2{ target->size.x, (float)in_rows_down + 1.5f }) * style->grid_size;
            addLink(link_start, link_end, node->minimised || target->minimised);
        }
    }

    // TODO: come up with a list of input types
}

WeakRef<NodeView::Node> NodeView::makeNode(const string& title, glm::vec3 colour, int tiles_wide)
{
    Ref<Node> node = new Node();
    node->title = title;
    node->colour = colour;
    node->self = node;
    node->size.x = static_cast<float>(tiles_wide);
    nodes.push_back(node);
    return node;
}

void NodeView::makeLink(WeakRef<Node> sender_node, size_t output_index, WeakRef<Node> receiver_node, size_t input_index)
{
    receiver_node->incoming_links.resize(glm::max(input_index + 1, receiver_node->incoming_links.size()));
    receiver_node->incoming_links[input_index] = { sender_node, output_index };
}

NodeView::NodeView() {}

NodeView::~NodeView()
{
    style->font       = nullptr;
    style->node_atlas = nullptr;
    material          = nullptr;
    nodes.clear();
}

vector<DrawCommand> NodeView::getDrawCommands()
{
    auto arr = StaticMeshComponent::getDrawCommands();
    // if (context_menu)
    //     arr.push_back(context_menu->draw());
    return arr;
}
