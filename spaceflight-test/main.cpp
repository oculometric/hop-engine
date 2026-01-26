#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

Ref<Scene> initScene()
{
	return new Scene();
}

void updateScene(Ref<Scene> scene, float delta_time)
{
	static float total_time = 0.0f;
	total_time += delta_time;

	scene->getCamera(0)->clear_colour.r = (sinf(total_time * 4) * 0.5f) + 0.5f;
}

void main()
{
	Engine::init();
	
	Engine::setup(initScene, updateScene, nullptr);
	Engine::mainLoop();

	Engine::destroy();
}
