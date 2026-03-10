#pragma once

#include <fstream>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable: 4005)
#endif

#if defined(NDEBUG)
#define DEBUG_LEVEL 2
#else
#define DEBUG_LEVEL 1
#endif
#define DEBUG_ENABLED

// these macros simplify the process of reporting to the debugger, but also allow
// compile-time elimination of frequently-issued debug commands (e.g. a release build
// probably doesn't need a debug print every time a command buffer is submitted).

#define DBG_BABBLE(str)
#define DBG_VERBOSE(str)
#define DBG_INFO(str)
#define DBG_WARNING(str)
#define DBG_ERROR(str)
#define DBG_FAULT(str) std::cout << str << std::endl; exit(-1)

#define PTR(ptr) Debug::pointerToString(ptr)

#if defined (DEBUG_ENABLED)

#if DEBUG_LEVEL == 0
#undef DBG_BABBLE
#define DBG_BABBLE(str) Debug::write(str, DEBUG_BABBLE)
#endif
#if DEBUG_LEVEL <= 1
#undef DBG_VERBOSE
#define DBG_VERBOSE(str) Debug::write(str, DEBUG_VERBOSE)
#endif
#if DEBUG_LEVEL <= 2
#undef DBG_INFO
#define DBG_INFO(str) Debug::write(str, DEBUG_INFO)
#endif
#if DEBUG_LEVEL <= 3
#undef DBG_WARNING
#define DBG_WARNING(str) Debug::write(str, DEBUG_WARNING)
#endif
#if DEBUG_LEVEL <= 4
#undef DBG_ERROR
#define DBG_ERROR(str) Debug::write(str, DEBUG_ERROR)
#endif
#if DEBUG_LEVEL <= 5
#undef DBG_FAULT
#define DBG_FAULT(str) Debug::write(str, DEBUG_FAULT)
#endif

#endif

#if defined(_WIN32)
#pragma warning(pop)
#endif

#include <string>

#include "common.h"

namespace HopEngine
{

/**
 * @brief enum which describes the severity of a debug output call.
 */
enum DebugLevel
{
	DEBUG_BABBLE,
	DEBUG_VERBOSE,
	DEBUG_INFO,
	DEBUG_WARNING,
	DEBUG_ERROR,
	DEBUG_FAULT
};

/**
 * @brief singleton class encapsulating logging functionality.
 */
class Debug
{
private:
	// minimum severity for a debug command to be sent to the log/terminal.
	DebugLevel log_level = DEBUG_INFO;
	// minimum severity for a debug command to trigger a program crash.
	DebugLevel crash_level = DEBUG_FAULT;
	std::ofstream file_output;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Debug);

	/**
	 * @brief initialise the logging system including the output log file.
	 * @param crash_level debug severity level which should cause the program to exit.
	 */
	static void init(DebugLevel crash_level);
	/**
	 * @brief shut down the logging system and close the output log file.
	 */
	static void close();

	/**
	 * @brief utility function converting any pointer type into a hex string.
	 * @param ptr pointer to convert, can be any type.
	 * @return a string with a hexadecimal description of the pointer.
	 */
	static std::string pointerToString(const void* ptr);
	/**
	 * @brief set the minimum severity level before \code write()\endcode calls
	 * will be printed to the terminal/logfile.
	 * @param severity new severity to be used from now on.
	 */
	static void setLogLevel(DebugLevel severity);
	/**
	 * @brief sends a line of debug output to the console, and the logfile. the
	 * line is prepended with a severity tag based on the given severity. this call
	 * will only proceed if the specified severity level is equal to or greater than
	 * the \code log_level\endcode field.
	 * @param description text to be printed.
	 * @param severity severity of the debug command. higher means worse.
	 */
	static void write(const std::string& description, DebugLevel severity);
	/**
	 * @brief force flush output to file, useful for circumstances where the program
	 * crashes.
	 */
	static void flush();

private:
	Debug(DebugLevel crash);
	~Debug();
};

}
