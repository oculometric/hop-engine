#include "hop_engine.h"
#include "editor.h"

using namespace HopEngine;

int main()
{
    Engine::init();

    Package::importPackage("resources.hop");
    
    Engine::startApplication<Editor>();

    Engine::destroy();

    return 0;
}