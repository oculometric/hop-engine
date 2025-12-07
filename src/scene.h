#pragma once

#include <vector>

#include "common.h"
#include "object.h"

namespace HopEngine
{

class Scene
{
public:
	glm::vec3 background_colour = { 0.004f, 0.509f, 0.506f };
	std::vector<Ref<Object>> objects;
	Ref<Camera> camera = nullptr;

private:

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Scene);
	
	inline Scene() { camera = new Camera(); }
	inline ~Scene() { };
};

}
