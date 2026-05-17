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

#include <source_location>

#define PTR(ptr) HopEngine::Debug::pointerToString(ptr)

#if defined(_WIN32)
#define __FUNCTION_SIGNATURE__ __FUNCSIG__
#else
#define __FUNCTION_SIGNATURE__ __PRETTY_FUNCTION__
#endif

#define DBG_VERBOSE(str) HopEngine::Debug::write(str, HopEngine::Debug::DEBUG_VERBOSE)
#define DBG_INFO(str)    HopEngine::Debug::write(str, HopEngine::Debug::DEBUG_INFO)
#define DBG_WARNING(str) HopEngine::Debug::write(str, HopEngine::Debug::DEBUG_WARNING)
#define DBG_ERROR(str)   HopEngine::Debug::write(str, HopEngine::Debug::DEBUG_ERROR)
#define DBG_FAULT(str)   HopEngine::Debug::write(str, HopEngine::Debug::DEBUG_FAULT)

#include "common.h"

#include <deque>
#include <fstream>
#include <string>
#include <vector>

namespace HopEngine
{

/**
 * @brief singleton class encapsulating logging functionality.
 */
class Debug final
{
    friend class InitMachine;

public:
    /**
     * @brief enum which describes the severity of a debug output call.
     */
    enum Level
    {
        DEBUG_VERBOSE,
        DEBUG_INFO,
        DEBUG_WARNING,
        DEBUG_ERROR,
        DEBUG_FAULT
    };

private:
    Level log_level   = DEBUG_INFO;        // severity needed before debug is sent to the log/terminal
    Level crash_level = DEBUG_FAULT;       // severity which triggers a program crash
    std::ofstream file_output;             // file pointer to the log file
    std::deque<std::string> lines_history; // history of calls to `write`

public:
    DELETE_NOT_ALL_CONSTRUCTORS(Debug);

    /**
     * @brief utility function converting any pointer type into a hex string.
     * @param ptr pointer to convert, can be any type.
     * @return a string with a hexadecimal description of the pointer.
     */
    static std::string pointerToString(const void* ptr);
    /**
     * @brief set the minimum severity level before `write()` calls will be printed to the
     * terminal/logfile.
     * @param severity new severity to be used from now on.
     */
    static void setLogLevel(Level severity);
    /**
     * @brief set the maximum severity level at which `write()` calls will cause the application to
     * exit.
     * @param severity new severity to be used from now on.
     */
    static void setCrashLevel(Level severity);
    /**
     * @brief sends a line of debug output to the console, and the logfile. the
     * line is prepended with a severity tag based on the given severity. this call
     * will only proceed if the specified `severity` is equal to or greater than
     * the `log_level` field (specified by `setLogLevel`). if `severity` is equal to or greater than
     * `crash level`, this call will cause the program to exit.
     * @param description text to be printed.
     * @param severity severity of the debug command. higher means worse.
     * @param location place in the codebase where the error ocurred
     */
    static void write(const std::string& description, Level severity,
        const std::source_location& location = std::source_location::current());
    /**
     * @brief force flush output to file, useful for circumstances where the program crashes.
     */
    static void flush();
    /**
     * @brief retrieves the last `count` lines of debug output.
     * @param count number of lines to retrieve (newlines within debug messages are ignored, so really
     * this is the last `count` calls to `write`).
     * @returns array of lines, individual lines may contain newlines.
     */
    static std::vector<std::string> queryLines(size_t count);

private:
    Debug(Level crash, bool create_file);
    ~Debug();

    static void init(bool create_file);
    static void close();
};

} // namespace HopEngine
