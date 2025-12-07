#include "transform.h"

#include <glm/gtc/matrix_transform.hpp>

#include "object.h"

using namespace HopEngine;

void Transform::updateMatrix()
{
	local_matrix = glm::mat4(1);
	local_matrix = glm::translate(local_matrix, local_position);
	local_matrix = glm::rotate(local_matrix, local_euler.z, glm::vec3(0, 0, 1));
	local_matrix = glm::rotate(local_matrix, local_euler.y, glm::vec3(0, 1, 0));
	local_matrix = glm::rotate(local_matrix, local_euler.x, glm::vec3(1, 0, 0));
	local_matrix = glm::scale(local_matrix, local_scale);

	updateWorldMatrix();
}

void Transform::updateWorldMatrix()
{
	world_matrix = glm::mat4(1);
	if (object && object->parent.get())
		world_matrix = object->parent->transform.getMatrix();
	world_matrix = world_matrix * local_matrix;
}
