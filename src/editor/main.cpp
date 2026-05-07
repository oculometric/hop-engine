#include "editor.h"
#include "hop_engine.h"

#include <iostream>

using namespace HopEngine;

void printLicense()
{
    std::cout << "        hop-engine  Copyright (C) 2026  cassette costen" << std::endl;
    std::cout << "This program comes with ABSOLUTELY NO WARRANTY; see COPYING.md for details." << std::endl;
    std::cout << "This is free software, and you are welcome to redistribute it" << std::endl;
    std::cout << "under certain conditions; see COPYING.md for details." << std::endl;
}

void printVersion()
{
    std::cout << "        hop-editor v" << HOP_ENGINE_VERSION_STRING << "-c" << HOP_ENGINE_COMMIT_STRING
              << ", created by cassette costen." << std::endl;
}

void printUsage()
{
    std::cout << "usage: hop-editor [OPTIONS]" << std::endl;
    std::cout << "options:" << std::endl;
    std::cout << "   -h" << std::endl;
    std::cout << "  --help       | show this help prompt" << std::endl;
    std::cout << "   -v" << std::endl;
    std::cout << "  --version    | display version information" << std::endl;
    std::cout << std::endl;
    std::cout << "  --validation | enable Vulkan validation layers if available" << std::endl;
    std::cout << std::endl;
    std::cout << "  --verboselog | set log level to verbose to display more output" << std::endl;
    std::cout << "  --infolog    | set log level to info to display regular output (default)" << std::endl;
    std::cout << "  --warninglog | set log level to warning to display only warnings and above"
              << std::endl;
    std::cout << "  --errorlog   | set log level to error to display only errors and above" << std::endl;
    std::cout << "  --silentlog  | set log level to fault to display only fatal errors" << std::endl;
    std::cout << "  --nologfile  | only display debug logs to the console, don't generate a logfile"
              << std::endl;
}

bool readOBJ(const DataBlock& data, std::vector<Mesh::Vertex>& verts, std::vector<uint16_t>& inds);

int main(int nargs, const char** cargs)
{
    printLicense();
    printVersion();
    Engine::InitParams params;
    CommandLineParser clargs(nargs, cargs);
    for (const auto& arg : clargs)
    {
        if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "validation")
            params.enable_vulkan_validation = true;
        else if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "verboselog")
            params.debug_log_level = Debug::DEBUG_VERBOSE;
        else if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "infolog")
            params.debug_log_level = Debug::DEBUG_INFO;
        else if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "warninglog")
            params.debug_log_level = Debug::DEBUG_WARNING;
        else if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "errorlog")
            params.debug_log_level = Debug::DEBUG_ERROR;
        else if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "silentlog")
            params.debug_log_level = Debug::DEBUG_FAULT;
        else if (arg.type == CommandLineParser::FLAG_TEXT && arg.value == "nologfile")
            params.create_log_file = false;
        else if ((arg.type == CommandLineParser::FLAG_TEXT && arg.value == "help") ||
                 (arg.type == CommandLineParser::FLAG_CHAR && arg.value == "h"))
        {
            printUsage();
            return 0;
        }
        else if ((arg.type == CommandLineParser::FLAG_TEXT && arg.value == "version") ||
                 (arg.type == CommandLineParser::FLAG_CHAR && arg.value == "v"))
        {
            return 0;
        }
        else
        {
            std::cout << "invalid option: " << arg.value << std::endl;
            printUsage();
            return 0;
        }
    }

    Engine::init(params);
    Engine::startApplication<Editor>();
    Engine::destroy();

    return 0;
}