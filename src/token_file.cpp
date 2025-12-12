#include "token_file.h"

#include <format>

using namespace HopEngine;
using namespace std;

vector<TokenReader::Token> TokenReader::tokenise(const string& content, bool trim_comments, bool trim_whitespace)
{
    string trimmed_content;
    trimmed_content.reserve(content.size());

    for (char c : content)
    {
        if (c != '\r')
            trimmed_content.push_back(c);
    }

    if (trimmed_content.length() == 0) return { };

    size_t offset = 0;
    vector<Token> tokens;
    string current_token = "";
    TokenType current_type = getType(trimmed_content[0]);
    size_t start_offset = 0;
    if (current_type != TEXT && current_type != COMMENT && current_type != WHITESPACE && current_type != NEWLINE)
    {
        reportError("invalid first token", offset, trimmed_content);
        return { };
    }

    current_type = WHITESPACE;

    while (offset < trimmed_content.length())
    {
        char chr = trimmed_content[offset];
        TokenType char_type = getType(chr);

        TokenType new_type = current_type;
        bool append_chr = true;
        bool reset_token = false;
        Token finished_token = Token(current_type);
        finished_token.start_offset = start_offset;

        if (char_type == INVALID && current_type != STRING)
        {
            reportError("illegal character", offset, trimmed_content);
            return { };
        }
        if (char_type == END_VECTOR && current_type != VECTOR)
        {
            reportError("invalid end of vector token", offset, trimmed_content);
            return { };
        }

        switch (current_type)
        {
        case TEXT:
            if (char_type == TEXT || char_type == INT)
                break;
            else if (isSeparator(char_type))
            {
                finished_token.s_value = current_token;
                reset_token = true;
                break;
            }
            else
            {
                reportError("invalid conjoined tokens", offset, trimmed_content);
                return { };
            }
        case STRING:
            if (char_type == STRING)
            {
                append_chr = false;
                finished_token.s_value = current_token.substr(1);
                reset_token = true;
                char_type = INVALID;
                break;
            }
            else
                break;
        case INT:
            if (char_type == INT)
                break;
            else if (char_type == FLOAT)
            {
                new_type = FLOAT;
                break;
            }
            else if (isSeparator(char_type))
            {
                finished_token.i_value = stoi(current_token);
                reset_token = true;
                break;
            }
            else
            {
                reportError("invalid conjoined tokens", offset, trimmed_content);
                return { };
            }
        case FLOAT:
            if (char_type == INT)
                break;
            else if (char_type == FLOAT)
            {
                reportError("invalid float literal", offset, trimmed_content);
                return { };
            }
            else if (isSeparator(char_type))
            {
                finished_token.f_value = stof(current_token);
                reset_token = true;
                break;
            }
            else
            {
                reportError("invalid conjoined tokens", offset, trimmed_content);
                return { };
            }
        case IDENTIFIER:
            if (char_type == TEXT || char_type == INT)
                break;
            else if (isSeparator(char_type))
            {
                finished_token.s_value = current_token.substr(1);
                reset_token = true;
                break;
            }
            else
            {
                reportError("invalid conjoined tokens", offset, trimmed_content);
                return { };
            }
        case VECTOR:
            if (char_type == WHITESPACE)
            {
                append_chr = false;
                break;
            }
            else if (char_type == INT || char_type == FLOAT || char_type == COMMA)
                break;
            else if (char_type == END_VECTOR)
            {
                finished_token.c_value = deserialiseVectorToken(current_token.substr(1), offset, trimmed_content);
                append_chr = false;
                reset_token = true;
                char_type = INVALID;
                break;
            }
            else if (char_type == VECTOR)
            {
                reportError("invalid nested vector token", offset, trimmed_content);
                return { };
            }
            else
            {
                reportError("invalid token inside vector", offset, trimmed_content);
                return { };
            }
        case COMMENT:
            if (char_type != COMMENT && current_token.length() < 2)
            {
                reportError("incomplete comment initiator", offset, trimmed_content);
                return { };
            }
            else if (char_type == NEWLINE)
            {
                finished_token.s_value = current_token;
                append_chr = false;
                reset_token = true;
                break;
            }
            else
                break;
        case WHITESPACE:
            if (char_type == WHITESPACE)
                break;
            else
            {
                reset_token = true;
                break;
            }
        case INVALID:
            if (!isSeparator(char_type))
            {
                reportError("invalid conjoined tokens", offset, trimmed_content);
                return { };
            }
            else
            {
                reset_token = true;
                break;
            }
        default:
            reset_token = true;
            break;
        }

        if (reset_token)
        {
            if (current_type == TokenType::INVALID
                || (trim_whitespace && (current_type == WHITESPACE || current_type == NEWLINE))
                || (trim_comments && (current_type == COMMENT)))
            {
                // ignore the token
            }
            else
                tokens.push_back(finished_token);
            current_token = "";
            start_offset = offset;
            new_type = char_type;
        }
        current_type = new_type;

        if (append_chr)
            current_token.push_back(chr);

        offset++;
    }
    
    if (current_type == VECTOR || current_type == STRING)
    {
        reportError("invalid unclosed token at end of content", offset - 2, trimmed_content);
        return { };
    }

    return tokens;
}

size_t TokenReader::findClosingBrace(const vector<Token>& tokens, size_t open_index, const string& original_content)
{
    vector<Token> brackets;
    size_t index = open_index;

    while (index < tokens.size())
    {
        switch (tokens[index].type)
        {
        case OPEN_ROUND:
        case OPEN_CURLY:
            brackets.push_back(tokens[index]);
            break;
        case CLOSE_ROUND:
            if (!brackets.empty() && brackets[brackets.size() - 1].type == TokenType::OPEN_ROUND)
                brackets.pop_back();
            else
                return reportError("invalid closing bracket", tokens[index].start_offset, original_content);
            break;
        case CLOSE_CURLY:
            if (!brackets.empty() && brackets[brackets.size() - 1].type == TokenType::OPEN_CURLY)
                brackets.pop_back();
            else
                return reportError("invalid closing curly brace", tokens[index].start_offset, original_content);
            break;
        default:
            break;
        }

        if (brackets.size() == 0)
            break;

        ++index;
    }

    if (index >= tokens.size())
        return reportError("missing closing " + string(tokens[open_index].type == TokenType::OPEN_ROUND ? "bracket" : "curly brace"), tokens[open_index].start_offset, original_content);

    return index;
}

glm::vec4 TokenReader::deserialiseVectorToken(string str, size_t offset, const string& original_content)
{
    vector<float> values;
    
    try
    {
        size_t next_comma = -1;
        do
        {
            size_t last_comma = next_comma + 1;
            next_comma = str.find(',', last_comma);
            values.push_back(stof(str.substr(last_comma, next_comma - last_comma)));
        } while (next_comma != string::npos);
    }
    catch (invalid_argument e)
    {
        reportError("invalid vector literal", offset, original_content);
        return { 0, 0, 0, 0 };
    }

    if (values.size() > 4)
    {
        reportError("too many values in vector literal", offset, original_content);
        return { 0, 0, 0, 0 };
    }

    glm::vec4 value = { 0, 0, 0, 0 };
    for (size_t i = 0; i < values.size(); ++i)
    {
        value[i] = values[i];
    }

    return value;
}

size_t TokenReader::reportError(const string err, size_t off, const string& str)
{
    int32_t extract_start = max(0, (int32_t)off - 16);
    int32_t extract_end = extract_start + 32;
    while (true)
    {
        size_t find = str.find('\n', extract_start);
        if (find >= off) break;
        extract_start = find + 1;
    }
    size_t find = str.find('\n', off);
    if (find != string::npos)
    {
        if ((int32_t)find < extract_end)
            extract_end = find;
    }
    string extract = str.substr(extract_start, extract_end - extract_start);

    size_t ln = 0;
    size_t last = 0;
    size_t next = 0;
    while (next < off)
    {
        ln++;
        last = next;
        next = str.find('\n', next + 1);
    }
    size_t col = off - last;
    if (ln > 0) col--;
    if (col > 0 && ln > 0) ln--;

    string error = format("token document parsing error: {}"
                        "\n\t-> '... {} ...'"
                        "\n\t->{}      ^ here (ln {}, col {})", err, extract, string((int32_t)off - extract_start, ' '), ln + 1, col + 1);
    DBG_ERROR(error);
    return -1;
}
