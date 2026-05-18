#include "engine.h"
#include "texture.h"
#include "user_interface.h"

#include <glm/glm.hpp>

using namespace HopEngine;

Font::Font(const std::string& atlas_name, const glm::ivec2 glyph_size_pixels,
    const std::string& bold_atlas_name)
{
    std::vector<Ref<Texture>> atlases;
    atlases.push_back(Engine::loadTexture(atlas_name));
    if (!bold_atlas_name.empty()) atlases.push_back(Engine::loadTexture(bold_atlas_name));
    createFromAtlases(atlases, glyph_size_pixels);
}

Font::Font(std::vector<Ref<Texture>> atlases, glm::ivec2 glyph_size_pixels)
{ createFromAtlases(atlases, glyph_size_pixels); }

void Font::createFromAtlases(const std::vector<Ref<Texture>>& atlases, glm::ivec2 glyph_size_pixels)
{
    if (atlases.empty())
    {
        DBG_ERROR("cannot create font with no atlases");
        return;
    }
    atlas = atlases[0];
    if (atlases.size() > 1) bold_atlas = atlases[1];

    glyph_size = glyph_size_pixels;
    // compute size constants
    chars_resolution = glm::ivec2(atlas->getSize()) / glyph_size;
    char_uv_size     = 1.0f / glm::vec2(chars_resolution);

    DBG_VERBOSE("created font using '" + atlas->getOrigin() + "' atlas with character size " +
                std::to_string(glyph_size.x) + "x" + std::to_string(glyph_size.y));
}

Font::~Font() {}

glm::vec2 Font::getGlyphUVOffset(const char c) const
{
    return { glm::fract(static_cast<float>(c) / static_cast<float>(chars_resolution.x)),
        glm::floor(static_cast<float>(c) / static_cast<float>(chars_resolution.x)) /
            static_cast<float>(chars_resolution.y) };
}
