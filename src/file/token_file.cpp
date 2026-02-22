#include "token_file.h"

#include <algorithm>
#include <format>

using namespace HopEngine;
using namespace std;

vector<TokenReader::Token> TokenReader::tokenise(const string& content, bool trim_comments, bool trim_whitespace)
{
    string trimmed_content;
    trimmed_content.reserve(content.size());

    for (char c : content)
        if (c != '\r') trimmed_content.push_back(c);

    if (trimmed_content.empty()) return { };

    size_t offset = 0;
    vector<Token> tokens;
    string current_token;
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

        if (char_type == INVALID && !(current_type == STRING || current_type == COMMENT))
        {
            reportError("illegal character", offset, trimmed_content);
            return { };
        }
        if (char_type == END_VECTOR && !(current_type == VECTOR || current_type == STRING || current_type == COMMENT))
        {
            reportError("invalid end of vector token", offset, trimmed_content);
            return { };
        }

        switch (current_type)
        {
        case TEXT:
            if (char_type == TEXT || char_type == INT)
                break;
            if (isSeparator(char_type))
            {
                finished_token.s_value = current_token;
                reset_token = true;
                break;
            }
            reportError("invalid conjoined tokens", offset, trimmed_content);
            return { };
        case STRING:
            if (char_type == STRING)
            {
                append_chr = false;
                finished_token.s_value = current_token.substr(1);
                reset_token = true;
                char_type = INVALID;
                break;
            }
            break;
        case INT:
            if (char_type == INT)
                break;
            if (char_type == FLOAT)
            {
                new_type = FLOAT;
                break;
            }
            if (char_type == TEXT)
            {
                new_type = TEXT;
                break;
            }
            if (isSeparator(char_type))
            {
                finished_token.i_value = stoi(current_token);
                reset_token = true;
                break;
            }
            reportError("invalid conjoined tokens", offset, trimmed_content);
            return { };
        case FLOAT:
            if (char_type == INT)
                break;
            if (char_type == FLOAT)
            {
                reportError("invalid float literal", offset, trimmed_content);
                return { };
            }
            if (isSeparator(char_type))
            {
                finished_token.f_value = stof(current_token);
                reset_token = true;
                break;
            }
            reportError("invalid conjoined tokens", offset, trimmed_content);
            return { };
        case IDENTIFIER:
            if (char_type == TEXT || char_type == INT)
                break;
            if (isSeparator(char_type))
            {
                finished_token.s_value = current_token.substr(1);
                reset_token = true;
                break;
            }
            reportError("invalid conjoined tokens", offset, trimmed_content);
            return { };
        case VECTOR:
            if (char_type == WHITESPACE)
            {
                append_chr = false;
                break;
            }
            if (char_type == INT || char_type == FLOAT || char_type == COMMA)
                break;
            if (char_type == END_VECTOR)
            {
                finished_token.c_value = deserialiseVectorToken(current_token.substr(1), offset, trimmed_content);
                append_chr = false;
                reset_token = true;
                char_type = INVALID;
                break;
            }
            if (char_type == VECTOR)
            {
                reportError("invalid nested vector token", offset, trimmed_content);
                return { };
            }
            reportError("invalid token inside vector", offset, trimmed_content);
            return { };
        case COMMENT:
            if (char_type != COMMENT && current_token.length() < 2)
            {
                reportError("incomplete comment initiator", offset, trimmed_content);
                return { };
            }
            if (char_type == NEWLINE)
            {
                finished_token.s_value = current_token;
                append_chr = false;
                reset_token = true;
                break;
            }
            new_type = COMMENT;
            break;
        case WHITESPACE:
            if (char_type == WHITESPACE)
                break;
            reset_token = true;
            break;
        case INVALID:
            if (!isSeparator(char_type))
            {
                reportError("invalid conjoined tokens", offset, trimmed_content);
                return { };
            }
            reset_token = true;
            break;
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
            current_token.clear();
            start_offset = offset;
            new_type = char_type;
        }
        current_type = new_type;

        if (append_chr)
            current_token.push_back(chr);

        ++offset;
    }
    
    if (current_type == VECTOR || current_type == STRING)
    {
        reportError("invalid unclosed token at end of content", offset - 2, trimmed_content);
        return { };
    }

    return tokens;
}

vector<TokenReader::Token>::const_iterator TokenReader::findClosingBrace(const vector<Token>& tokens, const vector<Token>::const_iterator open_index, const string& original_content)
{
    vector<Token> brackets;
    vector<Token>::const_iterator index = open_index;

    while (index != tokens.end())
    {
        switch (index->type)
        {
        case OPEN_ROUND:
        case OPEN_CURLY:
            brackets.push_back(*index);
            break;
        case CLOSE_ROUND:
            if (!brackets.empty() && brackets[brackets.size() - 1].type == TokenType::OPEN_ROUND)
                brackets.pop_back();
            else
            {
                reportError("invalid closing bracket", index->start_offset, original_content);
                return tokens.end();
            }
            break;
        case CLOSE_CURLY:
            if (!brackets.empty() && brackets[brackets.size() - 1].type == TokenType::OPEN_CURLY)
                brackets.pop_back();
            else
            {
                reportError("invalid closing curly brace", index->start_offset, original_content);
                return tokens.end();
            }
            break;
        default:
            break;
        }

        if (brackets.empty())
            break;

        ++index;
    }

    if (index == tokens.end())
    {
        reportError("missing closing " + string(open_index->type == TokenType::OPEN_ROUND ? "bracket" : "curly brace"), open_index->start_offset, original_content);
        return tokens.end();
    }

    return index;
}

vector<TokenReader::Statement> TokenReader::extractSyntaxTree(const vector<Token>& tokens, const string& original_content)
{
    vector<Statement> statements;

    auto statement_start_it = tokens.begin();
    while (statement_start_it != tokens.end())
    {
        auto arg_start_it = tokens.end();
        auto arg_end_it = tokens.end();
        auto identifier_it = tokens.end();
        auto children_start_it = tokens.end();
        auto children_end_it = tokens.end();
        auto statement_end_it = tokens.end();
        auto current_it = statement_start_it;
        int stage = 0;
        while (current_it != tokens.end() && stage <= 5)
        {
            if (current_it->type == WHITESPACE || current_it->type == COMMENT || current_it->type == NEWLINE)
            {
                ++current_it;
                continue;
            }
            
            if (current_it->type == SEMICOLON)
            {
                statement_end_it = current_it;
                break;
            }

            switch (stage)
            {
            case 0: // looking for the keyword
                if (current_it->type == TEXT)
                {
                    statement_start_it = current_it;
                    stage = 1;
                }
                else
                {
                    reportError("expected keyword", current_it->start_offset, original_content);
                    return { };
                }
                break;
            case 1: // looking for the argument list
                if (current_it->type == OPEN_ROUND)
                {
                    arg_start_it = current_it;
                    arg_end_it = findClosingBrace(tokens, current_it, original_content);
                    if (arg_end_it == tokens.end())
                        return { };
                    current_it = arg_end_it;
                    stage = 2;
                }
                else if (current_it->type == COLON)
                {
                    stage = 3;
                }
                else if (current_it->type == OPEN_CURLY)
                {
                    stage = 5;
                    --current_it;
                }
                else
                {
                    reportError("unexpected token", current_it->start_offset, original_content);
                    return { };
                }
                break;
            case 2: // looking for the colon
                if (current_it->type == COLON)
                {
                    stage = 3;
                }
                else if (current_it->type == OPEN_CURLY)
                {
                    stage = 4;
                    --current_it;
                }
                else
                {
                    reportError("unexpected token", current_it->start_offset, original_content);
                    return { };
                }
                break;
            case 3: // looking for the identifier
                if (current_it->type == TEXT)
                {
                    identifier_it = current_it;
                    stage = 4;
                }
                else
                {
                    reportError("expected identifier", current_it->start_offset, original_content);
                    return { };
                }
                break;
            case 4: // looking for the child list
                if (current_it->type == OPEN_CURLY)
                {
                    children_start_it = current_it;
                    children_end_it = findClosingBrace(tokens, current_it, original_content);
                    if (children_end_it == tokens.end())
                        return { };
                    current_it = children_end_it;
                    stage = 5;
                }
                else
                {
                    reportError("unexpected token", current_it->start_offset, original_content);
                    return { };
                }
                break;
            case 5: // looking for the semicolon
                if (current_it->type == SEMICOLON)
                {
                    statement_end_it = current_it;
                    break;
                }
                reportError("unexpected token", current_it->start_offset, original_content);
                return { };
            default:
                reportError("unexpected token", current_it->start_offset, original_content);
                return { };
            }

            ++current_it;
        }

        if (statement_start_it == tokens.end())
            break;

        Statement statement;
        statement.keyword = statement_start_it->s_value;
        if (arg_start_it != arg_end_it)
            statement.arguments = parseArguments(arg_start_it, arg_end_it, original_content);
        if (identifier_it != tokens.end())
            statement.identifier = identifier_it->s_value;
        if (children_start_it != children_end_it)
        {
            vector<Token> child_tokens;
            child_tokens.insert(child_tokens.begin(), children_start_it + 1, children_end_it - 1);
            statement.children = extractSyntaxTree(child_tokens, original_content);
        }
        statements.push_back(statement);

        statement_start_it = statement_end_it;
        if (statement_start_it == tokens.end())
            break;

        ++statement_start_it;
    }

    return statements;
}

bool TokenReader::readStatementAnonymous(const Statement& statement, const bool children_allowed, const bool requires_identifier, const vector<TokenType>& expected_args, vector<Token>& extracted_args, const string& error_base)
{
    // check if there are children
    if (!statement.children.empty() && !children_allowed)
    {
        DBG_ERROR(error_base + ": children are not allowed in a '" + statement.keyword + "' statement");
        return false;
    }
    // check if there is an identifier
    if (statement.identifier.empty() && requires_identifier)
    {
        DBG_ERROR(error_base + ": an identifier is required in a '" + statement.keyword + "' statement");
        return false;
    }
    // check if enough args are present
    if (statement.arguments.size() > expected_args.size())
    {
        DBG_ERROR(error_base + ": too many arguments in '" + statement.keyword + "' statement, requires " + to_string(expected_args.size()));
        return false;
    }
    if (statement.arguments.size() < expected_args.size())
    {
        DBG_ERROR(error_base + ": not enough arguments in '" + statement.keyword + "' statement, requires " + to_string(expected_args.size()));
        return false;
    }
    // check if there are named arguments (not allowed)
    if (!checkNamedArgs(statement, false))
    {
        DBG_ERROR(error_base + ": named arguments are not allowed in a '" + statement.keyword + "' statement");
        return false;
    }
    // check if all the args have the expected types
    vector<Token> extracted;
    size_t index = 0;
    for (const auto& [name, token] : statement.arguments)
    {
        if (expected_args[index] == FLOAT && token.type == INT)
        {
            Token t = token;
            t.type = FLOAT;
            t.f_value = static_cast<float>(token.i_value);
            extracted.push_back(t);
        }
        else if (token.type != expected_args[index])
        {
            DBG_ERROR(error_base + ": argument " + to_string(index) + " in a '" + statement.keyword + "' statement must be a " + typeToString(expected_args[index]));
            return false;
        }
        else
            extracted.push_back(token);
        ++index;
    }

    extracted_args = extracted;

    return true;
}

bool TokenReader::readStatementNamed(const Statement& statement, const bool children_allowed, const bool requires_identifier, const std::map<std::string, std::pair<TokenType, bool>>& expected_args, std::map<std::string, Token>& extracted_args, const std::string& error_base)
{
    // check if there are children
    if (!statement.children.empty() && !children_allowed)
    {
        DBG_ERROR(error_base + ": children are not allowed in a '" + statement.keyword + "' statement");
        return false;
    }
    // check if there is an identifier
    if (statement.identifier.empty() && requires_identifier)
    {
        DBG_ERROR(error_base + ": an identifier is required in a '" + statement.keyword + "' statement");
        return false;
    }
    // check if there are non-named arguments (not allowed)
    if (!checkNamedArgs(statement, true))
    {
        DBG_ERROR(error_base + ": only named arguments are allowed in a '" + statement.keyword + "' statement");
        return false;
    }
    // for each arg, check if it is present, throw error if it is not present and required, or if it is the wrong type
    // check for duplicate args, and unrecognised args
    for (const auto& [name, token] : statement.arguments)
    {
        auto it = expected_args.find(name);
        if (it == expected_args.end())
        {
            DBG_ERROR(error_base + ": invalid argument '" + name + "' in '" + statement.keyword + "' statement");
            return false;
        }
        auto is_found = extracted_args.find(name);
        if (is_found != extracted_args.end())
        {
            DBG_ERROR(error_base + ": duplicate argument '" + name + "' in '" + statement.keyword + "' statement");
            return false;
        }
        if (it->second.first == FLOAT && token.type == INT)
        {
            Token t = token;
            t.type = FLOAT;
            t.f_value = static_cast<float>(token.i_value);
            extracted_args[name] = t;
        }
        else if (token.type != it->second.first)
        {
            DBG_ERROR(error_base + ": argument '" + name + "' has wrong type for '" + statement.keyword + "' statement, must be a " + typeToString(it->second.first));
            return false;
        }
        else
            extracted_args[name] = token;
    }
    // check if all args are present
    for (const auto& [name, spec] : expected_args)
    {
        if (spec.second)
        {
            auto is_found = extracted_args.find(name);
            if (is_found == extracted_args.end())
            {
                DBG_ERROR(error_base + ": argument '" + name + "' is required for '" + statement.keyword + "' statement, must be a " + typeToString(spec.first));
                return false;
            }
        }
    }

    return true;
}

bool TokenReader::checkNamedArgs(const Statement& statement, bool named)
{
    for (const auto& [name, token] : statement.arguments)
    {
        if (name.empty() == named)
            return false;
    }
    return true;
}

glm::vec4 TokenReader::deserialiseVectorToken(const string& str, const size_t offset, const string& original_content)
{
    vector<float> values;
    
    try
    {
        size_t next_comma = -1;
        do
        {
            const size_t last_comma = next_comma + 1;
            next_comma = str.find(',', last_comma);
            values.push_back(stof(str.substr(last_comma, next_comma - last_comma)));
        } while (next_comma != string::npos);
    }
    catch (invalid_argument& e)
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
        value[static_cast<glm::length_t>(i)] = values[i];

    return value;
}

vector<pair<string, TokenReader::Token>> TokenReader::parseArguments(vector<Token>::const_iterator start, vector<Token>::const_iterator end, const string& original_content)
{
    auto current = start + 1;

    vector<pair<string, Token>> arguments;
    Token keyword_token(TEXT);
    int stage = 0;

    while (current != end)
    {
        if (current->type == WHITESPACE || current->type == COMMENT || current->type == NEWLINE)
        {
            ++current;
            continue;
        }

        switch (stage)
        {
        case 0: // looking for the argument identifier or value
            switch (current->type)
            {
            case TEXT:
                stage = 1;
                keyword_token = *current;
                break;
            case VECTOR:
            case STRING:
            case INT:
            case FLOAT:
            case IDENTIFIER:
                keyword_token = Token(TEXT);
                arguments.emplace_back("", *current);
                stage = 3;
                break;
            default:
                reportError("unexpected token", current->start_offset, original_content);
                return { };
            }
            break;
        case 1: // looking for the equals sign
            if (current->type == COMMA)
            {
                arguments.emplace_back("", keyword_token);
                keyword_token = Token(TEXT);
                stage = 0;
                break;
            }
            if (current->type == EQUALS)
                stage = 2;
            else
            {
                reportError("unexpected token", current->start_offset, original_content);
                return { };
            }
            break;
        case 2: // looking for the argument value
            switch (current->type)
            {
            case TEXT:
            case VECTOR:
            case STRING:
            case INT:
            case FLOAT:
            case IDENTIFIER:
                arguments.emplace_back(keyword_token.s_value, *current);
                keyword_token = Token(TEXT);
                stage = 3;
                break;
            default:
                reportError("unexpected token", current->start_offset, original_content);
                return { };
            }
            break;
        case 3: // looking for comma
            if (current->type == COMMA)
                stage = 0;
            else
            {
                reportError("unexpected token", current->start_offset, original_content);
                return { };
            }
            break;
        }

        ++current;
    }

    return arguments;
}

size_t TokenReader::reportError(const string& err, const size_t off, const string& str)
{
    int32_t extract_start = max(0, static_cast<int32_t>(off) - 16);
    int32_t extract_end = extract_start + 32;
    while (true)
    {
        const size_t find = str.find('\n', extract_start);
        if (find >= off) break;
        extract_start = static_cast<int32_t>(find) + 1;
    }
    const size_t find = str.find('\n', off);
    if (find != string::npos)
        extract_end = std::min(static_cast<int32_t>(find), extract_end);
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

    const string error = format("token document parsing error: {}"
                        "\n\t-> '... {} ...'"
                        "\n\t->{}      ^ here (ln {}, col {})", err, extract, string(static_cast<int32_t>(off) - extract_start, ' '), ln + 1, col + 1);
    DBG_ERROR(error);
    return -1;
}
