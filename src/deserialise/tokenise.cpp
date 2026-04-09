#include "deserialise.h"

#include <algorithm>
#include <format>

using namespace HopEngine;

TokenReader::Token::Token(const Token& other)
{
    type         = other.type;
    start_offset = other.start_offset;

    switch (static_cast<TokenTypeInternal>(type))
    {
    case TOKEN_INTERNAL_TEXT:
    case TOKEN_INTERNAL_STRING:
    case TOKEN_INTERNAL_IDENTIFIER:
    case TOKEN_INTERNAL_COMMENT:    s_value = other.s_value; break;
    case TOKEN_INTERNAL_VECTOR:     c_value = other.c_value; break;
    case TOKEN_INTERNAL_INT:        i_value = other.i_value; break;
    case TOKEN_INTERNAL_FLOAT:      f_value = other.f_value; break;
    default:                        break;
    }
}

TokenReader::Token TokenReader::Token::operator=(const Token& other)
{
    type         = other.type;
    start_offset = other.start_offset;

    switch (static_cast<TokenTypeInternal>(type))
    {
    case TOKEN_INTERNAL_IDENTIFIER:
    case TOKEN_INTERNAL_TEXT:
    case TOKEN_INTERNAL_STRING:
    case TOKEN_INTERNAL_COMMENT:    s_value = other.s_value; break;
    case TOKEN_INTERNAL_VECTOR:     c_value = other.c_value; break;
    case TOKEN_INTERNAL_INT:        i_value = other.i_value; break;
    case TOKEN_INTERNAL_FLOAT:      f_value = other.f_value; break;
    default:                        break;
    }

    return *this;
}

TokenReader::Token TokenReader::Token::operator=(Token&& other) noexcept
{
    type         = other.type;
    start_offset = other.start_offset;

    switch (static_cast<TokenTypeInternal>(type))
    {
    case TOKEN_INTERNAL_IDENTIFIER:
    case TOKEN_INTERNAL_TEXT:
    case TOKEN_INTERNAL_STRING:
    case TOKEN_INTERNAL_COMMENT:    s_value = other.s_value; break;
    case TOKEN_INTERNAL_VECTOR:     c_value = other.c_value; break;
    case TOKEN_INTERNAL_INT:        i_value = other.i_value; break;
    case TOKEN_INTERNAL_FLOAT:      f_value = other.f_value; break;
    default:                        break;
    }

    return *this;
}

bool operator==(TokenReader::TokenType a, TokenReader::TokenTypeInternal b)
{ return static_cast<TokenReader::TokenTypeInternal>(a) == b; }

std::vector<TokenReader::Token> TokenReader::tokenise(const std::string& content, bool trim_comments,
    bool trim_whitespace)
{
    std::string trimmed_content;
    trimmed_content.reserve(content.size());

    for (char c : content)
        if (c != '\r') trimmed_content.push_back(c);

    if (trimmed_content.empty()) return {};

    size_t offset = 0;
    std::vector<Token> tokens;
    std::string current_token;
    TokenTypeInternal current_type = getType(trimmed_content[0]);
    size_t start_offset            = 0;
    if (current_type != TOKEN_INTERNAL_TEXT && current_type != TOKEN_INTERNAL_COMMENT &&
        current_type != TOKEN_INTERNAL_WHITESPACE && current_type != TOKEN_INTERNAL_NEWLINE)
    {
        reportError("invalid first token", offset, trimmed_content);
        return {};
    }

    current_type = TOKEN_INTERNAL_WHITESPACE;

    while (offset < trimmed_content.length())
    {
        char chr                    = trimmed_content[offset];
        TokenTypeInternal char_type = getType(chr);
        TokenTypeInternal new_type  = current_type;
        bool append_chr             = true;
        bool reset_token            = false;
        Token finished_token        = Token(static_cast<TokenType>(current_type));
        finished_token.start_offset = start_offset;

        if (char_type == TOKEN_INTERNAL_INVALID &&
            !(current_type == TOKEN_INTERNAL_STRING || current_type == TOKEN_INTERNAL_COMMENT))
        {
            reportError("illegal character", offset, trimmed_content);
            return {};
        }
        if (char_type == TOKEN_INTERNAL_END_VECTOR &&
            !(current_type == TOKEN_INTERNAL_VECTOR || current_type == TOKEN_INTERNAL_STRING ||
                current_type == TOKEN_INTERNAL_COMMENT))
        {
            reportError("invalid end of vector token", offset, trimmed_content);
            return {};
        }

        switch (current_type)
        {
        case TOKEN_INTERNAL_TEXT:
            if (char_type == TOKEN_INTERNAL_TEXT || char_type == TOKEN_INTERNAL_INT) break;
            if (isSeparator(char_type))
            {
                finished_token.s_value = current_token;
                reset_token            = true;
                break;
            }
            reportError("invalid conjoined tokens", offset, trimmed_content);
            return {};
        case TOKEN_INTERNAL_STRING:
            if (char_type == TOKEN_INTERNAL_STRING)
            {
                append_chr             = false;
                finished_token.s_value = current_token.substr(1);
                reset_token            = true;
                char_type              = TOKEN_INTERNAL_INVALID;
                break;
            }
            break;
        case TOKEN_INTERNAL_INT:
            if (char_type == TOKEN_INTERNAL_INT) break;
            if (char_type == TOKEN_INTERNAL_FLOAT)
            {
                new_type = TOKEN_INTERNAL_FLOAT;
                break;
            }
            if (char_type == TOKEN_INTERNAL_TEXT)
            {
                new_type = TOKEN_INTERNAL_TEXT;
                break;
            }
            if (isSeparator(char_type))
            {
                finished_token.i_value = stoi(current_token);
                reset_token            = true;
                break;
            }
            reportError("invalid conjoined tokens", offset, trimmed_content);
            return {};
        case TOKEN_INTERNAL_FLOAT:
            if (char_type == TOKEN_INTERNAL_INT) break;
            if (char_type == TOKEN_INTERNAL_FLOAT)
            {
                reportError("invalid float literal", offset, trimmed_content);
                return {};
            }
            if (isSeparator(char_type))
            {
                finished_token.f_value = stof(current_token);
                reset_token            = true;
                break;
            }
            reportError("invalid conjoined tokens", offset, trimmed_content);
            return {};
        case TOKEN_INTERNAL_IDENTIFIER:
            if (char_type == TOKEN_INTERNAL_TEXT || char_type == TOKEN_INTERNAL_INT) break;
            if (isSeparator(char_type))
            {
                finished_token.s_value = current_token.substr(1);
                reset_token            = true;
                break;
            }
            reportError("invalid conjoined tokens", offset, trimmed_content);
            return {};
        case TOKEN_INTERNAL_VECTOR:
            if (char_type == TOKEN_INTERNAL_WHITESPACE)
            {
                append_chr = false;
                break;
            }
            if (char_type == TOKEN_INTERNAL_INT || char_type == TOKEN_INTERNAL_FLOAT ||
                char_type == TOKEN_INTERNAL_COMMA)
                break;
            if (char_type == TOKEN_INTERNAL_END_VECTOR)
            {
                finished_token.c_value =
                    deserialiseVectorToken(current_token.substr(1), offset, trimmed_content);
                append_chr  = false;
                reset_token = true;
                char_type   = TOKEN_INTERNAL_INVALID;
                break;
            }
            if (char_type == TOKEN_INTERNAL_VECTOR)
            {
                reportError("invalid nested vector token", offset, trimmed_content);
                return {};
            }
            reportError("invalid token inside vector", offset, trimmed_content);
            return {};
        case TOKEN_INTERNAL_COMMENT:
            if (char_type != TOKEN_INTERNAL_COMMENT && current_token.length() < 2)
            {
                reportError("incomplete comment initiator", offset, trimmed_content);
                return {};
            }
            if (char_type == TOKEN_INTERNAL_NEWLINE)
            {
                finished_token.s_value = current_token;
                append_chr             = false;
                reset_token            = true;
                break;
            }
            new_type = TOKEN_INTERNAL_COMMENT;
            break;
        case TOKEN_INTERNAL_WHITESPACE:
            if (char_type == TOKEN_INTERNAL_WHITESPACE) break;
            reset_token = true;
            break;
        case TOKEN_INTERNAL_INVALID:
            if (!isSeparator(char_type))
            {
                reportError("invalid conjoined tokens", offset, trimmed_content);
                return {};
            }
            reset_token = true;
            break;
        default: reset_token = true; break;
        }

        if (reset_token)
        {
            if (current_type == TOKEN_INTERNAL_INVALID ||
                (trim_whitespace && (current_type == TOKEN_INTERNAL_WHITESPACE ||
                                        current_type == TOKEN_INTERNAL_NEWLINE)) ||
                (trim_comments && (current_type == TOKEN_INTERNAL_COMMENT)))
            {
                // ignore the token
            }
            else
                tokens.push_back(finished_token);
            current_token.clear();
            start_offset = offset;
            new_type     = char_type;
        }
        current_type = new_type;

        if (append_chr) current_token.push_back(chr);

        ++offset;
    }

    if (current_type == TOKEN_INTERNAL_VECTOR || current_type == TOKEN_INTERNAL_STRING)
    {
        reportError("invalid unclosed token at end of content", offset - 2, trimmed_content);
        return {};
    }

    return tokens;
}

std::vector<TokenReader::Token>::const_iterator TokenReader::findClosingBrace(
    const std::vector<Token>& tokens, const std::vector<Token>::const_iterator open_index,
    const std::string& original_content)
{
    std::vector<Token> brackets;
    std::vector<Token>::const_iterator index = open_index;

    while (index != tokens.end())
    {
        switch (static_cast<TokenTypeInternal>(index->type))
        {
        case TOKEN_INTERNAL_OPEN_ROUND:
        case TOKEN_INTERNAL_OPEN_CURLY: brackets.push_back(*index); break;
        case TOKEN_INTERNAL_CLOSE_ROUND:
            if (!brackets.empty() && brackets[brackets.size() - 1].type == TOKEN_INTERNAL_OPEN_ROUND)
                brackets.pop_back();
            else
            {
                reportError("invalid closing bracket", index->start_offset, original_content);
                return tokens.end();
            }
            break;
        case TOKEN_INTERNAL_CLOSE_CURLY:
            if (!brackets.empty() && brackets[brackets.size() - 1].type == TOKEN_INTERNAL_OPEN_CURLY)
                brackets.pop_back();
            else
            {
                reportError("invalid closing curly brace", index->start_offset, original_content);
                return tokens.end();
            }
            break;
        default: break;
        }

        if (brackets.empty()) break;

        ++index;
    }

    if (index == tokens.end())
    {
        reportError("missing closing " + std::string(static_cast<TokenTypeInternal>(open_index->type) ==
                                                             TOKEN_INTERNAL_OPEN_ROUND
                                                         ? "bracket"
                                                         : "curly brace"),
            open_index->start_offset, original_content);
        return tokens.end();
    }

    return index;
}

std::vector<TokenReader::Statement> TokenReader::extractSyntaxTree(const std::vector<Token>& tokens,
    const std::string& original_content)
{
    std::vector<Statement> statements;

    auto statement_start_it = tokens.begin();
    while (statement_start_it != tokens.end())
    {
        auto arg_start_it      = tokens.end();
        auto arg_end_it        = tokens.end();
        auto identifier_it     = tokens.end();
        auto children_start_it = tokens.end();
        auto children_end_it   = tokens.end();
        auto statement_end_it  = tokens.end();
        auto current_it        = statement_start_it;
        int stage              = 0;
        while (current_it != tokens.end() && stage <= 5)
        {
            if (current_it->type == TOKEN_INTERNAL_WHITESPACE ||
                current_it->type == TOKEN_INTERNAL_COMMENT ||
                current_it->type == TOKEN_INTERNAL_NEWLINE)
            {
                ++current_it;
                continue;
            }

            if (current_it->type == TOKEN_INTERNAL_SEMICOLON)
            {
                statement_end_it = current_it;
                break;
            }

            switch (stage)
            {
            case 0: // looking for the keyword
                if (current_it->type == TOKEN_INTERNAL_TEXT)
                {
                    statement_start_it = current_it;
                    stage              = 1;
                }
                else
                {
                    reportError("expected keyword", current_it->start_offset, original_content);
                    return {};
                }
                break;
            case 1: // looking for the argument list
                if (current_it->type == TOKEN_INTERNAL_OPEN_ROUND)
                {
                    arg_start_it = current_it;
                    arg_end_it   = findClosingBrace(tokens, current_it, original_content);
                    if (arg_end_it == tokens.end()) return {};
                    current_it = arg_end_it;
                    stage      = 2;
                }
                else if (current_it->type == TOKEN_INTERNAL_COLON) { stage = 3; }
                else if (current_it->type == TOKEN_INTERNAL_OPEN_CURLY)
                {
                    stage = 5;
                    --current_it;
                }
                else
                {
                    reportError("unexpected token", current_it->start_offset, original_content);
                    return {};
                }
                break;
            case 2: // looking for the colon
                if (current_it->type == TOKEN_INTERNAL_COLON) { stage = 3; }
                else if (current_it->type == TOKEN_INTERNAL_OPEN_CURLY)
                {
                    stage = 4;
                    --current_it;
                }
                else
                {
                    reportError("unexpected token", current_it->start_offset, original_content);
                    return {};
                }
                break;
            case 3: // looking for the identifier
                if (current_it->type == TOKEN_INTERNAL_TEXT)
                {
                    identifier_it = current_it;
                    stage         = 4;
                }
                else
                {
                    reportError("expected identifier", current_it->start_offset, original_content);
                    return {};
                }
                break;
            case 4: // looking for the child list
                if (current_it->type == TOKEN_INTERNAL_OPEN_CURLY)
                {
                    children_start_it = current_it;
                    children_end_it   = findClosingBrace(tokens, current_it, original_content);
                    if (children_end_it == tokens.end()) return {};
                    current_it = children_end_it;
                    stage      = 5;
                }
                else
                {
                    reportError("unexpected token", current_it->start_offset, original_content);
                    return {};
                }
                break;
            case 5: // looking for the semicolon
                if (current_it->type == TOKEN_INTERNAL_SEMICOLON)
                {
                    statement_end_it = current_it;
                    break;
                }
                reportError("unexpected token", current_it->start_offset, original_content);
                return {};
            default:
                reportError("unexpected token", current_it->start_offset, original_content);
                return {};
            }

            ++current_it;
        }

        if (statement_start_it == tokens.end()) break;

        Statement statement;
        statement.start_offset = statement_start_it->start_offset;
        statement.keyword      = statement_start_it->s_value;
        if (arg_start_it != arg_end_it)
            statement.arguments = parseArguments(arg_start_it, arg_end_it, original_content);
        if (identifier_it != tokens.end()) statement.identifier = identifier_it->s_value;
        if (children_start_it != children_end_it)
        {
            std::vector<Token> child_tokens;
            child_tokens.insert(child_tokens.begin(), children_start_it + 1, children_end_it - 1);
            statement.children = extractSyntaxTree(child_tokens, original_content);
        }
        statements.push_back(statement);

        statement_start_it = statement_end_it;
        if (statement_start_it == tokens.end()) break;

        ++statement_start_it;
    }

    return statements;
}

std::string TokenReader::generateError(size_t offset, const std::string& str)
{
    int32_t extract_start = glm::max(0, static_cast<int32_t>(offset) - 16);
    int32_t extract_end   = extract_start + 32;
    while (true)
    {
        const size_t find = str.find('\n', extract_start);
        if (find >= offset) break;
        extract_start = static_cast<int32_t>(find) + 1;
    }
    const size_t find = str.find('\n', offset);
    if (find != std::string::npos) extract_end = glm::min(static_cast<int32_t>(find), extract_end);
    std::string extract = str.substr(extract_start, extract_end - extract_start);

    size_t ln   = 0;
    size_t last = 0;
    size_t next = 0;
    while (next < offset)
    {
        ln++;
        last = next;
        next = str.find('\n', next + 1);
    }
    size_t col = offset - last;
    if (ln > 0) col--;
    if (col > 0 && ln > 0) ln--;

    const std::string error = std::format(
        "\n  -> '... {} ...'"
        "\n  ->{}      ^ here (ln {}, col {})",
        extract, std::string(static_cast<int32_t>(offset) - extract_start, ' '), ln + 1, col + 1);

    return error;
}

glm::vec4 TokenReader::deserialiseVectorToken(const std::string& str, const size_t offset,
    const std::string& original_content)
{
    std::vector<float> values;
    try
    {
        size_t next_comma = SIZE_MAX;
        do
        {
            const size_t last_comma = next_comma + 1;
            next_comma              = str.find(',', last_comma);
            values.push_back(stof(str.substr(last_comma, next_comma - last_comma)));
        } while (next_comma != std::string::npos);
    }
    catch (std::invalid_argument& e)
    {
        reportError("invalid vector literal: " + std::string(e.what()), offset, original_content);
        return { 0, 0, 0, 0 };
    }

    if (values.size() > 4)
    {
        reportError("too many values in vector literal", offset, original_content);
        return { 0, 0, 0, 0 };
    }

    glm::vec4 value = { 0, 0, 0, 0 };
    for (size_t i = 0; i < values.size(); ++i) value[static_cast<glm::length_t>(i)] = values[i];

    return value;
}

std::vector<std::pair<std::string, TokenReader::Token>> TokenReader::parseArguments(
    std::vector<Token>::const_iterator start, std::vector<Token>::const_iterator end,
    const std::string& original_content)
{
    auto current = start + 1;

    std::vector<std::pair<std::string, Token>> arguments;
    Token keyword_token(TOKEN_TEXT);
    int stage = 0;

    while (current != end)
    {
        if (current->type == TOKEN_INTERNAL_WHITESPACE || current->type == TOKEN_INTERNAL_COMMENT ||
            current->type == TOKEN_INTERNAL_NEWLINE)
        {
            ++current;
            continue;
        }

        switch (stage)
        {
        case 0: // looking for the argument identifier or value
            switch (static_cast<TokenTypeInternal>(current->type))
            {
            case TOKEN_INTERNAL_TEXT:
                stage         = 1;
                keyword_token = *current;
                break;
            case TOKEN_INTERNAL_VECTOR:
            case TOKEN_INTERNAL_STRING:
            case TOKEN_INTERNAL_INT:
            case TOKEN_INTERNAL_FLOAT:
            case TOKEN_INTERNAL_IDENTIFIER:
                keyword_token = Token(TOKEN_TEXT);
                arguments.emplace_back("", *current);
                stage = 3;
                break;
            default:
                reportError("unexpected token", current->start_offset, original_content);
                return {};
            }
            break;
        case 1: // looking for the equals sign
            if (current->type == TOKEN_INTERNAL_COMMA)
            {
                arguments.emplace_back("", keyword_token);
                keyword_token = Token(TOKEN_TEXT);
                stage         = 0;
                break;
            }
            if (current->type == TOKEN_INTERNAL_EQUALS) stage = 2;
            else
            {
                reportError("unexpected token", current->start_offset, original_content);
                return {};
            }
            break;
        case 2: // looking for the argument value
            switch (static_cast<TokenTypeInternal>(current->type))
            {
            case TOKEN_INTERNAL_TEXT:
            case TOKEN_INTERNAL_VECTOR:
            case TOKEN_INTERNAL_STRING:
            case TOKEN_INTERNAL_INT:
            case TOKEN_INTERNAL_FLOAT:
            case TOKEN_INTERNAL_IDENTIFIER:
                arguments.emplace_back(keyword_token.s_value, *current);
                keyword_token = Token(TOKEN_TEXT);
                stage         = 3;
                break;
            default:
                reportError("unexpected token", current->start_offset, original_content);
                return {};
            }
            break;
        case 3: // looking for comma
            if (current->type == TOKEN_INTERNAL_COMMA) stage = 0;
            else
            {
                reportError("unexpected token", current->start_offset, original_content);
                return {};
            }
            break;
        }

        ++current;
    }

    return arguments;
}

size_t TokenReader::reportError(const std::string& err, const size_t off, const std::string& str)
{
    DBG_ERROR("token document parsing error: " + err + generateError(off, str));
    return SIZE_MAX;
}
