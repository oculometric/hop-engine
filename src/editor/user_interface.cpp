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

void UIRenderer::addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4,
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
        { p4.x, -p4.y, 0, 1 },
        colour, normal, tangent, uv_br
    });

    indices.push_back(v_off + 0);
    indices.push_back(v_off + 3);
    indices.push_back(v_off + 1);
    indices.push_back(v_off + 0);
    indices.push_back(v_off + 2);
    indices.push_back(v_off + 3);
}

void UIRenderer::addText(const string& text, glm::vec2 position, glm::vec3 colour, int align)
{
    const glm::vec2 uv_size   = style->font->getGlyphUVSize();
    const glm::vec2 char_size = style->font->getGlyphSize();

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

        addQuad(top_left, top_left + glm::vec2{ char_size.x, 0 },
            top_left + glm::vec2{ 0, char_size.y }, top_left + char_size,
            uv_tl, uv_br, glm::vec4{ colour, 1 }, glm::vec4{ 0, 0, 0, 0 }, glm::vec4{ 0, 0, 0, 0 });

        top_left.x += style->font->getGlyphSize().x - 1.0f;
    }
}

void UIRenderer::addNineSlice(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 fill)
{
    addQuad(position, position + glm::vec2{ size.x, 0 },
            position + glm::vec2{ 0, size.y }, position + size,
            { 0, 0 }, { 1, 1 }, glm::vec4{ fill, 1 }, glm::vec4{ 1, layer, 0b1111, 0 }, glm::vec4{ size, 0, 0 });
}

void UIRenderer::addSimple(glm::vec2 position, glm::vec2 size, glm::vec2 uv_base, glm::vec2 uv_size, int layer)
{
    addQuad(position, position + glm::vec2{ size.x, 0 },
            position + glm::vec2{ 0, size.y }, position + size,
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
    renderer->addText(text, top_corner + glm::vec2{ 5, (element_index * 32) + 5 }, glm::vec3{ 0, 0, 0 }, -1);
    ++element_index;
}

void UIContextMenu::addButton(const string& text, function<void()> callback)
{
    renderer->addNineSlice(top_corner + glm::vec2{ 3, (element_index * 32) + 3 }, glm::vec2{ 100 - 6, 32 - 6 }, 0, glm::vec3{ 0.9f, 0.9f, 0.9f });
    renderer->addText(text, top_corner + glm::vec2{ 5, (element_index * 32) + 5 }, glm::vec3{ 0, 0, 0 }, -1);
    ++element_index;
}

void UIContextMenu::done()
{
    renderer->finalise();
    renderer->clear();
}

bool UIContextMenu::checkInput()
{
    glm::vec2 bounds_min = top_corner;
    glm::vec2 bounds_max = top_corner + glm::vec2{ 100, (element_index * 32) };
    glm::vec2 mouse_pos = Input::getMousePosition();

    if (mouse_pos.x < bounds_min.x || mouse_pos.x > bounds_max.x
     || mouse_pos.y < bounds_min.y || mouse_pos.y > bounds_max.y)
        return false;

    return true;
}
