#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "common.h"

namespace HopEngine
{

struct Transform
{
	friend class Object;
private:
	glm::vec3 local_position;
	glm::vec3 local_euler;
	glm::vec3 local_scale;
	Transform* parent_transform = nullptr;
	glm::mat4 local_matrix;
	glm::mat4 world_matrix;

public:
	Transform() : local_position({ 0, 0, 0 }), local_euler({ 0, 0, 0 }), local_scale({ 1, 1, 1 }) { updateMatrix(); };
	Transform(glm::vec3 position, glm::vec3 euler, glm::vec3 scale) : local_position(position), local_euler(euler), local_scale(scale) { updateMatrix(); }
	
	inline glm::vec3 getLocalPosition() { return local_position; }
	inline void setLocalPosition(glm::vec3 position) { local_position = position; updateMatrix(); }
	inline glm::vec3 getLocalEuler() { return local_euler; }
	inline void setLocalEuler(glm::vec3 euler) { local_euler = euler; updateMatrix(); }
	inline glm::vec3 getLocalScale() { return local_scale; }
	inline void setLocalScale(glm::vec3 scale) { local_scale = scale; updateMatrix(); }

	inline glm::vec3 getPosition() { return world_matrix[3]; }
	inline void setPosition(glm::vec3 position) { world_matrix[3] = glm::vec4(position, 1); correctLocalMatrix(); }
	glm::vec3 getEuler();
	inline void setEuler(glm::vec3 euler);

	inline void translateLocal(glm::vec3 offset) { local_position += offset; updateMatrix(); }
	inline void rotateLocal(glm::vec3 euler_offset) { local_euler += euler_offset; updateMatrix(); }
	inline void scaleLocal(glm::vec3 factor) { local_scale *= factor; updateMatrix(); }
	inline void scaleLocal(float factor) { local_scale *= factor; updateMatrix(); }

	inline void translate(glm::vec3 offset) { world_matrix[3] += glm::vec4(offset, 0); correctLocalMatrix(); }
	inline void rotate(glm::vec3 euler_offset);
	inline void scale(float factor);

	inline glm::mat4 getLocalMatrix() { return local_matrix; }
	inline glm::mat4 getMatrix() { return world_matrix; }
	inline void setMatrix(glm::mat4 matrix) { world_matrix = matrix; correctLocalMatrix(); }

	// TODO: world space transforms
	
	// TODO: quaternion support

	void lookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up);

private:
	void correctLocalMatrix();
	void updateWorldMatrix();
	void updateMatrix();
};

struct Spline
{
	std::vector<glm::vec3> points;
	bool loop = false;

	glm::vec3 operator[](float t);
};

}