#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#include "common.h"

namespace HopEngine
{

struct Transform final
{
	friend class Object;
private:
	Object* owner = nullptr;
	glm::vec3 local_position;
	glm::vec3 local_euler;
	glm::vec3 local_scale;
	glm::mat4 local_matrix;
	glm::mat4 world_matrix;

public:
	Transform() : local_position({ 0, 0, 0 }), local_euler({ 0, 0, 0 }), local_scale({ 1, 1, 1 }) { localFromVars(); };
	Transform(const glm::vec3 position, const glm::vec3 euler, const glm::vec3 scale) : local_position(position), local_euler(euler), local_scale(scale) { localFromVars(); }
	
	glm::vec3 getLocalPosition() const { return local_position; }
	glm::vec3 getLocalEuler() const { return local_euler; }
	glm::vec3 getLocalScale() const { return local_scale; }
	glm::mat4 getLocalMatrix() const { return local_matrix; }
	glm::vec3 getPosition() const { return world_matrix[3]; }
	glm::vec3 getEuler() const; // TODO:
	glm::mat4 getMatrix() { worldFromLocal(); return world_matrix; }
	glm::vec3 right() const { return world_matrix[0]; }		// represents world space X axis
	glm::vec3 up() const { return world_matrix[1]; }			// represents world space Y axis
	glm::vec3 forward() const { return -world_matrix[2]; }	// represents world space -Z axis
	
	void setLocalPosition(glm::vec3 position);
	void setLocalEuler(glm::vec3 euler);
	void setLocalScale(glm::vec3 scale);
	void setPosition(glm::vec3 position);
	void setEuler(glm::vec3 euler); // TODO:
	void setMatrix(const glm::mat4& matrix);

	void translateLocal(glm::vec3 offset);
	void rotateLocal(glm::vec3 degrees);
	void scaleLocal(glm::vec3 factor);
	void scaleLocal(float factor);
	void translate(glm::vec3 offset);
	void rotate(glm::vec3 axis, float degrees); // TODO:
	void rotate(glm::vec3 degrees);
	void scale(float factor);
	void lookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up);
	
	// TODO: quaternion support

private:
	void localFromWorld();
	void worldFromLocal();
	void localFromVars();
};

struct Spline
{
	std::vector<glm::vec3> points;
	bool loop = false;

	glm::vec3 operator[](float t) const;
};

}