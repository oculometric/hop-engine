#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"

namespace HopEngine
{

class Texture;

class Font
{
private:
	Ref<Texture> atlas = nullptr;
	glm::ivec2 character_size;
	glm::ivec2 chars_resolution;
	glm::vec2 char_uv_size;

public:
	DELETE_CONSTRUCTORS(Font);

	Font(std::string atlas, glm::ivec2 character_bitmap_size);
	~Font();

	inline glm::vec2 getCharacterSize() { return character_size; }
	Ref<Texture> getAtlas();
	glm::vec2 getCharUVOffset(char c);
	inline glm::vec2 getCharUVSize() { return char_uv_size; }
};

}
