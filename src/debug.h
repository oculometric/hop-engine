#pragma once

#pragma warning(push)
#pragma warning(disable: 4005)

#define DEBUG_LEVEL 0
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
#define DBG_BABBLE(str) Debug::write(str, Debug::DEBUG_BABBLE)
#endif
#if DEBUG_LEVEL <= 1
#define DBG_VERBOSE(str) Debug::write(str, Debug::DEBUG_VERBOSE)
#endif
#if DEBUG_LEVEL <= 2
#define DBG_INFO(str) Debug::write(str, Debug::DEBUG_INFO)
#endif
#if DEBUG_LEVEL <= 3
#define DBG_WARNING(str) Debug::write(str, Debug::DEBUG_WARNING)
#endif
#if DEBUG_LEVEL <= 4
#define DBG_ERROR(str) Debug::write(str, Debug::DEBUG_ERROR)
#endif
#if DEBUG_LEVEL <= 5
#define DBG_FAULT(str) Debug::write(str, Debug::DEBUG_FAULT)
#endif

#endif
#pragma warning(pop)

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

	~Debug();

	static void init(DebugLevel crash_level);
	static void write(std::string description, DebugLevel severity);
	static void flush();
	static void close();
	static std::string pointerToString(void* ptr);

private:
	Debug();
};

}
