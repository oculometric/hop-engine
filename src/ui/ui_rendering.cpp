#include "engine.h"
#include "input.h"
#include "material.h"
#include "user_interface.h"

using namespace HopEngine;

UIRenderer::UIRenderer(Ref<UIStyle> _style)
{
    style    = _style;
    material = style->makeMaterial(false);
    next_id  = 123;
}

UIRenderer::~UIRenderer() {}

void UIRenderer::clear()
{
    transform = glm::mat3(1.0);
    vertices.clear();
    indices.clear();
    backing_datas.clear();
}

void UIRenderer::addQuad(float z, BackingData& backing_ref)
{
    if (isBackingValid(backing_ref)) return;

    if (!indices.contains(z)) indices[z] = {};

    auto& _indices = indices[z];

    BackingDataInternal backing;
    backing.first_vertex = static_cast<uint16_t>(vertices.size());
    backing.vertex_count = 4;
    backing.first_index  = static_cast<uint16_t>(_indices.size());
    backing.index_count  = 6;
    backing.z            = z;

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

    addBacking(backing_ref, backing);
}

void UIRenderer::addQuad(float z)
{
    BackingData backing;
    addQuad(z, backing);
}

void UIRenderer::addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float z, glm::vec2 uv_tl,
    glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent, BackingData& backing_ref)
{
    addQuad(z, backing_ref);
    BackingDataInternal backing = backing_datas[backing_ref.id];

    updateQuad(p1, p2, p3, p4, uv_tl, uv_br, colour, normal, tangent, backing);
}

void UIRenderer::addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float z, glm::vec2 uv_tl,
    glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent)
{
    BackingData backing;
    addQuad(p1, p2, p3, p4, z, uv_tl, uv_br, colour, normal, tangent, backing);
}

float calculateTextWidth(const std::string& text, UIRenderer::TextFormatting formatting, WeakRef<Font> font)
{
    return static_cast<float>(
        (static_cast<int>(text.size()) * (static_cast<int>(font->getGlyphSize().x) + formatting.spacing)) -
        formatting.spacing);
}

glm::vec2 UIRenderer::addText(glm::vec2 position, float z, TextFormatting formatting,
    const std::string& text, glm::vec3 colour, BackingData& backing_ref)
{
    BackingDataInternal backing;
    if (!isBackingValid(backing_ref))
    {
        if (!indices.contains(z)) indices[z] = {};

        auto& _indices = indices[z];

        backing.first_vertex = static_cast<uint16_t>(vertices.size());
        backing.vertex_count = static_cast<uint16_t>(4 * text.size());
        backing.first_index  = static_cast<uint16_t>(_indices.size());
        backing.index_count  = static_cast<uint16_t>(6 * text.size());
        backing.z            = z;

        for (size_t i = 0; i < text.size(); ++i)
        {
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
        }

        addBacking(backing_ref, backing);
    }
    else
    {
        backing = backing_datas[backing_ref.id];
    }

    const glm::vec2 char_size = style->font->getGlyphSize();
    size_t allocated_chars    = backing.vertex_count / 4;

    std::vector<std::string> lines;

    if (!formatting.wrap || (formatting.wrap && !formatting.clip))
    {
        if (formatting.terminate_at_newline) lines.push_back(text.substr(0, text.find('\n')));
        else
        {
            size_t newline = 0;
            size_t next    = 0;
            do
            {
                next = text.find('\n', newline);
                lines.push_back(text.substr(newline, (next - newline)));
                newline = next + 1;
            } while (next != std::string::npos);
        }
    }
    else
    {
        const size_t chars_wide =
            static_cast<size_t>(glm::floor(static_cast<float>(formatting.clip_bounds.x) / char_size.x));
        size_t base = 0;
        while (base < text.size())
        {
            size_t split    = std::min(base + chars_wide, text.size());
            size_t new_base = split;
            size_t newline  = text.find('\n', base);
            if (newline <= split)
            {
                split = newline;
                if (newline == split) new_base = split + 1;
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
                        split    = base + chars_wide;
                        new_base = split;
                        break;
                    }
                }
            }

            lines.push_back(text.substr(base, split - base));
            base = new_base;
            if (split < text.size() && text[split] == '\n' && formatting.terminate_at_newline) break;
        }
    }

    BackingDataInternal temp = backing;
    temp.vertex_count        = 4;
    temp.index_count         = 6;

    glm::vec2 top_left = position;
    int bottom_clip    = formatting.clip_bounds.y;
    if (bottom_clip <= 0 || !formatting.clip) bottom_clip = INT_MAX;
    for (const auto& line : lines)
    {
        TextFormatting sub_format = formatting;
        sub_format.clip_bounds.y  = bottom_clip;
        if (allocated_chars < line.size())
        {
            updateTextSingleLine(top_left, sub_format, line.substr(0, allocated_chars), colour, temp);
            break;
        }
        updateTextSingleLine(top_left, sub_format, line, colour, temp);
        allocated_chars -= line.size();

        top_left.y += char_size.y;
        bottom_clip -= static_cast<int>(char_size.y);
        temp.first_vertex += 4 * static_cast<uint16_t>(line.size());
        temp.first_index += 6 * static_cast<uint16_t>(line.size());
    }

    float longest = 0;
    for (const auto& line : lines)
    {
        float length = calculateTextWidth(line, formatting, style->font);
        if (length > longest) longest = length;
    }

    return char_size * glm::vec2{ longest, static_cast<float>(lines.size()) };
}

glm::vec2 UIRenderer::addText(glm::vec2 position, float z, TextFormatting formatting,
    const std::string& text, glm::vec3 colour)
{
    BackingData backing;
    return addText(position, z, formatting, text, colour, backing);
}

void UIRenderer::addNineSlice(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec3 fill,
    BackingData& backing_ref)
{
    addQuad(position, position + glm::vec2{ size.x, 0 }, position + glm::vec2{ 0, size.y }, position + size,
        z, { 0, 0 }, { 1, 1 }, glm::vec4{ fill, 1 }, glm::vec4{ 1, layer, 0b1111, 0 },
        glm::vec4{ size, 0, 0 }, backing_ref);
}

void UIRenderer::addNineSlice(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec3 fill)
{
    BackingData backing;
    addNineSlice(position, z, size, layer, fill, backing);
}

void UIRenderer::addSimple(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec2 uv_base,
    glm::vec2 uv_size, BackingData& backing_ref)
{
    addQuad(position, position + glm::vec2{ size.x, 0 }, position + glm::vec2{ 0, size.y }, position + size,
        z, uv_base, uv_base + uv_size, glm::vec4{ 1, 1, 1, 1 }, glm::vec4{ 2, layer, 0, 0 },
        glm::vec4{ 0, 0, 0, 0 }, backing_ref);
}

void UIRenderer::addSimple(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec2 uv_base,
    glm::vec2 uv_size)
{
    BackingData backing;
    addSimple(position, z, size, layer, uv_base, uv_size, backing);
}

void UIRenderer::finalise()
{
    std::vector<uint16_t> final_indices;
    size_t total_indices = 0;
    for (const auto& arr : indices) total_indices += arr.second.size();
    final_indices.resize(total_indices);
    size_t offset = 0;
    for (const auto& arr : indices)
    {
        memcpy(final_indices.data() + offset, arr.second.data(), arr.second.size() * sizeof(uint16_t));
        offset += arr.second.size();
    }
    if (!mesh) mesh = new Mesh(vertices, final_indices, true);
    else
        mesh->updateData(vertices, final_indices, ((vertices.size() / 256) + 1) * 256,
            ((final_indices.size() / 256) + 1) * 256);
}

void UIRenderer::setWorldSpace(bool world_space) { material = style->makeMaterial(world_space); }

bool UIRenderer::isBackingValid(const BackingData& backing_ref)
{ return backing_datas.contains(backing_ref.id); }

void UIRenderer::addBacking(BackingData& backing_ref, BackingDataInternal backing)
{
    backing_datas[next_id] = backing;
    backing_ref.id         = next_id;
    ++next_id;
    if (next_id == 0) next_id = 4;
}

void UIRenderer::updateTextSingleLine(glm::vec2 position, TextFormatting formatting,
    const std::string& text, glm::vec3 colour, BackingDataInternal backing)
{
    const glm::vec2 uv_size   = style->font->getGlyphUVSize();
    const glm::vec2 char_size = style->font->getGlyphSize();
    const float width         = calculateTextWidth(text, formatting, style->font);

    glm::vec2 top_left = position;
    if (formatting.align == TEXT_ALIGN_RIGHT) top_left.x -= width;
    else if (formatting.align == TEXT_ALIGN_CENTER)
        top_left.x -= glm::round(width / 2.0f);

    BackingDataInternal temp = backing;
    temp.vertex_count        = 4;
    temp.index_count         = 6;

    for (char c : text)
    {
        glm::vec2 uv_base = style->font->getGlyphUVOffset(c);

        glm::vec2 uv_br = uv_base + uv_size;
        uv_br.y         = 1.0f - uv_br.y;
        glm::vec2 uv_tl = uv_base;
        uv_tl.y         = 1.0f - uv_tl.y;

        glm::vec2 skew = { 0, 0 };
        if (formatting.flags & UIRenderer::TEXT_FLAGS_ITALIC) skew.x = glm::round(char_size.x / 2.0f);

        int flags = 0;
        if (formatting.flags & UIRenderer::TEXT_FLAGS_BOLD) flags |= 1;
        if (formatting.flags & UIRenderer::TEXT_FLAGS_UNDERLINE) flags |= 2;
        if (formatting.flags & UIRenderer::TEXT_FLAGS_STRIKETHROUGH) flags |= 4;

        glm::vec2 tl = top_left + skew;
        glm::vec2 tr = top_left + glm::vec2{ char_size.x, 0 } + skew;
        glm::vec2 bl = top_left + glm::vec2{ 0, char_size.y };
        glm::vec2 br = top_left + char_size;

        if (char_size.y > formatting.clip_bounds.y)
        {
            float subtract_amount_px = glm::min(char_size.y, char_size.y - formatting.clip_bounds.y);
            float subtract_amount_uv = (uv_size.y / char_size.y) * subtract_amount_px;
            bl.y -= subtract_amount_px;
            br.y -= subtract_amount_px;
            uv_br.y += subtract_amount_uv;
        }

        updateQuad(tl, tr, bl, br, uv_tl, uv_br, glm::vec4{ colour, 1 },
            glm::vec4{ 0, static_cast<float>(flags), 0, 0 }, glm::vec4{ char_size, 0, 0 }, temp);

        top_left.x += char_size.x + formatting.spacing;

        temp.first_vertex += 4;
        temp.first_index += 6;
    }
}

void UIRenderer::updateQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2 uv_tl,
    glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent, BackingDataInternal backing)
{
    glm::vec3 _p1 = transform * glm::vec3{ p1, 1 };
    glm::vec3 _p2 = transform * glm::vec3{ p2, 1 };
    glm::vec3 _p3 = transform * glm::vec3{ p3, 1 };
    glm::vec3 _p4 = transform * glm::vec3{ p4, 1 };

    // top left
    vertices[backing.first_vertex + 0] = Mesh::Vertex{
        { _p1.x, _p1.y, 0, 1 },
        colour, normal, tangent, { uv_tl, 0 }
    };
    // top right
    vertices[backing.first_vertex + 1] = Mesh::Vertex{
        { _p2.x, _p2.y, 0, 1 },
        colour, normal, tangent, { uv_br.x, uv_tl.y, 0 }
    };
    // bottom left
    vertices[backing.first_vertex + 2] = Mesh::Vertex{
        { _p3.x, _p3.y, 0, 1 },
        colour, normal, tangent, { uv_tl.x, uv_br.y, 0 }
    };
    // bottom right
    vertices[backing.first_vertex + 3] = Mesh::Vertex{
        { _p4.x, _p4.y, 0, 1 },
        colour, normal, tangent, { uv_br, 0 }
    };

    auto& _indices = indices[backing.z];

    _indices[backing.first_index + 0] = (backing.first_vertex + 0);
    _indices[backing.first_index + 1] = (backing.first_vertex + 3);
    _indices[backing.first_index + 2] = (backing.first_vertex + 1);
    _indices[backing.first_index + 3] = (backing.first_vertex + 0);
    _indices[backing.first_index + 4] = (backing.first_vertex + 2);
    _indices[backing.first_index + 5] = (backing.first_vertex + 3);
}
