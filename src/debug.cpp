#include "debug.h"

#include <fstream>
#include <format>
#include <ctime>
#include <iostream>

using namespace HopEngine;
using namespace std;

static Debug* application_debug = nullptr;
#if defined(DEBUG_LOGFILE)
static ofstream file_output;
#endif

Debug::~Debug()
{
	Debug::flush();
#if defined(DEBUG_LOGFILE)
	if (file_output.is_open())
		file_output.close();
#endif
}

void Debug::init(DebugLevel crash_level)
{
	if (application_debug == nullptr)
		application_debug = new Debug();
	application_debug->log_level = (DebugLevel)DEBUG_LEVEL;
	application_debug->crash_level = crash_level;
}

void Debug::write(string description, DebugLevel severity)
{
	if (severity < application_debug->log_level)
		return;
	
	string log_type = "";
	switch (severity)
	{
	case DEBUG_BABBLE: log_type = "BABBLE"; break;
	case DEBUG_VERBOSE: log_type = "VERBOSE"; break;
	case DEBUG_INFO: log_type = "INFO"; break;
	case DEBUG_WARNING: log_type = "WARNING"; break;
	case DEBUG_ERROR: log_type = "ERROR"; break;
	case DEBUG_FAULT: log_type = "FAULT"; break;
	}

	auto time_now = std::time(0);
	tm time;
	localtime_s(&time, &time_now);
	
	string log_line = format("[{: >8} ]: {:0>2}:{:0>2}:{:0>2} - {}", log_type, time.tm_hour, time.tm_min, time.tm_sec, description);

#if defined(DEBUG_LOGFILE)
	file_output << log_line << endl;
#endif
#if defined(DEBUG_TERMINAL)
	DEBUG_TERMINAL << log_line << endl;
#endif
	
	if (severity >= application_debug->crash_level)
		exit(-1);
}

void Debug::flush()
{
#if defined(DEBUG_LOGFILE)
	file_output.flush();
#endif
#if defined(DEBUG_TERMINAL)
	DEBUG_TERMINAL.flush();
#endif
}

void Debug::close()
{
	Debug::flush();
#if defined(DEBUG_LOGFILE)
	file_output.close();
#endif
}

Debug::Debug()
{
#if defined(DEBUG_LOGFILE)
	auto time_now = std::time(0);
	tm time;
	localtime_s(&time, &time_now);
	string file_name = format("engine_{:0>2}_{:0>2}_{:0>2}.log", time.tm_hour, time.tm_min, time.tm_sec);
	file_output.open(file_name);
#endif
}
