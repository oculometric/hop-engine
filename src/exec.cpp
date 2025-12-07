#include "common.h"

#include <array>

#ifndef _WIN32
#include <unistd.h>
#else
#define popen _popen
#define pclose _pclose
#endif

using namespace std;

int exec(string command, string& output)
{
	const size_t buffer_size = 512;
	array<char, buffer_size> buffer;

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
}