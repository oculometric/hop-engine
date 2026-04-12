#include "engine.h"

using namespace HopEngine;

CommandLineParser::CommandLineParser(int nargs, const char** cargs)
{
    if (nargs == 0) return;
    executable_path = cargs[0];
    for (int i = 1; i < nargs; ++i) arguments.push_back(cargs[i]);

    for (const auto& arg : arguments)
    {
        if (arg.size() >= 2 && arg[0] == '-' && arg[1] == '-')
            arguments_parsed.emplace_back(FLAG_TEXT, arg.substr(2));
        else if (arg.size() >= 1 && arg[0] == '-')
            arguments_parsed.emplace_back(FLAG_CHAR, arg.substr(1));
        else
            arguments_parsed.emplace_back(ARGUMENT_TEXT, arg);
    }
}
