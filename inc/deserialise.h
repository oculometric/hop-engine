#pragma once

#include "common.h"

#include <functional>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

namespace HopEngine
{

/**
 * @brief static class which provides functionality to tokenise, parse, and process an abstract syntax
 * tree.
 */
class TokenReader final
{
public:
    /**
     * @brief enumerates all possible types of tokens found in the text representation of the syntax
     * tree.
     */
    enum TokenType : uint8_t
    {
        TOKEN_TEXT,
        TOKEN_STRING,
        TOKEN_INT,
        TOKEN_FLOAT,
        TOKEN_VECTOR,
        TOKEN_IDENTIFIER
    };

    enum TokenTypeInternal : uint8_t
    {
        TOKEN_INTERNAL_TEXT       = TOKEN_TEXT,
        TOKEN_INTERNAL_STRING     = TOKEN_STRING,
        TOKEN_INTERNAL_INT        = TOKEN_INT,
        TOKEN_INTERNAL_FLOAT      = TOKEN_FLOAT,
        TOKEN_INTERNAL_VECTOR     = TOKEN_VECTOR,
        TOKEN_INTERNAL_IDENTIFIER = TOKEN_IDENTIFIER,
        TOKEN_INTERNAL_OPEN_ROUND,
        TOKEN_INTERNAL_CLOSE_ROUND,
        TOKEN_INTERNAL_OPEN_CURLY,
        TOKEN_INTERNAL_CLOSE_CURLY,
        TOKEN_INTERNAL_NEWLINE,
        TOKEN_INTERNAL_COLON,
        TOKEN_INTERNAL_SEMICOLON,
        TOKEN_INTERNAL_COMMA,
        TOKEN_INTERNAL_END_VECTOR,
        TOKEN_INTERNAL_EQUALS,
        TOKEN_INTERNAL_COMMENT,
        TOKEN_INTERNAL_INVALID,
        TOKEN_INTERNAL_WHITESPACE
    };

    struct Token final
    {
        TokenType type = TOKEN_INT;
        union
        {
            glm::vec4 c_value = { 0, 0, 0, 0 };
            int i_value;
            float f_value;
        };
        std::string s_value = "";
        size_t start_offset = 0;

        Token() {}

        Token(const TokenType _type) { type = _type; }
        Token(const Token& other);
        Token operator=(const Token& other);
        Token operator=(Token&& other) noexcept;
    };

    struct Statement final
    {
        std::string keyword;
        std::string identifier;
        std::vector<std::pair<std::string, Token>> arguments;
        std::vector<Statement> children;
        size_t start_offset = 0;
    };

public:
    DELETE_CONSTRUCTORS(TokenReader);

    static std::vector<Token> tokenise(const std::string& content, bool trim_comments = true,
        bool trim_whitespace = true);
    static std::vector<Token>::const_iterator findClosingBrace(const std::vector<Token>& tokens,
        const std::vector<Token>::const_iterator open_index, const std::string& original_content);
    static std::vector<Statement> extractSyntaxTree(const std::vector<Token>& tokens,
        const std::string& original_content);

    static constexpr std::string typeToString(const TokenType t)
    {
        switch (t)
        {
        case TOKEN_TEXT:       return "keyword";
        case TOKEN_STRING:     return "string";
        case TOKEN_INT:        return "int";
        case TOKEN_FLOAT:      return "float";
        case TOKEN_VECTOR:     return "vector";
        case TOKEN_IDENTIFIER: return "identifier";
        default:               return "";
        }
    }

    static std::string generateError(size_t offset, const std::string& str);

private:
    static bool isAlphabetic(const char c)
    {
        if (c >= 'a' && c <= 'z') return true;
        if (c >= 'A' && c <= 'Z') return true;
        if (c == '_') return true;
        return false;
    }

    static constexpr bool isSeparator(const TokenTypeInternal t)
    {
        switch (t)
        {
        case TOKEN_INTERNAL_TEXT:
        case TOKEN_INTERNAL_STRING:
        case TOKEN_INTERNAL_INT:
        case TOKEN_INTERNAL_IDENTIFIER:
        case TOKEN_INTERNAL_END_VECTOR: return false;
        default:                        return true;
        }
    }

    static TokenTypeInternal getType(const char c)
    {
        if (isAlphabetic(c)) return TOKEN_INTERNAL_TEXT;
        if (c == '-' || (c >= '0' && c <= '9')) return TOKEN_INTERNAL_INT;
        switch (c)
        {
        case '@':  return TOKEN_INTERNAL_IDENTIFIER;
        case '(':  return TOKEN_INTERNAL_OPEN_ROUND;
        case ')':  return TOKEN_INTERNAL_CLOSE_ROUND;
        case '{':  return TOKEN_INTERNAL_OPEN_CURLY;
        case '}':  return TOKEN_INTERNAL_CLOSE_CURLY;
        case '\"': return TOKEN_INTERNAL_STRING;
        case '\r':
        case '\n': return TOKEN_INTERNAL_NEWLINE;
        case ':':  return TOKEN_INTERNAL_COLON;
        case '=':  return TOKEN_INTERNAL_EQUALS;
        case ',':  return TOKEN_INTERNAL_COMMA;
        case '[':  return TOKEN_INTERNAL_VECTOR;
        case ']':  return TOKEN_INTERNAL_END_VECTOR;
        case '.':  return TOKEN_INTERNAL_FLOAT;
        case '/':  return TOKEN_INTERNAL_COMMENT;
        case ';':  return TOKEN_INTERNAL_SEMICOLON;
        case ' ':
        case '\t': return TOKEN_INTERNAL_WHITESPACE;
        default:   return TOKEN_INTERNAL_INVALID;
        }
    }

    static glm::vec4 deserialiseVectorToken(const std::string& str, size_t offset,
        const std::string& original_content);
    static std::vector<std::pair<std::string, Token>> parseArguments(
        std::vector<Token>::const_iterator start, std::vector<Token>::const_iterator end,
        const std::string& original_content);

    static size_t reportError(const std::string& err, size_t off, const std::string& str);
};

class Deserialiser final
{
public:
    enum StatementIdentifierStatus
    {
        STATEMENT_IDENTIFIER_FORBIDDEN,
        STATEMENT_IDENTIFIER_OPTIONAL,
        STATEMENT_IDENTIFIER_REQUIRED
    };

    struct StatementSpec
    {
        std::string keyword_name;
        StatementIdentifierStatus identifier_allowed;
        bool children_permitted;

        StatementSpec(std::string keyword, StatementIdentifierStatus identifier, bool children) :
            keyword_name(keyword), identifier_allowed(identifier), children_permitted(children)
        {
        }
        StatementSpec() :
            keyword_name(""), identifier_allowed(STATEMENT_IDENTIFIER_OPTIONAL),
            children_permitted(true)
        {
        }
    };

    struct AnonymousStatementSpec final : public StatementSpec
    {
        std::vector<TokenReader::TokenType> expected_anon_args;

        using StatementSpec::StatementSpec;
        AnonymousStatementSpec& argument(TokenReader::TokenType type)
        {
            expected_anon_args.push_back(type);
            return *this;
        }
    };

    struct NamedStatementSpec final : public StatementSpec
    {
        std::map<std::string, std::pair<TokenReader::TokenType, bool>> expected_named_args;

        using StatementSpec::StatementSpec;
        NamedStatementSpec& argument(std::string name, TokenReader::TokenType type, bool required)
        {
            expected_named_args[name] = { type, required };
            return *this;
        }
    };

    struct AnonymousStatementResult
    {
        TokenReader::Statement statement;
        std::vector<TokenReader::Token> arguments;

        size_t offsetOf(size_t index);
        void read(size_t index, float& destination) const;
        void read(size_t index, int& destination) const;
        void read(size_t index, uint32_t& destination) const;
        void read(size_t index, glm::vec2& destination) const;
        void read(size_t index, glm::ivec2& destination) const;
        void read(size_t index, glm::vec3& destination) const;
        void read(size_t index, glm::ivec3& destination) const;
        void read(size_t index, glm::vec4& destination) const;
        void read(size_t index, std::string& destination) const;
        bool read(size_t index, bool& destination) const;
        template<typename T> bool read(size_t index, T& destination,
            std::function<bool(const std::string&, T&)> converter) const
        { return converter(arguments[index].s_value, destination); }
    };

    struct NamedStatementResult
    {
        TokenReader::Statement statement;
        std::map<std::string, TokenReader::Token> arguments;

        size_t offsetOf(const std::string& name);
        void read(const std::string& name, float& destination) const;
        void read(const std::string& name, int& destination) const;
        void read(const std::string& name, uint32_t& destination) const;
        void read(const std::string& name, glm::vec2& destination) const;
        void read(const std::string& name, glm::ivec2& destination) const;
        void read(const std::string& name, glm::vec3& destination) const;
        void read(const std::string& name, glm::ivec3& destination) const;
        void read(const std::string& name, glm::vec4& destination) const;
        void read(const std::string& name, std::string& destination) const;
        bool read(const std::string& name, bool& destination) const;
        template<typename T> bool read(const std::string& name, T& destination,
            std::function<bool(const std::string&, T&)> converter) const
        {
            std::string data;
            read(name, data);
            if (data.empty()) return true;
            else
                return converter(data, destination);
        }
    };

private:
    std::map<std::string,
        std::pair<AnonymousStatementSpec, std::function<bool(AnonymousStatementResult)>>>
        anonymous_statement_types;
    std::map<std::string, std::pair<NamedStatementSpec, std::function<bool(NamedStatementResult)>>>
        named_statement_types;

    std::string error;

public:
    DELETE_CONSTRUCTORS(Deserialiser);
    Deserialiser(std::string error_base) : error(error_base) {}
    ~Deserialiser() = default;

    void addStatementAnonymous(const AnonymousStatementSpec& spec,
        std::function<bool(AnonymousStatementResult)> handler)
    { anonymous_statement_types[spec.keyword_name] = { spec, handler }; }
    void addStatementNamed(const NamedStatementSpec& spec,
        std::function<bool(NamedStatementResult)> handler)
    { named_statement_types[spec.keyword_name] = { spec, handler }; }

    bool execute(const std::vector<TokenReader::Statement>& statements, const std::string& token_str);
    bool emitError(const std::string& error);
    bool emitError(const std::string& error, size_t offset, const std::string& token_str);

private:
    bool errorCheckAnonymous(const TokenReader::Statement& statement,
        const AnonymousStatementSpec& spec, std::vector<TokenReader::Token>& output, const std::string& token_str);
    bool errorCheckNamed(const TokenReader::Statement& statement, const NamedStatementSpec& spec,
        std::map<std::string, TokenReader::Token>& output, const std::string& token_str);
};

} // namespace HopEngine
