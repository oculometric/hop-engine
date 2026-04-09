#include "hop_engine.h"
#include "editor.h"

using namespace HopEngine;

int main()
{
    Engine::init();
    
    Engine::startApplication<Editor>();

    Engine::destroy();

    return 0;
}