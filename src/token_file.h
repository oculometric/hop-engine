#pragma once

#include <string>
#include <vector>
#include <glm/vec4.hpp>
#include <map>

#include "common.h"

namespace HopEngine
{

class TokenReader
{
public:
    enum TokenType
    {
        TEXT,
        OPEN_ROUND,
        CLOSE_ROUND,
        OPEN_CURLY,
        CLOSE_CURLY,
        NEWLINE,
        COLON,
        SEMICOLON,
        STRING,
        INT,
        FLOAT,
        COMMA,
        VECTOR,
        END_VECTOR,
        EQUALS,
        IDENTIFIER,
        COMMENT,
        INVALID,
        WHITESPACE
    };

    struct Token
    {
        TokenType type = VECTOR;
        union
        {
            glm::vec4 c_value = { 0, 0, 0, 0 };
            int i_value;
            float f_value;
        };
        std::string s_value = "";
        size_t start_offset = 0;

        inline Token()
        {
            i_value = 0;
            type = INT;
        }

        inline Token(TokenType ttype)
        {
            type = ttype;
        }

        inline Token(const Token& other)
        {
            type = other.type;
            start_offset = other.start_offset;

            switch (type)
            {
            case TEXT:
            case STRING:
            case IDENTIFIER:
            case COMMENT:
                s_value = other.s_value;
                break;
            case VECTOR:
                c_value = other.c_value;
                break;
            case INT:
                i_value = other.i_value;
                break;
            case FLOAT:
                f_value = other.f_value;
                break;
            default:
                break;
            }
        }

        inline Token operator=(const Token& other)
        {
            type = other.type;
            start_offset = other.start_offset;

            switch (type)
            {
            case IDENTIFIER:
            case TEXT:
            case STRING:
            case COMMENT:
                s_value = other.s_value;
                break;
            case VECTOR:
                c_value = other.c_value;
                break;
            case INT:
                i_value = other.i_value;
                break;
            case FLOAT:
                f_value = other.f_value;
                break;
            default:
                break;
            }

            return *this;
        }

        inline Token operator=(Token&& other) noexcept
        {
            type = other.type;
            start_offset = other.start_offset;

            switch (type)
            {
            case IDENTIFIER:
            case TEXT:
            case STRING:
            case COMMENT:
                s_value = other.s_value;
                break;
            case VECTOR:
                c_value = other.c_value;
                break;
            case INT:
                i_value = other.i_value;
                break;
            case FLOAT:
                f_value = other.f_value;
                break;
            default:
                break;
            }

            return *this;
        }
    };

    struct Statement
    {
        std::string keyword;
        std::string identifier;
        std::vector<std::pair<std::string, Token>> arguments;
        std::vector<Statement> children;
    };

public:
    DELETE_CONSTRUCTORS(TokenReader);

    static std::vector<Token> tokenise(const std::string& content, bool trim_comments = true, bool trim_whitespace = true);
    static size_t findClosingBrace(const std::vector<Token>& tokens, size_t open_index, const std::string& original_content);
    static std::vector<Statement> extractSyntaxTree(const std::vector<Token>& tokens, const std::string& original_content);

    static bool readStatement(const Statement& statement, bool children_allowed, bool requires_identifier, const std::vector<TokenType> expected_args, std::vector<Token>& extracted_args, std::string error_base);
    static bool readStatement(const Statement& statement, bool children_allowed, bool requires_identifier, const std::map<std::string, std::pair<TokenType, bool>> expected_args, std::map<std::string, Token>& extracted_args, std::string error_base);
    static bool checkNamedArgs(const Statement& statement, bool named);

private:
    static inline bool isAlphabetic(const char c)
    {
        if (c >= 'a' && c <= 'z') return true;
        if (c >= 'A' && c <= 'Z') return true;
        if (c == '_') return true;

        return false;
    }

    static inline bool isSeparator(TokenType t)
    {
        switch (t)
        {
        case TEXT:
        case STRING:
        case INT:
        case IDENTIFIER:
        case END_VECTOR:
            return false;
        default:
            return true;
        }
    }

    static inline TokenType getType(const char c)
    {
        if (isAlphabetic(c)) return TEXT;
        if (c == '-' || (c >= '0' && c <= '9')) return INT;
        switch (c)
        {
        case '@': return IDENTIFIER;
        case '(': return OPEN_ROUND;
        case ')': return CLOSE_ROUND;
        case '{': return OPEN_CURLY;
        case '}': return CLOSE_CURLY;
        case '\"': return STRING;
        case '\r':
        case '\n':
            return NEWLINE;
        case ':': return COLON;
        case '=': return EQUALS;
        case ',': return COMMA;
        case '[': return VECTOR;
        case ']': return END_VECTOR;
        case '.': return FLOAT;
        case '/': return COMMENT;
        case ';': return SEMICOLON;
        case ' ':
        case '\t':
            return WHITESPACE;
        }
        return INVALID;
    }

    static inline std::string typeToString(TokenType t)
    {
        switch (t)
        {
        case TEXT:
            return "keyword";
        case STRING:
            return "string";
        case INT:
            return "int";
        case IDENTIFIER:
            return "identifier";
        case VECTOR:
            return "vector";
        case FLOAT:
            return "float";
        default:
            return "";
        }
    }

    static glm::vec4 deserialiseVectorToken(std::string str, size_t offset, const std::string& original_content);
    static std::vector<std::pair<std::string, HopEngine::TokenReader::Token>> parseArguments(std::vector<Token>::const_iterator start, std::vector<Token>::const_iterator end, std::string original_content);

    static size_t reportError(const std::string err, size_t off, const std::string& str);
};

}
