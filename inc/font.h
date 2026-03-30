#pragma once

#include <glm/vec2.hpp>

#include "common.h"
#include "texture.h"

namespace HopEngine
{

/**
 * @brief contains information for rendering text from a font atlas
 */
class Font final : public Destructible
{
private:
	Ref<Texture> atlas = nullptr;	// texture containing glyph bitmaps
	glm::ivec2 glyph_size;			// size of each glyph in pixels
	glm::ivec2 chars_resolution;	// number of glyphs in the texture
	glm::vec2 char_uv_size;			// size of a glyph, as a fraction of the texture

public:
	DELETE_CONSTRUCTORS(Font);
	Font(const std::string& atlas_name, glm::ivec2 glyph_size_pixels);
	~Font() override;
	
	Ref<Texture> getAtlas() const;
	glm::vec2 getGlyphSize() const { return glyph_size; }
	/**
	 * @brief calculates the UV position for the glyph of a given character.
	 * @param c character to index to.
	 * @return UV coordinate for the top left corner of a glyph in the texture.
	 */
	glm::vec2 getGlyphUVOffset(char c) const;
	glm::vec2 getGlyphUVSize() const { return char_uv_size; }
	// TODO: should be able to just generate vertices/indices from the info...
};

}
