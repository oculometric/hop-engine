#include "node_view.h"

#include "package.h"
#include "deserialise.h"
#include "engine.h"

using namespace HopEngine;
using namespace std;

Ref<NodeView::Style> NodeView::Style::deserialise(const string& path)
{
    auto raw_data = Package::load(path);
	if (raw_data.empty())
		return nullptr;

	const string token_str(reinterpret_cast<char*>(raw_data.data()), raw_data.size());
	const auto tokens = TokenReader::tokenise(token_str);
	if (tokens.empty())
		return nullptr;

	const auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
	if (syntax_tree.empty())
		return nullptr;

    Ref<Style> style = new Style();

    for (const auto& statement : syntax_tree)
    {
        if (statement.keyword == "Font")
        {
            map<string, TokenReader::Token> result;
            if (!TokenReader::readStatementNamed(statement, false, false,
                { { "atlas", { TokenReader::TokenType::STRING, true } },
                  { "glyph_size", { TokenReader::TokenType::VECTOR, true } } },
                result, "error deserialising NodeView::Style"))
            {
                return style;
            }
            style->font = new Font(result["atlas"].s_value, result["glyph_size"].c_value);
        }
        else if (statement.keyword == "Atlas")
        {
            map<string, TokenReader::Token> result;
            if (!TokenReader::readStatementNamed(statement, false, false,
                { { "nodes", { TokenReader::TokenType::STRING, false } },
                  { "extra", { TokenReader::TokenType::STRING, false } },
                  { "ui", { TokenReader::TokenType::STRING, false } } },
                result, "error deserialising NodeView::Style"))
            {
                return style;
            }
            auto it = result.find("nodes");
            if (it != result.end())
                style->node_atlas = Engine::loadTexture(it->second.s_value);
            else
                style->node_atlas = Engine::loadTexture("res://engine/textures/node_atlas.png");
            it = result.find("extra");
            if (it != result.end())
                style->extra_atlas = Engine::loadTexture(it->second.s_value);
            else
                style->extra_atlas = Engine::loadTexture("res://engine/textures/extra_atlas.png");
            it = result.find("ui");
            if (it != result.end())
                style->ui_atlas = Engine::loadTexture(it->second.s_value);
            else
                style->ui_atlas = Engine::loadTexture("res://engine/textures/ui_atlas.png");
        }
        else if (statement.keyword == "Grid")
        {
            map<string, TokenReader::Token> result;
            if (!TokenReader::readStatementNamed(statement, false, false,
                { { "size",              { TokenReader::TokenType::INT, false } },
                  { "show",              { TokenReader::TokenType::TEXT, false } },
                  { "line_scale",        { TokenReader::TokenType::INT, false } },
                  { "line_colour",       { TokenReader::TokenType::VECTOR, false } },
                  { "dots_modulate",     { TokenReader::TokenType::FLOAT, false } },
                  { "background_colour", { TokenReader::TokenType::VECTOR, false } }, },
                result, "error deserialising NodeView::Style"))
            {
                return style;
            }
            auto it = result.find("size");
            if (it != result.end())
                style->grid_size = it->second.i_value;
            // TODO: grid settings
        }
        else if (statement.keyword == "Header")
        {
            // TODO: header settings
        }
        else if (statement.keyword == "Text")
        {
            // TODO: text settings
        }
        else if (statement.keyword == "Outline")
        {
            // TODO: outline settings
        }
        else if (statement.keyword == "Fill")
        {
            // TODO: fill settings
        }
        else if (statement.keyword == "Shadows")
        {
            // TODO: shadow settings
        }
        else if (statement.keyword == "Misc")
        {
            // TODO: other settings
        }
    }

    return style;
}