#pragma once

#include <vector>
#include <vulkan/vulkan.h>

#include "common.h"
#include "object.h"

namespace HopEngine
{

class Scene
{
public:
	glm::vec3 background_colour = { 0.004f, 0.509f, 0.506f };
	std::vector<Ref<Object>> objects;

private:

public:
	DELETE_NOT_ALL_CONSTRUCTORS(Scene);
	
	inline Scene() { };
	inline ~Scene() { };
};

}
