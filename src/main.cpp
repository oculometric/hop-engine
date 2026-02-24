#if !defined(STANDALONE)

#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

int main()
{
    Engine::init();

    Package::loadPackage("resources.hop");
    const auto& funcs = getNodeScene();
    funcs.init_func();
    
    Engine::setup(funcs.update_func, funcs.imgui_func);

    Engine::mainLoop();

    Engine::destroy();
    
    return 0;
}

#endif