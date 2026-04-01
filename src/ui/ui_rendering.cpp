#include "user_interface.h"

#include "engine.h"
#include "input.h"

using namespace HopEngine;

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
    // if (align > 0) start.x -= width;
    // else if (align == 0)
    //     start.x -= glm::round(width / 2.0f);

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
    std::vector<uint16_t> final_indices;
    size_t total_indices = 0;
    for (const auto& arr : indices)
        total_indices += arr.second.size();
    final_indices.resize(total_indices);
    size_t offset = 0;
    for (const auto& arr : indices)
    {
        memcpy(final_indices.data() + offset, arr.second.data(), arr.second.size() * sizeof(uint16_t));
        offset +=  arr.second.size();
    }
    if (!mesh)
        mesh = new Mesh(vertices, final_indices, true);
    else
        mesh->updateData(vertices, final_indices, ((vertices.size() / 256) + 1) * 256, ((final_indices.size() / 256) + 1) * 256 );
}
