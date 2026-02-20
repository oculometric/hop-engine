#if !defined(STANDALONE)

#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

int main()
{
    Engine::init();

    Package::loadPackage("resources.hop");
    const auto& scene = getMuseumScene();
    const auto& scene2 = getAshaScene();

    auto sa = scene2.init_func();
    auto sm = scene.init_func();
    
    RenderServer::setMultiScene({
        MultiSceneRenderSpec{ sa, { 0, 0 }, { 0.6f, 1 } },
        MultiSceneRenderSpec{ sm, { 0.6f, 0}, { 0.4f, 1 } }
    });
    
    Engine::setup(scene.update_func, scene.imgui_func);

    sa = nullptr;
    sm = nullptr;
    
    Engine::mainLoop();

    Engine::destroy();
    
    return 0;
}

#endif