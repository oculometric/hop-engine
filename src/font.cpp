#include "font.h"

#include <glm/glm.hpp>

#include "texture.h"

using namespace HopEngine;
using namespace std;

Font::Font(const string& atlas_name, const glm::ivec2 glyph_size_pixels)
{
    atlas = Texture::loadImage(atlas_name);
    glyph_size = glyph_size_pixels;
    // compute size constants
    chars_resolution = glm::ivec2(atlas->getSize()) / glyph_size;
    char_uv_size = 1.0f / glm::vec2(chars_resolution);

    DBG_VERBOSE("created font using " + atlas_name + " atlas with character size " + ::to_string(glyph_size.x) + "x" + ::to_string(glyph_size.y));
}

Font::~Font()
{
    DBG_BABBLE("destroying font " + PTR(this));
    atlas = nullptr;
}

Ref<Texture> Font::getAtlas() const
{
    return atlas;
}

glm::vec2 Font::getGlyphUVOffset(const char c) const
{
    return { 
        glm::fract(static_cast<float>(c) / static_cast<float>(chars_resolution.x)), 
        glm::floor(static_cast<float>(c) / static_cast<float>(chars_resolution.x)) / static_cast<float>(chars_resolution.y) };
}
