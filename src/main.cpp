#if !defined(STANDALONE)

#include "main.h"

#include "hop_engine.h"
#include "editor.h"

using namespace HopEngine;

int main()
{
    Engine::init();

    Package::importPackage("resources.hop");
    
    Engine::runApplication<Editor>();

    Engine::destroy();

    return 0;
}

#endif