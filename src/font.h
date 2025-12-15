#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "common.h"

namespace HopEngine
{

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

	inline glm::vec2 getCharacterSize() const { return character_size; }
	Ref<Texture> getAtlas() const;
	glm::vec2 getCharUVOffset(char c) const;
	inline glm::vec2 getCharUVSize() const { return char_uv_size; }
};

}
