#include "deserialise.h"

using namespace HopEngine;

size_t Deserialiser::AnonymousStatementResult::offsetOf(size_t index)
{ return arguments[index].start_offset; }

void Deserialiser::AnonymousStatementResult::read(size_t index, float& destination) const
{
    if (arguments[index].type == TokenReader::TOKEN_FLOAT) destination = arguments[index].f_value;
    else
        destination = static_cast<float>(arguments[index].i_value);
}

void Deserialiser::AnonymousStatementResult::read(size_t index, int& destination) const
{
    if (arguments[index].type == TokenReader::TOKEN_FLOAT)
        destination = static_cast<int>(arguments[index].f_value);
    else
        destination = arguments[index].i_value;
}

void Deserialiser::AnonymousStatementResult::read(size_t index, uint32_t& destination) const
{
    if (arguments[index].type == TokenReader::TOKEN_FLOAT)
        destination = static_cast<uint32_t>(arguments[index].f_value);
    else
        destination = static_cast<uint32_t>(arguments[index].i_value);
}

void Deserialiser::AnonymousStatementResult::read(size_t index, glm::vec2& destination) const
{ destination = arguments[index].c_value; }

void Deserialiser::AnonymousStatementResult::read(size_t index, glm::ivec2& destination) const
{ destination = arguments[index].c_value; }

void Deserialiser::AnonymousStatementResult::read(size_t index, glm::vec3& destination) const
{ destination = arguments[index].c_value; }

void Deserialiser::AnonymousStatementResult::read(size_t index, glm::ivec3& destination) const
{ destination = arguments[index].c_value; }

void Deserialiser::AnonymousStatementResult::read(size_t index, glm::vec4& destination) const
{ destination = arguments[index].c_value; }

void Deserialiser::AnonymousStatementResult::read(size_t index, std::string& destination) const
{ destination = arguments[index].s_value; }

bool Deserialiser::AnonymousStatementResult::read(size_t index, bool& destination) const
{
    if (arguments[index].s_value == "TRUE") destination = true;
    else if (arguments[index].s_value == "FALSE")
        destination = false;
    else
        return false;
    return true;
}

size_t Deserialiser::NamedStatementResult::offsetOf(const std::string& name)
{
    auto it = arguments.find(name);
    if (it == arguments.end()) return 0;

    return it->second.start_offset;
}

void Deserialiser::NamedStatementResult::read(const std::string& name, float& destination) const
{
    auto it = arguments.find(name);
    if (it == arguments.end()) return;

    if (it->second.type == TokenReader::TOKEN_FLOAT) destination = it->second.f_value;
    else
        destination = static_cast<float>(it->second.i_value);
}

void Deserialiser::NamedStatementResult::read(const std::string& name, int& destination) const
{
    auto it = arguments.find(name);
    if (it == arguments.end()) return;

    if (it->second.type == TokenReader::TOKEN_FLOAT) destination = static_cast<int>(it->second.f_value);
    else
        destination = it->second.i_value;
}

void Deserialiser::NamedStatementResult::read(const std::string& name, uint32_t& destination) const
{
    auto it = arguments.find(name);
    if (it == arguments.end()) return;

    if (it->second.type == TokenReader::TOKEN_FLOAT)
        destination = static_cast<uint32_t>(it->second.f_value);
    else
        destination = static_cast<uint32_t>(it->second.i_value);
}

void Deserialiser::NamedStatementResult::read(const std::string& name, glm::vec2& destination) const
{
    auto it = arguments.find(name);
    if (it != arguments.end()) destination = it->second.c_value;
}

void Deserialiser::NamedStatementResult::read(const std::string& name, glm::ivec2& destination) const
{
    auto it = arguments.find(name);
    if (it != arguments.end()) destination = it->second.c_value;
}

void Deserialiser::NamedStatementResult::read(const std::string& name, glm::vec3& destination) const
{
    auto it = arguments.find(name);
    if (it != arguments.end()) destination = it->second.c_value;
}

void Deserialiser::NamedStatementResult::read(const std::string& name, glm::ivec3& destination) const
{
    auto it = arguments.find(name);
    if (it != arguments.end()) destination = it->second.c_value;
}

void Deserialiser::NamedStatementResult::read(const std::string& name, glm::vec4& destination) const
{
    auto it = arguments.find(name);
    if (it != arguments.end()) destination = it->second.c_value;
}

void Deserialiser::NamedStatementResult::read(const std::string& name, std::string& destination) const
{
    auto it = arguments.find(name);
    if (it != arguments.end()) destination = it->second.s_value;
}

bool Deserialiser::NamedStatementResult::read(const std::string& name, bool& destination) const
{
    auto it = arguments.find(name);
    if (it == arguments.end()) return true;

    if (it->second.s_value == "TRUE") destination = true;
    else if (it->second.s_value == "FALSE")
        destination = false;
    else
        return false;
    return true;
}

bool Deserialiser::execute(const std::vector<TokenReader::Statement>& statements,
    const std::string& token_str)
{
    std::vector<AnonymousStatementResult> anonymous_results;
    std::vector<NamedStatementResult> named_results;
    std::vector<std::pair<bool, size_t>> results_ordered;

    anonymous_results.reserve(statements.size());
    named_results.reserve(statements.size());
    for (const auto& statement : statements)
    {
        auto it1 = anonymous_statement_types.find(statement.keyword);
        if (it1 != anonymous_statement_types.end())
        {
            std::vector<TokenReader::Token> arguments;
            if (!errorCheckAnonymous(statement, it1->second.first, arguments, token_str)) return false;
            results_ordered.emplace_back(false, anonymous_results.size());
            anonymous_results.emplace_back(statement, arguments);
            continue;
        }

        auto it2 = named_statement_types.find(statement.keyword);
        if (it2 != named_statement_types.end())
        {
            std::map<std::string, TokenReader::Token> arguments;
            if (!errorCheckNamed(statement, it2->second.first, arguments, token_str)) return false;
            results_ordered.emplace_back(true, named_results.size());
            named_results.emplace_back(statement, arguments);
            continue;
        }

        emitError("unable to deserialise unknown '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }

    for (const auto& pair : results_ordered)
    {
        if (!pair.first)
        {
            const auto& result = anonymous_results[pair.second];
            if (!anonymous_statement_types[result.statement.keyword].second(result)) return false;
        }
        else
        {
            const auto& result = named_results[pair.second];
            if (!named_statement_types[result.statement.keyword].second(result)) return false;
        }
    }

    return true;
}

bool Deserialiser::emitError(const std::string& _error)
{
    DBG_ERROR(error + ": " + _error);
    return false;
}

bool Deserialiser::emitError(const std::string& _error, size_t offset, const std::string& token_str)
{
    DBG_ERROR(error + ": " + _error + TokenReader::generateError(offset, token_str));
    return false;
}

static bool checkNamedArgs(const TokenReader::Statement& statement, bool named)
{
    for (const auto& [name, token] : statement.arguments)
    {
        if (name.empty() == named) return false;
    }
    return true;
}

bool Deserialiser::errorCheckAnonymous(const TokenReader::Statement& statement,
    const AnonymousStatementSpec& spec, std::vector<TokenReader::Token>& output,
    const std::string& token_str)
{
    // check if there are children
    if (!statement.children.empty() && !spec.children_permitted)
    {
        emitError("children are not allowed in a '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }
    // check if there is an identifier
    if (statement.identifier.empty() &&
        spec.identifier_allowed == Deserialiser::STATEMENT_IDENTIFIER_REQUIRED)
    {
        emitError("an identifier is required in a '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }
    if (!statement.identifier.empty() &&
        spec.identifier_allowed == Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN)
    {
        emitError("an identifier is forbidden in a '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }
    // check if enough args are present
    if (statement.arguments.size() > spec.expected_anon_args.size())
    {
        emitError("too many arguments in '" + statement.keyword + "' statement, requires " +
                      std::to_string(spec.expected_anon_args.size()),
            statement.start_offset, token_str);
        return false;
    }
    if (statement.arguments.size() < spec.expected_anon_args.size())
    {
        emitError("not enough arguments in '" + statement.keyword + "' statement, requires " +
                      std::to_string(spec.expected_anon_args.size()),
            statement.start_offset, token_str);
        return false;
    }
    // check if there are named arguments (not allowed)
    if (!checkNamedArgs(statement, false))
    {
        emitError("named arguments are not allowed in a '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }
    // check if all the args have the expected types
    output.clear();
    size_t index = 0;
    for (const auto& [name, token] : statement.arguments)
    {
        if (spec.expected_anon_args[index] == TokenReader::TOKEN_FLOAT &&
            token.type == TokenReader::TOKEN_INT)
        {
            TokenReader::Token t = token;
            t.type               = TokenReader::TOKEN_FLOAT;
            t.f_value            = static_cast<float>(token.i_value);
            output.push_back(t);
        }
        else if (spec.expected_anon_args[index] == TokenReader::TOKEN_INT &&
                 token.type == TokenReader::TOKEN_FLOAT)
        {
            TokenReader::Token t = token;
            t.type               = TokenReader::TOKEN_INT;
            t.i_value            = static_cast<int>(token.f_value);
            output.push_back(t);
        }
        else if (token.type != spec.expected_anon_args[index])
        {
            emitError("argument " + std::to_string(index) + " in a '" + statement.keyword +
                          "' statement must be a " +
                          TokenReader::typeToString(spec.expected_anon_args[index]),
                token.start_offset, token_str);
            return false;
        }
        else
            output.push_back(token);
        ++index;
    }

    return true;
}

bool Deserialiser::errorCheckNamed(const TokenReader::Statement& statement,
    const NamedStatementSpec& spec, std::map<std::string, TokenReader::Token>& output,
    const std::string& token_str)
{
    // check if there are children
    if (!statement.children.empty() && !spec.children_permitted)
    {
        emitError("children are not allowed in a '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }
    // check if there is an identifier
    if (statement.identifier.empty() &&
        spec.identifier_allowed == Deserialiser::STATEMENT_IDENTIFIER_REQUIRED)
    {
        emitError("an identifier is required in a '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }
    if (!statement.identifier.empty() &&
        spec.identifier_allowed == Deserialiser::STATEMENT_IDENTIFIER_FORBIDDEN)
    {
        emitError("an identifier is forbidden in a '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }
    // check if there are non-named arguments (not allowed)
    if (!checkNamedArgs(statement, true))
    {
        emitError("only named arguments are allowed in a '" + statement.keyword + "' statement",
            statement.start_offset, token_str);
        return false;
    }
    // for each arg, check if it is present, throw error if it is not present and required, or if it is
    // the wrong type check for duplicate args, and unrecognised args
    output.clear();
    for (const auto& [name, token] : statement.arguments)
    {
        auto it = spec.expected_named_args.find(name);
        if (it == spec.expected_named_args.end())
        {
            emitError("unrecognised argument '" + name + "' in '" + statement.keyword + "' statement",
                token.start_offset, token_str);
            return false;
        }
        auto is_found = output.find(name);
        if (is_found != output.end())
        {
            emitError("duplicate argument '" + name + "' in '" + statement.keyword + "' statement",
                token.start_offset, token_str);
            return false;
        }
        if (it->second.first == TokenReader::TOKEN_FLOAT && token.type == TokenReader::TOKEN_INT)
        {
            TokenReader::Token t = token;
            t.type               = TokenReader::TOKEN_FLOAT;
            t.f_value            = static_cast<float>(token.i_value);
            output[name]         = t;
        }
        else if (it->second.first == TokenReader::TOKEN_INT && token.type == TokenReader::TOKEN_FLOAT)
        {
            TokenReader::Token t = token;
            t.type               = TokenReader::TOKEN_INT;
            t.i_value            = static_cast<int>(token.f_value);
            output[name]         = t;
        }
        else if (token.type != it->second.first)
        {
            emitError("argument '" + name + "' has wrong type for '" + statement.keyword +
                          "' statement, must be a " + TokenReader::typeToString(it->second.first),
                token.start_offset, token_str);
            return false;
        }
        else
            output[name] = token;
    }
    // check if all args are present
    for (const auto& [name, conf] : spec.expected_named_args)
    {
        if (conf.second)
        {
            auto is_found = output.find(name);
            if (is_found == output.end())
            {
                emitError("argument '" + name + "' is required for '" + statement.keyword +
                              "' statement, must be a(n) " + TokenReader::typeToString(conf.first),
                    statement.start_offset, token_str);
                return false;
            }
        }
    }

    return true;
}
