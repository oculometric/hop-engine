#include "debug.h"

#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>

#define DEBUG_TERMINAL std::cout
#define DEBUG_LOGFILE  "log/"

using namespace HopEngine;

void Debug::setLogLevel(const Level severity) { getInstance()->log_level = severity; }

void Debug::setCrashLevel(const Level severity) { getInstance()->crash_level = severity; }

// generates an ANSI colour command from the given foreground and background colours
static std::string makeANSIColour(const int fgcol, const int bgcol)
{ return "\033[" + std::to_string(fgcol + 30) + ';' + std::to_string(bgcol + 40) + 'm'; }

static std::string makeANSIColour(const int fgcol) { return "\033[" + std::to_string(fgcol + 30) + 'm'; }

std::string Debug::pointerToString(const void* ptr)
{ return std::format("0x{:x}", reinterpret_cast<size_t>(ptr)); }

void Debug::write(const std::string& description, Level severity, const std::source_location& location)
{
    if (!getInstance())
    {
        DEBUG_TERMINAL << description << std::endl;
        return;
    }

    if (severity < getInstance()->log_level) return;

    static const std::string bracket_col  = makeANSIColour(60);
    static const std::string standard_col = makeANSIColour(67);
    static const std::string time_col     = makeANSIColour(5);
    const std::filesystem::path file_path = location.file_name();
    const std::string file_str            = file_path.filename().generic_string();

    std::string type_col  = "";
    std::string log_type  = "";
    std::string next_line = "";
    // for the terminal output, use ANSI terminal colours to make the output more fun
    switch (severity)
    {
    case DEBUG_VERBOSE:
        log_type = "VERBOSE";
        type_col = makeANSIColour(64);
        break;
    case DEBUG_INFO:
        log_type = "INFO";
        type_col = makeANSIColour(6);
        break;
    case DEBUG_WARNING:
        log_type = "WARNING";
        type_col = makeANSIColour(63);
        break;
    case DEBUG_ERROR:
        log_type  = "ERROR";
        type_col  = makeANSIColour(61);
        next_line = " -> in " + file_str + ", ln " + std::to_string(location.line()) + "\n -> " +
                    location.function_name();
        break;
    case DEBUG_FAULT:
        log_type  = "FAULT";
        type_col  = makeANSIColour(1);
        next_line = " -> in " + file_str + ", ln " + std::to_string(location.line()) + "\n -> " +
                    location.function_name();
        break;
    }

    auto time_now = std::time(nullptr);
    tm time;
#if defined(_WIN32)
    (void)localtime_s(&time, &time_now);
#else
    auto tmp = localtime(&time_now);
    time     = *tmp;
#endif
    // generating a timestamp for the output
    std::string log_line  = std::format("[{: >8} ]: {:0>2}:{:0>2}:{:0>2} - {}", log_type, time.tm_hour,
        time.tm_min, time.tm_sec, description);
    std::string term_line = std::format("{}[{}{: >8} {}]{}: {}{:0>2}:{:0>2}:{:0>2}{} - {}", bracket_col,
        type_col, log_type, bracket_col, standard_col, time_col, time.tm_hour, time.tm_min, time.tm_sec,
        standard_col, description);
    if (!next_line.empty())
    {
        log_line.append("\n" + next_line);
        term_line.append("\n" + next_line);
    }

    size_t next = 0;
    while (next != std::string::npos)
    {
        size_t newline   = log_line.find('\n', next);
        std::string line = log_line.substr(next, newline - next);
        getInstance()->lines_history.push_back(log_line);
        if (getInstance()->lines_history.size() > 256) getInstance()->lines_history.pop_front();
        if (newline == std::string::npos) break;
        next = newline + 1;
    }
    if (getInstance()->file_output.is_open()) getInstance()->file_output << log_line << std::endl;
    DEBUG_TERMINAL << term_line << std::endl;

    static const std::string crash_string = "crash-severity issue occurred. stopping.";
    if (severity >= getInstance()->crash_level)
    {
        // if severity is too high, stop the program
        if (getInstance()->file_output.is_open()) getInstance()->file_output << crash_string << std::endl;
        DEBUG_TERMINAL << makeANSIColour(1, 0) << crash_string;
        exit(-1);
    }
}

void Debug::flush()
{
    if (getInstance() == nullptr) return;
    if (getInstance()->file_output.is_open()) getInstance()->file_output.flush();
    DEBUG_TERMINAL.flush();
}

std::vector<std::string> Debug::queryLines(size_t count)
{
    std::vector<std::string> arr;
    arr.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        if (i >= getInstance()->lines_history.size()) break;
        arr.push_back(*(getInstance()->lines_history.rbegin() + (count - i - 1)));
    }
    return arr;
}

Debug::Debug(const InitParams& params, bool& success)
{
    log_level   = params.log_level;
    crash_level = params.crash_level;

    const auto time_now = std::time(nullptr);
    tm time;
#if defined(_WIN32)
    (void)localtime_s(&time, &time_now);
#else
    auto tmp = localtime(&time_now);
    time     = *tmp;
#endif
    if (params.create_log_file)
    {
        // generate a unique name for the log file based on the time
        const std::string file_name =
            std::format("{}hop-engine {:0>2}_{:0>2}_{:0>2} {:0>2}.{:0>2}.{:0>2}.log", DEBUG_LOGFILE,
                time.tm_mday, time.tm_mon, time.tm_year - 100, time.tm_hour, time.tm_min, time.tm_sec);
        std::filesystem::create_directory(DEBUG_LOGFILE);
        file_output.open(file_name);
        if (!file_output.is_open())
        {
            DEBUG_TERMINAL << "FATAL ERROR: FAILED TO OPEN LOG FILE." << std::endl;
            success = false;
            exit(-1);
        }
    }

    success = true;
}

Debug::~Debug()
{
    Debug::flush();
    if (file_output.is_open()) file_output.close();
}
