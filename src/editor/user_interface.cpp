#include "user_interface.h"

#include "engine.h"
#include "font.h"
#include "input.h"

using namespace HopEngine;
using namespace std;

UIStyle::UIStyle()
{
    shader = Engine::loadShader("res://engine/shaders/user_interface.glsl");
    font = new Font("res://engine/textures/font_IBM_XGA_AI_12x23.png", { 14, 25 });
    ui_atlas = Engine::loadTexture3D("res://engine/textures/ui_kit_98.png", 4, 4);
}

UIStyle::~UIStyle() { }

Ref<Material> UIStyle::makeMaterial()
{
    Ref mat = new Material(shader, Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthTest(false).depthWrite(false));
    mat->setTexture(1, font->getAtlas());
    mat->setTexture(2, ui_atlas);
    mat->setSampler(1, Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_NEAREST)));
    mat->setSampler(2, Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_NEAREST)));
    return mat;
}

UIRenderer::UIRenderer(Ref<UIStyle> _style)
{
    style = _style;
    material = style->makeMaterial();
}

UIRenderer::~UIRenderer() { }

void UIRenderer::clear()
{
    vertices.clear();
    indices.clear();
}

UIRenderer::BackingData UIRenderer::addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float z,
    glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent)
{
    uint16_t v_off = static_cast<uint16_t>(vertices.size());

    // top left
    vertices.push_back(Mesh::Vertex{
        { p1.x, p1.y, 0, 1 },
        colour, normal, tangent, uv_tl
    });
    // top right
    vertices.push_back(Mesh::Vertex{
        { p2.x, p2.y, 0, 1 },
        colour, normal, tangent,
        glm::vec2{ uv_br.x, uv_tl.y }
    });
    // bottom left
    vertices.push_back(Mesh::Vertex{
        { p3.x, p3.y, 0, 1 },
        colour, normal, tangent,
        glm::vec2{ uv_tl.x, uv_br.y }
    });
    // bottom right
    vertices.push_back(Mesh::Vertex{
        { p4.x, p4.y, 0, 1 },
        colour, normal, tangent, uv_br
    });

    if (!indices.contains(z))
        indices[z] = {};

    auto& _indices = indices[z];

    BackingData backing;
    backing.first_vertex = v_off;
    backing.vertex_count = 4;
    backing.first_index = static_cast<uint16_t>(_indices.size());
    backing.index_count = 6;
    backing.z = z;

    _indices.push_back(v_off + 0);
    _indices.push_back(v_off + 3);
    _indices.push_back(v_off + 1);
    _indices.push_back(v_off + 0);
    _indices.push_back(v_off + 2);
    _indices.push_back(v_off + 3);

    return backing;
}

UIRenderer::BackingData UIRenderer::addText(glm::vec2 position, float z, TextFormatting formatting, const std::string& text, glm::vec3 colour)
{
    const glm::vec2 uv_size   = style->font->getGlyphUVSize();
    const glm::vec2 char_size = style->font->getGlyphSize();

    UIRenderer::BackingData backing;
    backing.first_vertex = -1;
    backing.vertex_count = 0;
    backing.first_index = -1;
    backing.index_count = 0;
    backing.z = z;

    // TODO: advanced text with multiline, wrapping, formatting

    glm::vec2 start = position;
    float width = ((text.size() + 1) * (style->font->getGlyphSize().x - 1.0f)) -
                  (-1.0f);
    if (align > 0) start.x -= width;
    else if (align == 0)
        start.x -= glm::round(width / 2.0f);

    glm::vec2 top_left = start;
    for (char c : text)
    {
        glm::vec2 uv_base = style->font->getGlyphUVOffset(c);

        glm::vec2 uv_br  = uv_base + uv_size;
        uv_br.y = 1.0f - uv_br.y;
        glm::vec2 uv_tl  = uv_base;
        uv_tl.y = 1.0f - uv_tl.y;

        auto back = addQuad(top_left, top_left + glm::vec2{ char_size.x, 0 },
            top_left + glm::vec2{ 0, char_size.y }, top_left + char_size, z,
            uv_tl, uv_br, glm::vec4{ colour, 1 }, glm::vec4{ 0, 0, 0, 0 }, glm::vec4{ 0, 0, 0, 0 });
        backing.first_vertex = std::min(backing.first_vertex, back.first_vertex);
        backing.vertex_count += back.vertex_count;
        backing.first_index = std::min(backing.first_index, back.first_index);
        backing.index_count += back.index_count;

        top_left.x += style->font->getGlyphSize().x - 1.0f;
    }

    return backing;
}

UIRenderer::BackingData UIRenderer::addNineSlice(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec3 fill)
{
    return addQuad(position, position + glm::vec2{ size.x, 0 },
            position + glm::vec2{ 0, size.y }, position + size, z,
            { 0, 0 }, { 1, 1 },
            glm::vec4{ fill, 1 }, glm::vec4{ 1, layer, 0b1111, 0 }, glm::vec4{ size, 0, 0 });
}

UIRenderer::BackingData UIRenderer::addSimple(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec2 uv_base, glm::vec2 uv_size)
{
    return addQuad(position, position + glm::vec2{ size.x, 0 },
            position + glm::vec2{ 0, size.y }, position + size, z,
            uv_base, uv_base + uv_size, glm::vec4{ 1, 1, 1, 1 }, glm::vec4{ 2, layer, 0, 0 }, glm::vec4{ 0, 0, 0, 0 });
}

void UIRenderer::finalise()
{
    if (!mesh)
        mesh = new Mesh(vertices, indices, true);
    else
        mesh->updateData(vertices, indices, ((vertices.size() / 256) + 1) * 256, ((indices.size() / 256) + 1) * 256 );
}

UIContextMenu::UIContextMenu(glm::vec2 position)
{
    top_corner = position;
    renderer = new UIRenderer(new UIStyle());
}

void UIContextMenu::addText(const string& text)
{
    elements.emplace_back(text, false, nullptr);
}

void UIContextMenu::addButton(const string& text, function<void()> callback)
{
    elements.emplace_back(text, true, callback);
}

void UIContextMenu::done()
{
    renderer->addNineSlice(top_corner - glm::vec2{ 5, 5 }, glm::vec2{ 220, elements.size() * 32 } + glm::vec2{ 10, 10 }, 1, glm::vec3{ 0.8f, 0.8f, 0.8f });
    size_t element_index = 0;
    for (const auto& elem : elements)
    {
        if (std::get<bool>(elem))
            renderer->addNineSlice(top_corner + glm::vec2{ 0, (element_index * 32) + 0 }, glm::vec2{ 220, 32 }, 0, glm::vec3{ 0.8f, 0.8f, 0.8f });
        renderer->addText(std::get<std::string>(elem), top_corner + glm::vec2{ 5, (element_index * 32) + 5 }, glm::vec3{ 0, 0, 0 }, -1);

        ++element_index;
    }
    renderer->finalise();
    renderer->clear();
}

bool UIContextMenu::checkInput()
{
    glm::vec2 bounds_min = top_corner;
    glm::vec2 bounds_max = top_corner + glm::vec2{ 220, (elements.size() * 32) };
    glm::vec2 mouse_pos = Input::getMousePosition();

    if (mouse_pos.x < bounds_min.x || mouse_pos.x > bounds_max.x
     || mouse_pos.y < bounds_min.y || mouse_pos.y > bounds_max.y)
        return false;

    return true;
}
