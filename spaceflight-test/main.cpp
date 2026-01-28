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

glm::vec3 spaceship_velocity = glm::vec3{ 0, 0, 0 };
float throttle = 0.0f;

void updateScene(Ref<Scene> scene, float delta_time)
{
	static float total_time = 0.0f;
	total_time += delta_time;
	
	static bool low_speed = true;
	if (Input::wasKeyPressed('F'))
		low_speed = !low_speed;
	
	WeakRef<Object> ship = scene->getCamera(0).cast<Object>();
	
	if (glm::dot(spaceship_velocity, ship->transform.forward()) < 4.0f)
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
		spaceship_velocity += ship->transform.getMatrix() * glm::vec4{ nudge_vector * delta_time * 65.0f, 0, 0 };
	}
	
	ship->transform.translate(spaceship_velocity * delta_time);

	// at low speed - WASDQE represents turning the ship (QE is roll, WS is pitch, AD is yaw)
	// at high speed - WASD represents moving the ship (AD is push along the camera X, WS is push along the camera Y), QE represents roll
	// UP/DOWN represents throttle, spacebar represents a fast sideways hop (according to high speed controls, but with EQ representing bumping the roll)
	throttle += Input::getAxis(Input::KEY_DOWN, Input::KEY_UP) * delta_time * 0.8f;
	throttle = glm::max(throttle, 0.0f);

	//glm::vec3 drag_force = -spaceship_velocity * glm::pow(glm::length(spaceship_velocity), 2.0f) * 2.0f;
	//spaceship_velocity += drag_force * delta_time;
	// TODO: instead of a drag force, either we should move the ship directly with throttle (i.e. no velocity effectively), or we should use a custom drag function which just pushes the movement vector back towards the forward vector directly without reducing overall speed
	spaceship_velocity += ship->transform.forward() * throttle;
}

void main()
{
	Engine::init();
	
	Engine::setup(initScene, updateScene, nullptr);
	Engine::mainLoop();

	Engine::destroy();
}
