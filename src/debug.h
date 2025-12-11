#pragma once

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

#define DBG_BABBLE(str)
#define DBG_VERBOSE(str)
#define DBG_INFO(str)
#define DBG_WARNING(str)
#define DBG_ERROR(str)
#define DBG_FAULT(str) std::cout << str << std::endl; exit(-1)

#define PTR(ptr) Debug::pointerToString(ptr)

#if defined (DEBUG_ENABLED)

#define DEBUG_TERMINAL cout
#define DEBUG_LOGFILE "log/"

#if DEBUG_LEVEL == 0
#undef DBG_BABBLE
#define DBG_BABBLE(str) Debug::write(str, Debug::DEBUG_BABBLE)
#endif
#if DEBUG_LEVEL <= 1
#undef DBG_VERBOSE
#define DBG_VERBOSE(str) Debug::write(str, Debug::DEBUG_VERBOSE)
#endif
#if DEBUG_LEVEL <= 2
#undef DBG_INFO
#define DBG_INFO(str) Debug::write(str, Debug::DEBUG_INFO)
#endif
#if DEBUG_LEVEL <= 3
#undef DBG_WARNING
#define DBG_WARNING(str) Debug::write(str, Debug::DEBUG_WARNING)
#endif
#if DEBUG_LEVEL <= 4
#undef DBG_ERROR
#define DBG_ERROR(str) Debug::write(str, Debug::DEBUG_ERROR)
#endif
#if DEBUG_LEVEL <= 5
#undef DBG_FAULT
#define DBG_FAULT(str) Debug::write(str, Debug::DEBUG_FAULT)
#endif

#endif

#if defined(_WIN32)
#pragma warning(pop)
#endif

#include <string>

#include "common.h"

namespace HopEngine
{

class Debug
{
public:
	enum DebugLevel
	{
		DEBUG_BABBLE,
		DEBUG_VERBOSE,
		DEBUG_INFO,
		DEBUG_WARNING,
		DEBUG_ERROR,
		DEBUG_FAULT
	};

private:
	DebugLevel log_level = DEBUG_INFO;
	DebugLevel crash_level = DEBUG_FAULT;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Debug);

	static void init(DebugLevel crash_level);
	static void write(std::string description, DebugLevel severity);
	static void flush();
	static void close();
	static std::string pointerToString(void* ptr);

private:
	Debug();
	~Debug();
};

}
