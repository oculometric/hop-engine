#include "font.h"

#include <glm/glm.hpp>

#include "texture.h"

using namespace HopEngine;
using namespace std;

Font::Font(string atlas_name, glm::ivec2 character_texture_size)
{
    atlas = new Texture(atlas_name);
    character_size = character_texture_size;
    chars_resolution = atlas->getSize() / character_size;
    char_uv_size = 1.0f / glm::vec2(chars_resolution);

    DBG_INFO("created font using " + atlas_name + " atlas with character size " + to_string(character_size.x) + "x" + to_string(character_size.y));
}

Font::~Font()
{
    DBG_INFO("destroying font " + PTR(this));
    atlas = nullptr;
}

Ref<Texture> Font::getAtlas()
{
    return atlas;
}

glm::vec2 Font::getCharUVOffset(char c)
{
    return { glm::fract(c / (float)chars_resolution.x), glm::floor(c / (float)chars_resolution.x) / (float)chars_resolution.y };
}
