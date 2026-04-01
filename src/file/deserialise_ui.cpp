#include "user_interface.h"

#include "deserialise.h"
#include "package.h"
#include "engine.h"

using namespace HopEngine;

Ref<Font> Font::deserialise(const std::string& path)
{
    auto raw_data = Package::load(path);
	if (raw_data.empty())
		return nullptr;

	const std::string token_str(reinterpret_cast<char*>(raw_data.data()), raw_data.size());
	const auto tokens = TokenReader::tokenise(token_str);
	if (tokens.empty())
		return nullptr;

	const auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
	if (syntax_tree.empty())
		return nullptr;

    std::map<std::string, Ref<Texture>> textures;
    Ref<Texture> regular_atlas;
    Ref<Texture> bold_atlas;
    glm::ivec2 glyph_size{ 1, 1 };

    Deserialiser deserialiser("error deserialising font '" + path + "'");
    deserialiser.addStatementAnonymous(
        Deserialiser::AnonymousStatementSpec("Resource", Deserialiser::STATEMENT_IDENTIFIER_REQUIRED, false)
            .argument(TokenReader::TEXT)
            .argument(TokenReader::STRING),
        [&](Deserialiser::AnonymousStatementResult result) -> bool
    {
        std::string res_type; result.read(0, res_type);
        std::string res_addr; result.read(1, res_addr);
        if (res_type == "texture")
            textures[result.statement.identifier] = Engine::loadTexture(res_addr);
        else
            return deserialiser.emitError("invalid resource type");
        return true;
    });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Atlas", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("regular", TokenReader::IDENTIFIER, true)
            .argument("bold", TokenReader::IDENTIFIER, false)
            .argument("glyph_size", TokenReader::VECTOR, true),
        [&](Deserialiser::NamedStatementResult result) -> bool
    {
        std::string regular; result.read("regular", regular);
        std::string bold; result.read("bold", bold);
        result.read("glyph_size", glyph_size);

        if (!textures.contains(regular))
            return deserialiser.emitError("invalid texture resource for 'regular' atlas");
        regular_atlas = textures[regular];
        if (!bold.empty())
        {
            if (!textures.contains(bold))
                return deserialiser.emitError("invalid texture resource for 'bold' atlas");
            bold_atlas = textures[bold];
        }
        return true;
    });
    
    deserialiser.execute(syntax_tree);

    return new Font(std::vector{ regular_atlas, bold_atlas }, glyph_size);
}
