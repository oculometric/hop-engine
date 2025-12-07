#pragma once

#include <glm/glm.hpp>

#include "common.h"

namespace HopEngine
{

class Object;

struct Transform
{
private:
	Object* object;
	glm::vec3 local_position;
	glm::vec3 local_euler;
	glm::vec4 local_quaternion;
	glm::vec3 local_scale;
	glm::mat4 local_matrix;
	bool world_matrix_dirty = true;
	glm::mat4 world_matrix;

public:
	Transform() : local_position({ 0, 0, 0 }), local_euler({ 0, 0, 0 }), local_quaternion({ 1, 0, 0, 0 }), local_scale({ 1, 1, 1 }), object(nullptr) { updateMatrix(); };
	Transform(glm::vec3 position, glm::vec3 euler, glm::vec3 scale) : local_position(position), local_euler(euler), local_quaternion({ 1, 0, 0, 0 }), local_scale(scale) { updateMatrix(); }
	Transform(Object* owner) : local_position({ 0, 0, 0 }), local_euler({ 0, 0, 0 }), local_quaternion({ 1, 0, 0, 0 }), local_scale({ 1, 1, 1 }), object(owner) { updateMatrix(); };
	
	inline glm::vec3 getLocalPosition() { return local_position; }
	inline void setLocalPosition(glm::vec3 position) { local_position = position; updateMatrix(); }
	inline glm::vec3 getLocalEuler() { return local_euler; }
	inline void setLocalEuler(glm::vec3 euler) { local_euler = euler; updateMatrix(); }
	inline glm::vec3 getLocalScale() { return local_scale; }
	inline void setLocalScale(glm::vec3 scale) { local_scale = scale; updateMatrix(); }

	inline void translateLocal(glm::vec3 offset) { local_position += offset; updateMatrix(); }
	inline void rotateLocal(glm::vec3 euler_offset) { local_euler += euler_offset; updateMatrix(); }
	inline void scaleLocal(glm::vec3 factor) { local_scale *= factor; updateMatrix(); }

	inline glm::mat4 getLocalMatrix() { return local_matrix; }
	inline glm::mat4 getMatrix() { updateWorldMatrix(); return world_matrix; }
	// TODO: world space transforms
	// TODO: quaternion functions

	void updateWorldMatrix();
private:
	void updateMatrix();
};

}