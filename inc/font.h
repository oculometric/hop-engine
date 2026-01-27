#pragma once

#include <glm/vec2.hpp>

#include "common.h"

namespace HopEngine
{

class Font : public Destructible
{
private:
	Ref<Texture> atlas = nullptr;
	glm::ivec2 glyph_size;
	glm::ivec2 chars_resolution;
	glm::vec2 char_uv_size;

public:
	DELETE_CONSTRUCTORS(Font);
	Font(const std::string& atlas_name, glm::ivec2 glyph_size_pixels);
	~Font() override;
	
	Ref<Texture> getAtlas() const;
	glm::vec2 getGlyphSize() const { return glyph_size; }
	glm::vec2 getGlyphUVOffset(char c) const;
	glm::vec2 getGlyphUVSize() const { return char_uv_size; }
};

}
