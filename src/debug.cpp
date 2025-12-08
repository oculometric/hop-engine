#include "debug.h"

#include <fstream>
#include <format>
#include <ctime>
#include <iostream>
#include <filesystem>

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
	DBG_INFO("initialised debug");
}

string makeANSIColour(int fgcol, int bgcol)
{
	return "\033[" + to_string(fgcol + 30) + ';' + to_string(bgcol + 40) + 'm';
}

void Debug::write(string description, DebugLevel severity)
{
	if (severity < application_debug->log_level)
		return;

	const string bracket_col = makeANSIColour(60, 0);
	const string standard_col = makeANSIColour(67, 0);
	const string time_col = makeANSIColour(5, 0);
	
	string type_col = "";
	string log_type = "";
	switch (severity)
	{
	case DEBUG_BABBLE:
		log_type = "BABBLE";
		type_col = makeANSIColour(4, 0);
		break;
	case DEBUG_VERBOSE:
		log_type = "VERBOSE";
		type_col = makeANSIColour(64, 0);
		break;
	case DEBUG_INFO:
		log_type = "INFO";
		type_col = makeANSIColour(6, 0);
		break;
	case DEBUG_WARNING:
		log_type = "WARNING";
		type_col = makeANSIColour(63, 0);
		break;
	case DEBUG_ERROR:
		log_type = "ERROR";
		type_col = makeANSIColour(61, 0);
		break;
	case DEBUG_FAULT:
		log_type = "FAULT";
		type_col = makeANSIColour(1, 0);
		break;
	}

	auto time_now = std::time(0);
	tm time;
	localtime_s(&time, &time_now);

	string log_line = format("[{: >8} ]: {:0>2}:{:0>2}:{:0>2} - {}", log_type, time.tm_hour, time.tm_min, time.tm_sec, description);
	string term_line = format("{}[{}{: >8} {}]{}: {}{:0>2}:{:0>2}:{:0>2}{} - {}", 
		bracket_col, type_col, log_type, bracket_col, standard_col,
		time_col, time.tm_hour, time.tm_min, time.tm_sec, standard_col, description);

#if defined(DEBUG_LOGFILE)
	file_output << log_line << endl;
#endif
#if defined(DEBUG_TERMINAL)
	DEBUG_TERMINAL << term_line << endl;
#endif
	
	static string crash_string = "crash-severity issue occurred. stopping.";
	if (severity >= application_debug->crash_level)
	{
		file_output << crash_string << endl;
		DEBUG_TERMINAL << makeANSIColour(1, 0) << crash_string;
		exit(-1);
	}
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

string Debug::pointerToString(void* ptr)
{
	return format("0x{:x}", (size_t)ptr);
}

Debug::Debug()
{
#if defined(DEBUG_LOGFILE)
	auto time_now = std::time(0);
	tm time;
	localtime_s(&time, &time_now);
	string file_name = format("{}engine_{:0>2}_{:0>2}_{:0>2}.log", DEBUG_LOGFILE, time.tm_hour, time.tm_min, time.tm_sec);
	filesystem::create_directory(DEBUG_LOGFILE);
	file_output.open(file_name);
#endif
}
