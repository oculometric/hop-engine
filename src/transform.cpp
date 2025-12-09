#include "transform.h"

#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

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

void Transform::lookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up)
{
	local_matrix = glm::inverse(glm::lookAt(eye, target, up));
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(local_matrix, local_scale, local_quaternion, local_position, skew, perspective);
	local_euler = glm::eulerAngles(local_quaternion);

	updateWorldMatrix();
}

void Transform::updateWorldMatrix()
{
	world_matrix = glm::mat4(1);
	if (object && object->parent.isValid())
		world_matrix = object->parent->transform.getMatrix();
	world_matrix = world_matrix * local_matrix;
}

glm::vec3 getPoint(const Spline& s, int p)
{
	if (s.loop)
		return s.points[(p + s.points.size()) % s.points.size()];
	else
		return s.points[p < 0 ? 0 : (p >= s.points.size() ? s.points.size() - 1 : p)];
}

glm::vec3 Spline::operator[](float t)
{
	if (points.size() <= 0) return glm::vec3{ 0, 0, 0 };
	if (!loop)
	{
		if (t < 0.0f) return points[0];
		if (t >= 1.0f) return points[points.size() - 1];
	}
	else
	{
		t = fmod(t, 1.0f);
	}

	// size of each subdivision along the total curve, in time
	float point_t_size = 1.0f / (float)(points.size());

	// t is between 0-1, we want to use that to find which segment we're in
	int segment = 0;
	float t_ = t;
	while (t_ > point_t_size)
	{
		++segment;
		t_ -= point_t_size;
	}
	t_ /= point_t_size;

	// segment indicates the index of the first of the two vertices to erp between
	// t_ indicates the actual t value (i.e. between the first vertex and the second vertex)

	// figure out which points we're using
	glm::vec3 p0 = getPoint(*this, segment - 1);
	glm::vec3 p1 = getPoint(*this, segment);
	glm::vec3 p2 = getPoint(*this, segment + 1);
	glm::vec3 p3 = getPoint(*this, segment + 2);

	float t2 = t_ * t_;
	float t3 = t2 * t_;

	glm::vec3 p0_2 = p0 * 2.0f;
	glm::vec3 p1_2 = p1 * 2.0f;
	glm::vec3 p1_3 = p1 * 3.0f;
	glm::vec3 p1_5 = p1 * 5.0f;
	glm::vec3 p2_3 = p2 * 3.0f;
	glm::vec3 p2_4 = p2 * 4.0f;

	glm::vec3 tm0 = p1_2;
	glm::vec3 tm1 = (p2 - p0) * t_;
	glm::vec3 tm2 = ((p0_2 + p2_4) - (p1_5 + p3)) * t2;
	glm::vec3 tm3 = ((p1_3 + p3) - (p2_3 + p0)) * t3;

	// big maths
	return (tm0 + tm1 + tm2 + tm3) * 0.5f;
	//return (p1 * (1.0f - t_)) + (p2 * t_); // simple lerp alternative
}