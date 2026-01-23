#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

static int selected_scene = 0;

int main()
{
    Engine::init();
    
    const auto& scene = getMuseumScene();

    Engine::setup(scene.init_func, scene.update_func, scene.imgui_func);

    Engine::mainLoop();

    Engine::destroy();
    
    return 0;
}