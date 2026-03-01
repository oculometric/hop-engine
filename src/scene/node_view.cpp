#include "node_view.h"

#include <glm/gtc/integer.hpp>

#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

constexpr size_t v_i_buffer_rounding_size = 256;
constexpr float RENDER_MODE_BOX = 0.0f;
constexpr float RENDER_MODE_TEXT = 1.0f;
constexpr float RENDER_MODE_PINS = 2.0f;
constexpr float RENDER_MODE_BACKGROUND = 3.0f;
constexpr float RENDER_MODE_UI = 4.0f;

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

void NodeView::setStyle(Ref<Style> new_style)
{
    material->setTexture("text_atlas", new_style->font->getAtlas());
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

    if (getScene())
        getScene()->getCamera(0)->clear_colour = style->background_colour;

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
    
    if (style->show_grid)
    {
        const glm::vec2 half_viewport = glm::vec2{ 1000000.0f };
        addQuad(-half_viewport, half_viewport * 2.0f, { 0, 0 }, { 1, 1 }, style->grid_colour, RENDER_MODE_BACKGROUND);
    }
    
    // draw nodes
    for (auto& node : nodes)
    {
        const glm::vec2 position = node->position * style->grid_size;
        int node_width_tiles = static_cast<int>(node->size.x);
        int node_height_tiles = static_cast<int>(node->elements.size()) + style->after_header_spacing + style->after_elements_spacing;
        node->size.y = static_cast<float>(node_height_tiles);

        float node_fill_mode = style->fill_modulate_colour ? 2.0f : 0.0f;
        float node_outline_mode = style->outline_style == HIDDEN ? 0.0f : 1.0f;
        if (node->highlighted)
            node_outline_mode = 2.0f;
        const glm::vec2 box_width = glm::vec2{ node_width_tiles, 0 } * style->grid_size;
        const glm::vec2 half_tile_width = glm::vec2{ style->grid_size * 0.5f, 0 };
        const glm::vec2 pin_offset = glm::vec2{ style->pin_offset, 0 };

        // shadows
        if (style->shadows)
        {
            addQuad(position + style->shadow_offset, glm::vec2{ node_width_tiles, 1 + (node->minimised ? 0 : node_height_tiles) } * style->grid_size,
                glm::vec2{ 0, 0 }, glm::vec2{ 1, 1 },
                style->shadow_colour, RENDER_MODE_BOX, { 0, 1, 0 });
        //    float shadow_width_extra = (style->grid_size * 0.25f);
        //    addQuad(position + (glm::vec2{ 0,  } * style->grid_size) + glm::vec2{ style->shadow_offset.x, -shadow_width_extra }, glm::vec2{ node_width_tiles * style->grid_size, style->shadow_offset.y + shadow_width_extra },
        //        glm::vec2{ 0, 0.5f }, glm::vec2{ 1, 1 }, style->shadow_colour, RENDER_MODE_BOX, { 0, 1, 0 }, glm::vec2{ node_width_tiles * style->grid_size, (style->shadow_offset.y + shadow_width_extra) * 2.0f });
        //    addQuad(position + (glm::vec2{ node_width_tiles, 0 } * style->grid_size) + glm::vec2{ -shadow_width_extra, style->shadow_offset.y }, glm::vec2{ style->shadow_offset.x + shadow_width_extra, (1 + (node->minimised ? 0 : node_height_tiles)) * style->grid_size },
        //        glm::vec2{ 0.5f, 0 }, glm::vec2{ 1, 1 }, style->shadow_colour, RENDER_MODE_BOX, { 0, 1, 0 }, glm::vec2{ (style->shadow_offset.x + shadow_width_extra) * 2.0f, (1 + (node->minimised ? 0 : node_height_tiles)) * style->grid_size });
        }

        // heading box
        glm::vec2 header_position = position;
        {
            float header_uv_top = 0.0f;
            float header_uv_bottom = 0.5f;
            if (!style->header_at_top)
            {
                header_position = header_position + glm::vec2{ 0, node_height_tiles * style->grid_size };
                header_uv_top = 0.5f;
                header_uv_bottom = 1.0f;
            }
            if (node->minimised)
            {
                header_uv_top = 0.0f;
                header_uv_bottom = 1.0f;
            }
            addQuad(header_position, glm::vec2{ node_width_tiles, 1 } * style->grid_size,
                { 0, header_uv_top }, { 1, header_uv_bottom },
                node->colour, RENDER_MODE_BOX,
                { node_outline_mode, style->header_fill ? 1.0f : node_fill_mode, 0.0f },
                glm::vec2{ node_width_tiles, node->minimised ? 1.0f : 2.0f } * style->grid_size);
            if (style->header_align > 0) header_position = header_position + box_width;
            else if (style->header_align == 0) header_position = header_position + (box_width * 0.5f);
            addText(node->title, header_position, style->text_colour, style->header_align);

            // minimise button
            glm::vec2 seg_size = glm::vec2(style->ui_atlas->getSize()) / 4.0f;
            glm::vec2 uv_size = glm::vec2(1.0f / 4.0f);
            glm::vec2 uv_base = glm::vec2{ uv_size.x * (0 % 4), uv_size.y * (0 / 4) };
            addQuad(header_position + glm::round((style->grid_size - seg_size) * 0.5f) + (glm::vec2{ node_width_tiles - 1, 0 } * style->grid_size), seg_size,
                flipUV(uv_base), flipUV(uv_base + uv_size), style->outline_colour, RENDER_MODE_UI, style->fill_colour);

            if (node->minimised)
                continue;
        }

        // TODO: line between header and main area
        
        // main box
        glm::vec2 box_position = position + glm::vec2{ 0, style->grid_size };
        {
            float box_uv_top = 0.5f;
            float box_uv_bottom = 1.0f;
            if (!style->header_at_top)
            {
                box_position -= glm::vec2{ 0, style->grid_size };
                box_uv_top = 0.0f;
                box_uv_bottom = 0.5f;
            }
            addQuad(box_position, glm::vec2{ node_width_tiles, node_height_tiles } * style->grid_size,
                { 0, box_uv_top }, { 1, box_uv_bottom },
                node->colour, RENDER_MODE_BOX,
                { node_outline_mode, node_fill_mode, 0.0f },
                glm::vec2{ node_width_tiles, node_height_tiles * 2.0f } * style->grid_size);
        }

        // elements
        glm::vec2 elem_position = box_position;
        if (style->header_at_top)
            elem_position += glm::vec2{ 0, style->after_header_spacing * style->grid_size };
        else
            elem_position += glm::vec2{ 0, style->after_elements_spacing * style->grid_size };
        auto it = node->elements.begin();
        auto end_it = node->elements.end() - 1;
        if (style->reverse_element_order)
        {
            it = node->elements.end() - 1;
            end_it = node->elements.begin();
        }
        while (!node->elements.empty())
        {
            const auto& elem = *it;
            switch (elem.type)
            {
            case ELEMENT_INPUT:
                addText(elem.text, elem_position + half_tile_width, style->text_colour);
                addPin(elem_position + pin_offset - half_tile_width, style->outline_colour, elem.pin_type, elem.pin_solid);
                break;
            case ELEMENT_OUTPUT:
                addText(elem.text, elem_position + box_width - half_tile_width, style->text_colour, 1);
                addPin(elem_position + box_width - (half_tile_width + pin_offset), style->outline_colour, elem.pin_type, elem.pin_solid);
                break;
            case ELEMENT_TEXT:
                if (style->center_text_elements)
                    addText(elem.text, elem_position + (box_width * 0.5f), style->text_colour, 0);
                else
                    addText(elem.text, elem_position, style->text_colour);
                break;
            }
            if (it == end_it)
                break;
            elem_position += glm::vec2{ 0, style->grid_size };
            if (style->reverse_element_order)
                --it;
            else
                ++it;
        }

        // TODO: come up with a list of input types
    }

    size_t vertices_rounded_up = ((vertices.size() / v_i_buffer_rounding_size) + 2) * v_i_buffer_rounding_size;
    size_t indices_rounded_up = ((indices.size() / v_i_buffer_rounding_size) + 2) * v_i_buffer_rounding_size;

    if (mesh)
        mesh->updateData(vertices, indices, vertices_rounded_up, indices_rounded_up);
    else
        mesh = new Mesh(vertices, indices, true);
}

void NodeView::checkInput(glm::ivec2 rect_min, glm::ivec2 rect_size)
{
    static glm::vec2 mouse_delta_since_down = glm::vec2{ 0, 0 };

    glm::vec2 mouse_pos = Input::getMousePosition();
    if (mouse_pos.x < rect_min.x
        || mouse_pos.y < rect_min.y
        || mouse_pos.x > rect_min.x + rect_size.x
        || mouse_pos.y > rect_min.y + rect_size.y)
        return;
    glm::vec2 node_space_pos = (mouse_pos - glm::vec2(rect_min)) - (glm::vec2(rect_size) / 2.0f);

    bool needs_update = false;
    static auto it = nodes.rend();
    if (Input::wasMousePressed(Input::MOUSE_LEFT))
    {
        for (it = nodes.rbegin(); it != nodes.rend(); ++it)
        {
            auto node = *it;
            glm::vec2 node_min = node->position * style->grid_size;
            glm::vec2 node_max = node_min + (node->size * style->grid_size);
            if (node_space_pos.x < node_min.x
                || node_space_pos.y < node_min.y
                || node_space_pos.x > node_max.x
                || node_space_pos.y > node_max.y)
                continue;
            break;
        }
        mouse_delta_since_down = glm::vec2{ 0, 0 };
        if (it == nodes.rend())
        {
            for (auto& n : nodes)
                n->highlighted = false;
            needs_update = true;
        }
    }
    if (Input::isMouseDown(Input::MOUSE_LEFT))
        mouse_delta_since_down += Input::getMouseDelta();

    if (it != nodes.rend() && (length(mouse_delta_since_down) < 2.0f) && !Input::isMouseDown(Input::MOUSE_LEFT))
    {
        // mouse pressed and released quickly
        auto node = *it;

        glm::vec2 minibox_min = (node->position + glm::vec2{ node->size.x - 1, 0 }) * style->grid_size;
        glm::vec2 minibox_max = minibox_min + style->grid_size;
        if (!(node_space_pos.x < minibox_min.x
            || node_space_pos.y < minibox_min.y
            || node_space_pos.x > minibox_max.x
            || node_space_pos.y > minibox_max.y))
        {
            node->minimised = !node->minimised;
            it = nodes.rend();
            needs_update = true;
        }
        else
        {
            if (!Input::isKeyDown(Input::KEY_LEFT_SHIFT))
            {
                for (auto& n : nodes)
                    n->highlighted = false;
                node->highlighted = true;
            }
            else
                node->highlighted = !node->highlighted;
            nodes.erase((it + 1).base());
            nodes.insert(nodes.end(), node);
            it = nodes.rend();
            needs_update = true;
        }
    }
    else if (length(mouse_delta_since_down) > 2.0f && Input::isMouseDown(Input::MOUSE_LEFT))
    {
        // mouse drag is occurring!
        if (it != nodes.rend() && !(*it)->highlighted && !Input::isKeyDown(Input::KEY_LEFT_SHIFT))
        {
            auto node = *it;
            for (auto& n : nodes)
                n->highlighted = false;
            node->highlighted = true;
            nodes.erase((it + 1).base());
            nodes.insert(nodes.end(), node);
            it = nodes.rend();
        }

        for (auto& n : nodes)
            if (n->highlighted)
                n->position += Input::getMouseDelta() * 4.0f / style->grid_size;
        needs_update = true;
    }

    // TODO: links
    // TODO: add new
    // TODO: resize
    if (needs_update)
        updateMesh();
    Input::resetMouseDelta();
    //DBG_INFO("pos: " + ::to_string(node_space_pos.x) + ", " + ::to_string(node_space_pos.y));
}

NodeView::NodeView() : StaticMesh(nullptr, nullptr)
{
    material = new Material(new Shader("res://engine/shaders/node_shader.glsl"), PipelineBuilder().cullMode(CULL_NONE).depthTest(false).depthWrite(false));
    Ref<Sampler> sampler = Engine::makeSampler(SamplerBuilder().filter(FILTER_NEAREST));
    material->setSampler("node_atlas", sampler);
    material->setSampler("text_atlas", sampler);
    material->setSampler("extra_atlas", sampler);
    material->setSampler("ui_atlas", sampler);

    style = new Style();
    style->node_atlas = new Texture("res://engine/textures/node_atlas.png");
    style->extra_atlas = new Texture("res://engine/textures/extra_atlas.png");
    style->ui_atlas = new Texture("res://engine/textures/ui_atlas.png");
    style->font = new Font("res://engine/textures/font_IBM_XGA_AI_12x23.png", glm::ivec2{ 14, 25 });

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
    glm::vec2 seg_size = style->extra_atlas->getSize() / 3;
    glm::vec2 uv_size = glm::vec2{ 1.0f / 3.0f };
    glm::vec2 uv_base = glm::vec2{ uv_size.x * (type % 3), uv_size.y * (type / 3) };
    addQuad(position + glm::round((style->grid_size - seg_size) * 0.5f), seg_size, flipUV(uv_base), flipUV(uv_base + uv_size), tint, RENDER_MODE_PINS, {filled ? 1 : 0, 0, 0});
}

void NodeView::addText(const string& text, glm::vec2 _start, glm::vec3 tint, int align)
{
    const glm::vec2 uv_size = style->font->getGlyphUVSize();
    const glm::vec2 char_size = style->font->getGlyphSize();
    
    glm::vec2 start = _start;
    float width = ((text.size() + 1) * (style->font->getGlyphSize().x + style->text_spacing - 2.0f)) - (style->text_spacing - 2.0f);
    if (align > 0)
        start.x -= width;
    else if (align == 0)
        start.x -= glm::round(width / 2.0f);

    glm::vec2 position = start + style->text_offset + glm::vec2{ 0, (style->grid_size - style->font->getGlyphSize().y) * 0.5f };
    for (char c : text)
    {
        glm::vec2 uv_base = style->font->getGlyphUVOffset(c);

        glm::vec2 uv_br = flipUV(uv_base + uv_size);
        glm::vec2 uv_tl = flipUV(uv_base);
        glm::vec4 pos_tl = { position.x, position.y, 0, 1 };

        addQuad(pos_tl, char_size, uv_tl, uv_br, tint, RENDER_MODE_TEXT);
        
        position.x += style->font->getGlyphSize().x + style->text_spacing - 2.0f;
    }
}

NodeView::~NodeView()
{
    style->font = nullptr;
    style->node_atlas = nullptr;
    material = nullptr;
    nodes.clear();
}
