#pragma once

#include <string>
#include <vector>
#include <glm/vec4.hpp>

#include "common.h"

namespace HopEngine
{

class TokenReader
{
private:
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
        TokenType type;
        union
        {
            std::string s_value = "";
            int i_value;
            float f_value;
            glm::vec4 c_value;
        };
        size_t start_offset = 0;

        inline Token(TokenType ttype)
        {
            type = ttype;
            start_offset = 0;

            switch (ttype)
            {
            case TEXT:
            case STRING:
            case IDENTIFIER:
            case COMMENT:
                s_value = "";
                break;
            case VECTOR:
                c_value = { 0, 0, 0, 0 };
                break;
            case INT:
                i_value = 0;
                break;
            case FLOAT:
                f_value = 0.0f;
                break;
            default:
                break;
            }
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

        inline ~Token()
        {
            switch (type)
            {
            case TEXT:
            case STRING:
            case IDENTIFIER:
            case COMMENT:
                s_value.~basic_string();
                break;
            default:
                break;
            };
        }
    };

public:
    DELETE_CONSTRUCTORS(TokenReader);

    static std::vector<Token> tokenise(const std::string& content);
    static size_t findClosingBrace(const std::vector<Token>& tokens, size_t open_index, const std::string& original_content);

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

    static glm::vec4 deserialiseVectorToken(std::string str, size_t offset, const std::string& original_content);

    static size_t reportError(const std::string err, size_t off, const std::string& str);
};

}
