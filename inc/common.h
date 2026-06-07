/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

constexpr uint16_t HOP_ENGINE_VERSION_MAJOR = 0;
constexpr uint16_t HOP_ENGINE_VERSION_MINOR = 56;
constexpr uint16_t HOP_ENGINE_VERSION       = (HOP_ENGINE_VERSION_MAJOR << 8) | HOP_ENGINE_VERSION_MINOR;
#define HOP_ENGINE_VERSION_STRING \
    (std::to_string(HOP_ENGINE_VERSION_MAJOR) + '.' + std::to_string(HOP_ENGINE_VERSION_MINOR))
#if !defined(HOP_ENGINE_COMMIT)
#define HOP_ENGINE_COMMIT UNKNOWN
#endif
#define STRINGIFY_(arg)          #arg
#define STRINGIFY(arg)           STRINGIFY_(arg)
#define HOP_ENGINE_COMMIT_STRING STRINGIFY(HOP_ENGINE_COMMIT)

// automatically generates deleters for basic constructors, and copy/move constructors/operators.
// these should be used for any engine type which should be handled exclusively as a pointer (i.e. a
// Ref<>).

#define DELETE_CONSTRUCTORS(name)                \
    name()                             = delete; \
    name(const name& other)            = delete; \
    name(name&& other)                 = delete; \
    name& operator=(const name& other) = delete; \
    name& operator=(name&& other)      = delete

#define DELETE_NOT_ALL_CONSTRUCTORS(name)        \
    name(const name& other)            = delete; \
    name(name&& other)                 = delete; \
    name& operator=(const name& other) = delete; \
    name& operator=(name&& other)      = delete

namespace HopEngine
{

/**
 * @brief base class for all engine objects which are intended to be managed by the reference
 * system, ensuring that their destructors are called.
 */
class Destructible
{
public:
    virtual ~Destructible() {}
};

} // namespace HopEngine

/**
 * @brief execute a system command and capture the output.
 * @param command command to execute.
 * @param output destination for the command output.
 * @return result code from the command.
 */
int exec(const std::string& command, std::string& output);

// defines a to_string function for an enum type
#define TO_STRING_DECL(t) std::string to_string(t value)
#define VARGS(...)        __VA_ARGS__
// defines a function which converts a bitflag-style enum into a string
#define TO_STRING_IMPL_BITFLAGS(t, s, n)            \
    std::string HopEngine::to_string(const t value) \
    {                                               \
        constexpr const char* names[s] = { n };     \
        std::string result;                         \
        for (size_t i = 0; i < s; ++i)              \
        {                                           \
            if (value & (1 << i))                   \
            {                                       \
                result += names[i];                 \
                result += " | ";                    \
            }                                       \
        }                                           \
        result.pop_back();                          \
        result.pop_back();                          \
        result.pop_back();                          \
        return result;                              \
    }

// defines a function which converts a non-bitflag-style enum into a string
#define TO_STRING_IMPL(t, s, n)                     \
    std::string HopEngine::to_string(const t value) \
    {                                               \
        constexpr const char* names[s] = { n };     \
        return names[value];                        \
    }

// defines an operator which allows bitwise-or-ing two values of a custom type together
#define ENUM_OPERATOR(t) \
    inline t operator|(t a, t b) { return (t)((uint32_t)a | (uint32_t)b); }

#include "counted_ref.h"
#include "debug.h"
#include "hop_forward.h"
