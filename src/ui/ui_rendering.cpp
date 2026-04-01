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

UIRenderer::BackingData UIRenderer::addQuad(float z)
{
    if (!indices.contains(z))
        indices[z] = {};

    auto& _indices = indices[z];

    BackingData backing;
    backing.first_vertex = static_cast<uint16_t>(vertices.size());
    backing.vertex_count = 4;
    backing.first_index = static_cast<uint16_t>(_indices.size());
    backing.index_count = 6;
    backing.z = z;

    vertices.push_back({});
    vertices.push_back({});
    vertices.push_back({});
    vertices.push_back({});

    _indices.push_back(0);
    _indices.push_back(0);
    _indices.push_back(0);
    _indices.push_back(0);
    _indices.push_back(0);
    _indices.push_back(0);

    return backing;
}

UIRenderer::BackingData UIRenderer::addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float z,
    glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent)
{
    BackingData backing = addQuad(z);
    updateQuad(p1, p2, p3, p4, uv_tl, uv_br, colour, normal, tangent, backing);
    return backing;
}

UIRenderer::BackingData UIRenderer::addText(glm::vec2 position, float z, TextFormatting formatting, const std::string& text, glm::vec3 colour)
{
    BackingData backing = addQuad(z);
    for (size_t i = 0; i < text.size(); ++i)
    {
        BackingData temp = addQuad(z);
        backing.vertex_count += temp.vertex_count;
        backing.index_count += temp.index_count;
    }
    updateText(position, formatting, text, colour, backing);
    return backing;
}

UIRenderer::BackingData UIRenderer::addNineSlice(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec3 fill)
{
    BackingData backing = addQuad(z);
    updateNineSlice(position, size, layer, fill, backing);
    return backing;
}

UIRenderer::BackingData UIRenderer::addSimple(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec2 uv_base, glm::vec2 uv_size)
{
    BackingData backing = addQuad(z);
    updateSimple(position, size, layer, uv_base, uv_size, backing);
    return backing;
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

void UIRenderer::updateQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4,
    glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent,
    BackingData backing)
{
    // top left
    vertices[backing.first_vertex + 0] = Mesh::Vertex{
        { p1.x, p1.y, 0, 1 },
        colour, normal, tangent, uv_tl
    };
    // top right
    vertices[backing.first_vertex + 1] = Mesh::Vertex{
        { p2.x, p2.y, 0, 1 },
        colour, normal, tangent,
        glm::vec2{ uv_br.x, uv_tl.y }
    };
    // bottom left
    vertices[backing.first_vertex + 2] = Mesh::Vertex{
        { p3.x, p3.y, 0, 1 },
        colour, normal, tangent,
        glm::vec2{ uv_tl.x, uv_br.y }
    };
    // bottom right
    vertices[backing.first_vertex + 3] = Mesh::Vertex{
        { p4.x, p4.y, 0, 1 },
        colour, normal, tangent, uv_br
    };

    auto& _indices = indices[backing.z];
    
    _indices[backing.first_index + 0] = (backing.first_vertex + 0);
    _indices[backing.first_index + 1] = (backing.first_vertex + 3);
    _indices[backing.first_index + 2] = (backing.first_vertex + 1);
    _indices[backing.first_index + 3] = (backing.first_vertex + 0);
    _indices[backing.first_index + 4] = (backing.first_vertex + 2);
    _indices[backing.first_index + 5] = (backing.first_vertex + 3);
}

float calculateTextWidth(const std::string& text, UIRenderer::TextFormatting formatting, WeakRef<Font> font)
{
    return static_cast<float>((static_cast<int>(text.size()) * (static_cast<int>(font->getGlyphSize().x) + formatting.spacing)) - formatting.spacing);
}

bool UIRenderer::updateText(glm::vec2 position, TextFormatting formatting,
    const std::string& text, glm::vec3 colour, BackingData backing)
{
    const glm::vec2 char_size = style->font->getGlyphSize();
    size_t allocated_chars = backing.vertex_count / 4;

    std::vector<std::string> lines;

    if (!formatting.wrap || (formatting.wrap && formatting.clip_bounds.x <= 0))
    {
        if (formatting.terminate_at_newline)
            lines.push_back(text.substr(0, text.find('\n')));
        else
        {
            size_t newline = 0;
            size_t next = 0;
            do
            {
                next = text.find('\n', newline);
                lines.push_back(text.substr(newline, (next - newline)));
                newline = next + 1;
            }
            while (next != std::string::npos);
        }
    }
    else
    {
        const size_t chars_wide = static_cast<size_t>(glm::floor(static_cast<float>(formatting.clip_bounds.x) / char_size.x));
        size_t base = 0;
        while (base < text.size())
        {
            size_t split = std::min(base + chars_wide, text.size());
            size_t new_base;
            size_t newline = text.find('\n', base);
            if (newline <= split)
            {
                split = newline;
                if (newline == split)
                    new_base = split + 1;
                else
                    new_base = split;
            }
            else
            {
                while (split >= base)
                {
                    if (split < text.size() && (text[split] == ' ' || text[split] == '\n'))
                    {
                        new_base = split + 1;
                        break;
                    }
                    --split;
                    if (split == base)
                    {
                        split = base + chars_wide;
                        new_base = split;
                        break;
                    }
                }
            }

            lines.push_back(text.substr(base, split - base));
            base = new_base;
            if (split < text.size() && text[split] == '\n' && formatting.terminate_at_newline)
                break;
        }
    }
    
    BackingData temp = backing;
    temp.vertex_count = 4;
    temp.index_count = 6;

    glm::vec2 top_left = position;
    for (const auto& line : lines)
    {
        if (allocated_chars < line.size())
        {
            updateTextSingleLine(top_left, formatting, line.substr(0, allocated_chars), colour, temp);
            break;
        }
        updateTextSingleLine(top_left, formatting, line, colour, temp);
        allocated_chars -= line.size();

        top_left.y += char_size.y - 2.0f;
        temp.first_vertex += 4 * static_cast<uint16_t>(line.size());
        temp.first_index += 6 * static_cast<uint16_t>(line.size());
    }

    return true;
}

void UIRenderer::updateTextSingleLine(glm::vec2 position, TextFormatting formatting,
    const std::string& text, glm::vec3 colour, BackingData backing)
{
    const glm::vec2 uv_size   = style->font->getGlyphUVSize();
    const glm::vec2 char_size = style->font->getGlyphSize();
    const float width = calculateTextWidth(text, formatting, style->font);

    glm::vec2 top_left = position;
    if (formatting.align == TEXT_ALIGN_RIGHT) top_left.x -= width;
    else if (formatting.align == TEXT_ALIGN_CENTER)
        top_left.x -= glm::round(width / 2.0f);

    BackingData temp = backing;
    temp.vertex_count = 4;
    temp.index_count = 6;

    for (char c : text)
    {
        glm::vec2 uv_base = style->font->getGlyphUVOffset(c);

        glm::vec2 uv_br  = uv_base + uv_size;
        uv_br.y = 1.0f - uv_br.y;
        glm::vec2 uv_tl  = uv_base;
        uv_tl.y = 1.0f - uv_tl.y;

        glm::vec2 skew = { 0, 0 };
        if (formatting.flags & UIRenderer::TEXT_FLAGS_ITALIC)
            skew.x = glm::round(char_size.x / 2.0f);

        int flags = 0;
        if (formatting.flags & UIRenderer::TEXT_FLAGS_BOLD)          flags |= 1;
        if (formatting.flags & UIRenderer::TEXT_FLAGS_UNDERLINE)     flags |= 2;
        if (formatting.flags & UIRenderer::TEXT_FLAGS_STRIKETHROUGH) flags |= 4;
        
        // TODO: perform clipping (skip line or truncate quads (recalc UVs!!), horizontal as well as vertical)

        updateQuad(top_left + skew, top_left + glm::vec2{ char_size.x, 0 } + skew,
            top_left + glm::vec2{ 0, char_size.y }, top_left + char_size,
            uv_tl, uv_br, glm::vec4{ colour, 1 }, glm::vec4{ 0, static_cast<float>(flags), 0, 0 }, glm::vec4{ char_size, 0, 0 }, temp);
        
        top_left.x += char_size.x + formatting.spacing;

        temp.first_vertex += 4;
        temp.first_index += 6;
    }
}

void UIRenderer::updateNineSlice(
    glm::vec2 position, glm::vec2 size, int layer, glm::vec3 fill, BackingData backing)
{
    updateQuad(position, position + glm::vec2{ size.x, 0 },
            position + glm::vec2{ 0, size.y }, position + size,
            { 0, 0 }, { 1, 1 },
            glm::vec4{ fill, 1 }, glm::vec4{ 1, layer, 0b1111, 0 }, glm::vec4{ size, 0, 0 }, backing);
}

void UIRenderer::updateSimple(glm::vec2 position, glm::vec2 size, int layer,
    glm::vec2 uv_base, glm::vec2 uv_size, BackingData backing)
{
    updateQuad(position, position + glm::vec2{ size.x, 0 },
            position + glm::vec2{ 0, size.y }, position + size,
            uv_base, uv_base + uv_size, glm::vec4{ 1, 1, 1, 1 }, glm::vec4{ 2, layer, 0, 0 }, glm::vec4{ 0, 0, 0, 0 }, backing);
}
