#include "hop_engine.h"

void main()
{
	HopEngine::Engine::init();

	HopEngine::Engine::mainLoop();

	HopEngine::Engine::destroy();
}
