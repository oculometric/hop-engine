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
            else if (char_type == TEXT)
            {
                new_type = TEXT;
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
                    arg_end_it = tokens.begin() + findClosingBrace(tokens, current_it - tokens.begin(), original_content);
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
                    children_end_it = tokens.begin() + findClosingBrace(tokens, current_it - tokens.begin(), original_content);
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
                else
                {
                    reportError("unexpected token", current_it->start_offset, original_content);
                    return { };
                }
                break;
            default:
                reportError("unexpected token", current_it->start_offset, original_content);
                return { };
                break;
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

bool TokenReader::readStatement(const Statement& statement, bool children_allowed, bool requires_identifier, const vector<TokenType> expected_args, vector<Token>& extracted_args, string error_base)
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
    for (const auto& arg : statement.arguments)
    {
        if (arg.second.type != expected_args[index])
        {
            DBG_ERROR(error_base + ": argument " + to_string(index) + " in a '" + statement.keyword + "' statement must be a " + typeToString(expected_args[index]));
            return false;
        }
        extracted.push_back(arg.second);
        ++index;
    }

    extracted_args = extracted;

    return true;
}

bool TokenReader::readStatement(const Statement& statement, bool children_allowed, bool requires_identifier, const std::map<std::string, std::pair<TokenType, bool>> expected_args, std::map<std::string, Token>& extracted_args, std::string error_base)
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
    for (const auto& arg : statement.arguments)
    {
        auto it = expected_args.find(arg.first);
        if (it == expected_args.end())
        {
            DBG_ERROR(error_base + ": invalid argument '" + arg.first + "' in '" + statement.keyword + "' statement");
            return false;
        }
        auto is_found = extracted_args.find(arg.first);
        if (is_found != extracted_args.end())
        {
            DBG_ERROR(error_base + ": duplicate argument '" + arg.first + "' in '" + statement.keyword + "' statement");
            return false;
        }
        if (arg.second.type != it->second.first)
        {
            DBG_ERROR(error_base + ": argument '" + arg.first + "' has wrong type for '" + statement.keyword + "' statement, must be a " + typeToString(it->second.first));
            return false;
        }
        extracted_args[arg.first] = arg.second;
    }
    // check if all args are present
    for (const auto& expected : expected_args)
    {
        if (expected.second.second)
        {
            auto is_found = extracted_args.find(expected.first);
            if (is_found == extracted_args.end())
            {
                DBG_ERROR(error_base + ": argument '" + expected.first + "' is required for '" + statement.keyword + "' statement, must be a " + typeToString(expected.second.first));
                return false;
            }
        }
    }

    return true;
}

bool TokenReader::checkNamedArgs(const Statement& statement, bool named)
{
    for (const auto& arg : statement.arguments)
    {
        if (arg.first.empty() == named)
            return false;
    }
    return true;
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

vector<pair<string, TokenReader::Token>> TokenReader::parseArguments(vector<Token>::const_iterator start, vector<Token>::const_iterator end, string original_content)
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
                arguments.push_back({ "", *current });
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
                arguments.push_back({ "", keyword_token });
                keyword_token = Token(TEXT);
                stage = 0;
            }
            else if (current->type == EQUALS)
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
                arguments.push_back({ keyword_token.s_value, *current });
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

size_t TokenReader::reportError(const string err, size_t off, const string& str)
{
    int32_t extract_start = max(0, (int32_t)off - 16);
    int32_t extract_end = extract_start + 32;
    while (true)
    {
        size_t find = str.find('\n', extract_start);
        if (find >= off) break;
        extract_start = (int32_t)find + 1;
    }
    size_t find = str.find('\n', off);
    if (find != string::npos)
    {
        if ((int32_t)find < extract_end)
            extract_end = (int32_t)find;
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
