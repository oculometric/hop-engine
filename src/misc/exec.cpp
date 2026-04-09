#include "common.h"

#include <array>

#ifndef _WIN32
#include <unistd.h>
#else
#define popen _popen
#define pclose _pclose
#include <Windows.h>
#endif

using namespace std;

int exec(const string& command, string& output)
{
	constexpr size_t buffer_size = 512;
	array<char, buffer_size> buffer;
#if defined(_WIN32)
	STARTUPINFO startup_info{ };
	PROCESS_INFORMATION process_info{ };
	SECURITY_ATTRIBUTES security{ };
	security.nLength = sizeof(SECURITY_ATTRIBUTES);
	security.bInheritHandle = true;
	security.lpSecurityDescriptor = nullptr;
	HANDLE child_read;
	HANDLE child_write;

	if (!CreatePipe(&child_read, &child_write, &security, 0))
    {
        output = "popen failed.";
		return -1;
    }

    if (!SetHandleInformation(child_read, HANDLE_FLAG_INHERIT, 0))
    {
        output = "popen failed.";
		return -1;
    }

	startup_info.cb = sizeof(STARTUPINFO);
    startup_info.hStdError = child_write;
    startup_info.hStdOutput = child_write;
    startup_info.dwFlags |= STARTF_USESTDHANDLES;

	if (CreateProcess(
		nullptr,
		const_cast<char*>(command.c_str()),
		nullptr, nullptr, true, 0, nullptr, nullptr,
		&startup_info, &process_info))
	{
		DBG_VERBOSE("process created");
		WaitForSingleObject(process_info.hProcess, INFINITE);
		CloseHandle(child_read);
      	CloseHandle(child_write);

		DWORD dwRead;

		while(true)
		{
			auto result = 
			ReadFile(child_read, buffer.data(), static_cast<DWORD>(buffer.size()), &dwRead, NULL);
			output.insert(output.end(), begin(buffer), next(begin(buffer), dwRead));
			if (result != ERROR_MORE_DATA)
				break;
			if (dwRead == 0)
				break;
		}

		DWORD result;
		GetExitCodeProcess(process_info.hProcess, &result);
		
		CloseHandle(process_info.hProcess);
		CloseHandle(process_info.hThread);

		return result;
	}
	output = "popen failed.";
	return -1;

#else
	auto pipe = popen((command + " 2>&1").c_str(), "r");
	if (!pipe)
	{
		output = "popen failed.";
		return -1;
	}

	output = "";
	size_t count;
	do {
		if ((count = fread(buffer.data(), 1, buffer_size, pipe)) > 0)
			output.insert(output.end(), begin(buffer), next(begin(buffer), count));
	} while (count > 0);

	return pclose(pipe);
#endif
}