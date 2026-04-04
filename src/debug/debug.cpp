#include "debug.h"

#include <fstream>
#include <format>
#include <ctime>
#include <iostream>
#include <filesystem>

#define DEBUG_TERMINAL cout
#define DEBUG_LOGFILE "log/"

using namespace HopEngine;
using namespace std;

static Debug* instance = nullptr;

void Debug::init(const Level crash_level)
{
	if (instance == nullptr)
		instance = new Debug(crash_level);

	DBG_INFO("initialised debug");
}

void Debug::close()
{
	if (instance != nullptr)
	{
		delete instance;
		instance = nullptr;
	}
}

void Debug::setLogLevel(const Level severity)
{
	instance->log_level = severity;
}

// generates an ANSI colour command from the given foreground and background colours
static string makeANSIColour(const int fgcol, const int bgcol)
{
	return "\033[" + to_string(fgcol + 30) + ';' + to_string(bgcol + 40) + 'm';
}

string Debug::pointerToString(const void* ptr)
{
	return format("0x{:x}", reinterpret_cast<size_t>(ptr));
}

void Debug::write(const string& description, Level severity)
{
	if (instance == nullptr)
	{
		DEBUG_TERMINAL << description << endl;
		return;
	}

	if (severity < instance->log_level)
		return;

	const string bracket_col = makeANSIColour(60, 0);
	const string standard_col = makeANSIColour(67, 0);
	const string time_col = makeANSIColour(5, 0);
	
	string type_col = "";
	string log_type = "";
	// for the terminal output, use ANSI terminal colours to make the output more fun
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

	auto time_now = std::time(nullptr);
	tm time;
#if defined(_WIN32)
	(void)localtime_s(&time, &time_now);
#else
	auto tmp = localtime(&time_now);
	time = *tmp;
#endif
	// generating a timestamp for the output
	string log_line = format("[{: >8} ]: {:0>2}:{:0>2}:{:0>2} - {}", log_type, time.tm_hour, time.tm_min, time.tm_sec, description);
	string term_line = format("{}[{}{: >8} {}]{}: {}{:0>2}:{:0>2}:{:0>2}{} - {}", 
	                          bracket_col, type_col, log_type, bracket_col, standard_col,
	                          time_col, time.tm_hour, time.tm_min, time.tm_sec, standard_col, description);
    instance->lines_history.push_back(log_line);
    if (instance->lines_history.size() > 256)
        instance->lines_history.pop_front();
	if (instance->file_output.is_open())
		instance->file_output << log_line << endl;
	DEBUG_TERMINAL << term_line << endl;
	
	static string crash_string = "crash-severity issue occurred. stopping.";
	if (severity >= instance->crash_level)
	{
		// if severity is too high, stop the program
		if (instance->file_output.is_open())
			instance->file_output << crash_string << endl;
		DEBUG_TERMINAL << makeANSIColour(1, 0) << crash_string;
		exit(-1);
	}
}

void Debug::flush()
{
	if (instance == nullptr)
		return;
	if (instance->file_output.is_open())
		instance->file_output.flush();
	DEBUG_TERMINAL.flush();
}

vector<string> Debug::queryLines(size_t count)
{
    vector<string> arr;
    arr.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        if (i >= instance->lines_history.size())
            break;
        arr.push_back(*(instance->lines_history.rbegin() + (count - i - 1)));
    }
    return arr;
}

Debug::Debug(Level crash)
{
	instance = this;

	log_level = static_cast<Level>(DEBUG_LEVEL);
	crash_level = crash;

	const auto time_now = std::time(nullptr);
	tm time;
#if defined(_WIN32)
	(void)localtime_s(&time, &time_now);
#else
	auto tmp = localtime(&time_now);
	time = *tmp;
#endif
	// generate a unique name for the log file based on the time
	const string file_name = format("{}engine_{:0>2}_{:0>2}_{:0>2}.log", DEBUG_LOGFILE, time.tm_hour, time.tm_min, time.tm_sec);
	filesystem::create_directory(DEBUG_LOGFILE);
	file_output.open(file_name);
	if (!file_output.is_open())
	{
		DEBUG_TERMINAL << "FATAL ERROR: FAILED TO OPEN LOG FILE." << endl;
		exit(-1);
	}
}

Debug::~Debug()
{
	Debug::flush();
	if (file_output.is_open())
		file_output.close();
}
