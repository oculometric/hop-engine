#pragma once

#include <string>

// automatically generates deleters for basic constructors, and copy/move constructors/operators.
// these should be used for any engine type which should be handled exclusively as a pointer.

#define DELETE_CONSTRUCTORS(name) name () = delete;\
    name (const name & other) = delete;\
    name (name && other) = delete;\
    name & operator=(const name & other) = delete;\
    name & operator=(name && other) = delete

#define DELETE_NOT_ALL_CONSTRUCTORS(name) name (const name & other) = delete;\
    name (name && other) = delete;\
    name & operator=(const name & other) = delete;\
    name & operator=(name && other) = delete

#include "hop_forward.h"
#include "counted_ref.h"
#include "debug.h"

/**
 * @brief base class for all engine objects which are intended to be managed by the reference
 * system, ensuring that their destructors are called.
 */
class Destructible
{
public:
    virtual ~Destructible() { }
};

typedef void* GPUHandle;

/**
 * @brief execute a system command and capture the output.
 * @param command command to execute.
 * @param output destination for the command output.
 * @return result code from the command.
 */
int exec(const std::string& command, std::string& output);

#define TO_STRING_DEC(t) std::string to_string(t value)
#define VARGS(...) __VA_ARGS__
#define TO_STRING_DEF_BITFLAGS(t, s, n) string HopEngine::to_string(const t value) \
{ \
    constexpr const char* names[s] = \
    { n }; \
    string result; \
    for (size_t i = 0; i < s; ++i) \
    { \
        if (value & (1 << i)) \
        { \
            result += names[i]; \
            result += " | "; \
        } \
    } \
    result.pop_back(); \
    result.pop_back(); \
    result.pop_back(); \
    return result; \
}
#define TO_STRING_DEF(t, s, n) string HopEngine::to_string(const t value) \
{ \
    constexpr const char* names[s] = \
    { n }; \
    return names[value]; \
}

#define ENUM_OPERATOR(t) inline t operator|(t a, t b) { return (t)((uint32_t)a | (uint32_t)b); }
