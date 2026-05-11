#include "deserialise.h"
#include "engine.h"
#include "mesh.h"
#include "node_view.h"
#include "package.h"
#include "user_interface.h"

using namespace HopEngine;

Ref<NodeView::Style> NodeView::Style::deserialise(const std::string& path)
{
    auto raw_data = Package::load(path);
    if (raw_data.empty()) return nullptr;

    const std::string token_str(reinterpret_cast<char*>(raw_data.data()), raw_data.size());
    const auto tokens = TokenReader::tokenise(token_str);
    if (tokens.empty()) return nullptr;

    const auto syntax_tree = TokenReader::extractSyntaxTree(tokens, token_str);
    if (syntax_tree.empty()) return nullptr;

    Ref<Style> style = new Style();

    Deserialiser deserialiser("error deserialising node style '" + path + "'");
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Font", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("atlas", TokenReader::TOKEN_STRING, true)
            .argument("glyph_size", TokenReader::TOKEN_VECTOR, true),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            std::string atlas;
            result.read("atlas", atlas);
            glm::ivec2 glyph_size;
            result.read("glyph_size", glyph_size);
            style->font = new Font(atlas, glyph_size);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Atlas", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("nodes", TokenReader::TOKEN_STRING, true)
            .argument("extra", TokenReader::TOKEN_STRING, true)
            .argument("ui", TokenReader::TOKEN_STRING, true),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            std::string node_atlas;
            result.read("nodes", node_atlas);
            std::string extra_atlas;
            result.read("extra", extra_atlas);
            std::string ui_atlas;
            result.read("ui", ui_atlas);
            style->node_atlas  = Engine::loadTexture(node_atlas);
            style->extra_atlas = Engine::loadTexture(extra_atlas);
            style->ui_atlas    = Engine::loadTexture(ui_atlas);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Grid", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("size", TokenReader::TOKEN_INT, true)
            .argument("show", TokenReader::TOKEN_TEXT, false)
            .argument("line_scale", TokenReader::TOKEN_INT, false)
            .argument("line_colour", TokenReader::TOKEN_VECTOR, false)
            .argument("dots_modulate", TokenReader::TOKEN_FLOAT, false)
            .argument("background_colour", TokenReader::TOKEN_VECTOR, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            result.read("size", style->grid_size);
            if (!result.read("show", style->show_grid))
                return deserialiser.emitError("invalid boolean constant value", result.offsetOf("show"),
                    token_str);
            result.read("line_scale", style->grid_scale);
            result.read("line_colour", style->grid_colour);
            result.read("dots_modulate", style->grid_dots_modulate);
            result.read("background_colour", style->background_colour);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Header", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("align", TokenReader::TOKEN_TEXT, false)
            .argument("position", TokenReader::TOKEN_TEXT, false)
            .argument("filled", TokenReader::TOKEN_TEXT, false)
            .argument("lines_after", TokenReader::TOKEN_INT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            if (!result.read<int>("align", style->header_align,
                    [](const std::string& s, int& d) -> bool
                    {
                        if (s == "LEFT") d = -1;
                        else if (s == "CENTER")
                            d = 0;
                        else if (s == "RIGHT")
                            d = 1;
                        else
                            return false;
                        return true;
                    }))
                return deserialiser.emitError("invalid header align value", result.offsetOf("align"),
                    token_str);
            if (!result.read<bool>("position", style->header_at_top,
                    [](const std::string& s, bool& d) -> bool
                    {
                        if (s == "TOP") d = true;
                        else if (s == "BOTTOM")
                            d = false;
                        else
                            return false;
                        return true;
                    }))
                return deserialiser.emitError("invalid header position value", result.offsetOf("position"),
                    token_str);
            if (!result.read("filled", style->header_fill))
                return deserialiser.emitError("invalid boolean constant value", result.offsetOf("filled"),
                    token_str);
            result.read("lines_after", style->after_header_spacing);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Text", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("offset", TokenReader::TOKEN_VECTOR, false)
            .argument("colour", TokenReader::TOKEN_VECTOR, false)
            .argument("spacing", TokenReader::TOKEN_INT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            result.read("offset", style->text_offset);
            result.read("colour", style->text_colour);
            result.read("spacing", style->text_spacing);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Outline", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("style", TokenReader::TOKEN_TEXT, true)
            .argument("colour", TokenReader::TOKEN_VECTOR, false)
            .argument("highlight", TokenReader::TOKEN_VECTOR, false)
            .argument("modulate", TokenReader::TOKEN_FLOAT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            if (!result.read<NodeView::OutlineStyle>("style", style->outline_style,
                    [](const std::string& s, NodeView::OutlineStyle& d) -> bool
                    {
                        if (s == "HIDDEN") d = HIDDEN;
                        else if (s == "PRESET_COLOUR")
                            d = PRESET_COLOUR;
                        else if (s == "NODE_COLOUR")
                            d = NODE_COLOUR;
                        else if (s == "MODULATE_NODE_COLOUR")
                            d = MODULATE_NODE_COLOUR;
                        else
                            return false;
                        return true;
                    }))
                return deserialiser.emitError("invalid outline style value", result.offsetOf("style"),
                    token_str);
            result.read("colour", style->outline_colour);
            result.read("highlight", style->outline_colour_highlight);
            result.read("modulate", style->outline_colour_mult);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Fill", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("style", TokenReader::TOKEN_TEXT, true)
            .argument("colour", TokenReader::TOKEN_VECTOR, false)
            .argument("modulate", TokenReader::TOKEN_FLOAT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            if (!result.read<bool>("style", style->fill_modulate_colour,
                    [](const std::string& s, bool& d) -> bool
                    {
                        if (s == "MODULATE_COLOUR") d = true;
                        else if (s == "PRESET_COLOUR")
                            d = false;
                        else
                            return false;
                        return true;
                    }))
                return deserialiser.emitError("invalid fill style value", result.offsetOf("style"),
                    token_str);
            result.read("colour", style->fill_colour);
            result.read("modulate", style->fill_colour_mult);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Shadows", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("show", TokenReader::TOKEN_TEXT, true)
            .argument("offset", TokenReader::TOKEN_VECTOR, false)
            .argument("colour", TokenReader::TOKEN_VECTOR, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            if (!result.read("show", style->shadows))
                return deserialiser.emitError("invalid shadow enable value", result.offsetOf("show"),
                    token_str);
            result.read("offset", style->shadow_offset);
            result.read("colour", style->shadow_colour);
            return true;
        });
    deserialiser.addStatementNamed(
        Deserialiser::NamedStatementSpec("Misc", Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN, false)
            .argument("pin_offset", TokenReader::TOKEN_INT, false)
            .argument("element_order", TokenReader::TOKEN_TEXT, false)
            .argument("center_text", TokenReader::TOKEN_TEXT, false)
            .argument("lines_after", TokenReader::TOKEN_INT, false),
        [&](Deserialiser::NamedStatementResult result) -> bool
        {
            result.read("pin_offset", style->pin_offset);
            if (!result.read<bool>("element_order", style->reverse_element_order,
                    [](const std::string& s, bool& d) -> bool
                    {
                        if (s == "FORWARD") d = false;
                        else if (s == "REVERSE")
                            d = true;
                        else
                            return false;
                        return true;
                    }))
                return deserialiser.emitError("invalid element ordering value",
                    result.offsetOf("element_order"), token_str);
            if (!result.read("center_text", style->center_text_elements))
                return deserialiser.emitError("invalid text centering value",
                    result.offsetOf("center_text"), token_str);
            result.read("lines_after", style->after_elements_spacing);
            return true;
        });

    if (!deserialiser.execute(syntax_tree, token_str)) return nullptr;
    return style;
}