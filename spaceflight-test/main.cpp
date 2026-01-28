#include "hop_engine.h"

using namespace HopEngine;
using namespace std;

Ref<Scene> initScene()
{
	Ref<Scene> scene = new Scene();
	scene->skybox = Engine::loadTexture("res://samples/nasa_goddard_gaia_dr2_deep_star_map.png");
	auto torus = scene->insertObject<StaticMesh>(new StaticMesh(Engine::loadMesh("res://museum/Pedestal3.obj"), RenderServer::getDefaultMaterial()));
	torus->transform.translate({ 0, 5, 0 });
	torus = scene->insertObject<StaticMesh>(new StaticMesh(Engine::loadMesh("res://museum/Pedestal3.obj"), RenderServer::getDefaultMaterial()));
	torus->transform.translate({ 0, 10, 0 });
	torus = scene->insertObject<StaticMesh>(new StaticMesh(Engine::loadMesh("res://museum/Pedestal3.obj"), RenderServer::getDefaultMaterial()));
	torus->transform.translate({ 0, 15, 0 });
	torus = scene->insertObject<StaticMesh>(new StaticMesh(Engine::loadMesh("res://museum/Pedestal3.obj"), RenderServer::getDefaultMaterial()));
	torus->transform.translate({ 0, 20, 0 });
	torus = scene->insertObject<StaticMesh>(new StaticMesh(Engine::loadMesh("res://museum/Pedestal3.obj"), RenderServer::getDefaultMaterial()));
	torus->transform.translate({ 0, 25, 0 });
	return scene;
}

void updateScene(Ref<Scene> scene, float delta_time)
{
	static float total_time = 0.0f;
	total_time += delta_time;
	
	static bool low_speed = true;
	if (Input::wasKeyPressed('F'))
		low_speed = !low_speed;
	
	WeakRef<Object> ship = scene->getCamera(0).cast<Object>();
	
	// TODO: convert to velocity calculations (including turn velocity)
	if (low_speed)
	{
		glm::vec3 turn_vector =
		{
			Input::getAxis('S', 'W'),
			Input::getAxis('D', 'A'),
			Input::getAxis('E', 'Q'),
		};
		ship->transform.rotateLocal(turn_vector * delta_time * 60.0f);
	}
	else
	{
		glm::vec2 nudge_vector =
		{
			Input::getAxis('A', 'D'),
			Input::getAxis('S', 'W'),
		};
		float roll = Input::getAxis('E', 'Q');
		ship->transform.rotateLocal({ 0, 0, roll * delta_time * 60.0f });
		ship->transform.translateLocal(glm::vec3(nudge_vector * delta_time * 5.0f, 0));
	}
	
	if (Input::isKeyDown(Input::KEY_UP))
		ship->transform.translateLocal(glm::vec3{ 0, 0, -delta_time * 2.0f });
	
	// at low speed - WASDQE represents turning the ship (QE is roll, WS is pitch, AD is yaw)
	// at high speed - WASD represents moving the ship (AD is push along the camera X, WS is push along the camera Y), QE represents roll
	// UP/DOWN represents throttle, spacebar represents a fast sideways hop (according to high speed controls, but with EQ representing bumping the roll)
}

void main()
{
	Engine::init();
	
	Engine::setup(initScene, updateScene, nullptr);
	Engine::mainLoop();

	Engine::destroy();
}
