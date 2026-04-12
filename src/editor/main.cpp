#include "hop_engine.h"
#include "editor.h"

using namespace HopEngine;

int main(int nargs, const char** cargs)
{
    Engine::InitParams params;
    CommandLineParser clargs(nargs, cargs);
    for (const auto& arg : clargs)
    {
        if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "validation")
            params.enable_vulkan_validation = true;
        if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "verbose")
            Debug::setLogLevel(Debug::DEBUG_VERBOSE);
    }

    Engine::init(params);
    
    Engine::startApplication<Editor>();

    Engine::destroy();

    return 0;
}