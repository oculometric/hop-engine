#if !defined(STANDALONE)

#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

int main()
{
    Engine::init();

    Package::loadPackage("resources.hop");
    const auto& scene = getMuseumScene();

    Engine::setup(scene.init_func, scene.update_func, scene.imgui_func);

    Engine::mainLoop();

    Engine::destroy();
    
    return 0;
}

#endif