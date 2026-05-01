#include "scene.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/common.hpp>
#include <glm/common.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using namespace HopEngine;

void Transform::setLocalPosition(const glm::vec3 position)
{
	local_position = position;
	localFromVars();
}

void Transform::setLocalEuler(const glm::vec3 euler)
{
	local_euler = euler;
	localFromVars();
}

void Transform::setLocalScale(const glm::vec3 scale)
{
	local_scale = scale;
	localFromVars();
}

void Transform::setPosition(const glm::vec3 position)
{
	world_matrix[3] = glm::vec4(position, 1);
	localFromWorld();
}

void Transform::setMatrix(const glm::mat4& matrix)
{
	world_matrix = matrix;
	localFromWorld();
}

void Transform::translateLocal(const glm::vec3 offset)
{
	local_position += local_matrix * glm::vec4(offset, 0);
	localFromVars();
}

void Transform::rotateLocal(const glm::vec3 degrees)
{
	local_matrix = glm::mat4(1);
	local_matrix = glm::rotate(local_matrix, glm::radians(local_euler.z), glm::vec3(0, 0, 1));
	local_matrix = glm::rotate(local_matrix, glm::radians(local_euler.y), glm::vec3(0, 1, 0));
	local_matrix = glm::rotate(local_matrix, glm::radians(local_euler.x), glm::vec3(1, 0, 0));

	local_matrix = glm::rotate(local_matrix, glm::radians(degrees.z), glm::vec3(0, 0, 1));
	local_matrix = glm::rotate(local_matrix, glm::radians(degrees.y), glm::vec3(0, 1, 0));
	local_matrix = glm::rotate(local_matrix, glm::radians(degrees.x), glm::vec3(1, 0, 0));
	
	glm::vec3 skew;
	glm::vec3 tmp;
	glm::vec3 tmp2;
	glm::vec4 perspective;
	glm::quat quat;
	glm::decompose(local_matrix, tmp, quat, tmp2, skew, perspective);
	local_euler = glm::degrees(glm::eulerAngles(quat));
	
	localFromVars();
}

void Transform::scaleLocal(const glm::vec3 factor)
{
	local_scale *= factor;
	localFromVars();
}

void Transform::scaleLocal(const float factor)
{
	local_scale *= factor;
	localFromVars();
}

void Transform::translate(const glm::vec3 offset)
{
	world_matrix[3] += glm::vec4(offset, 0);
	localFromWorld();
}

void Transform::rotate(const glm::vec3 degrees)
{
	glm::mat4 m = glm::mat4(1);
	m = glm::rotate(m, glm::radians(degrees.z), glm::vec3{ 0, 0, 1 });
	m = glm::rotate(m, glm::radians(degrees.y), glm::vec3{ 0, 1, 0 });
	m = glm::rotate(m, glm::radians(degrees.x), glm::vec3{ 1, 0, 0 });
	glm::vec4 translation = world_matrix[3];
	world_matrix[3] = glm::vec4{ 0, 0, 0, 1 };
	world_matrix = m * world_matrix;
	world_matrix[3] = translation;

	localFromWorld();
}

void Transform::scale(const float factor)
{
	world_matrix[0][0] *= factor;
	world_matrix[1][1] *= factor;
	world_matrix[2][2] *= factor;
	localFromWorld();
}

void Transform::lookAt(glm::vec3 eye, glm::vec3 target, glm::vec3 up)
{
	world_matrix = glm::inverse(glm::lookAt(eye, target, up));
	localFromWorld();
}

void Transform::localFromWorld()
{
	glm::mat4 parent = glm::identity<glm::mat4>();
	if (owner && owner->getParent())
		parent = owner->getParent()->getTransform().getMatrix();
	local_matrix = glm::inverse(parent) * world_matrix;

	glm::vec3 skew;
	glm::vec4 perspective;
	glm::quat quat;
	glm::decompose(local_matrix, local_scale, quat, local_position, skew, perspective);
	local_euler = glm::degrees(glm::eulerAngles(quat));
}

void Transform::worldFromLocal()
{
	world_matrix = glm::identity<glm::mat4>();
	if (owner && owner->getParent())
		world_matrix = owner->getParent()->getTransform().getMatrix();
	world_matrix = world_matrix * local_matrix;
}

void Transform::localFromVars()
{
	local_matrix = glm::mat4(1);
	local_matrix = glm::translate(local_matrix, local_position);
	local_matrix = glm::rotate(local_matrix, glm::radians(local_euler.z), glm::vec3(0, 0, 1));
	local_matrix = glm::rotate(local_matrix, glm::radians(local_euler.y), glm::vec3(0, 1, 0));
	local_matrix = glm::rotate(local_matrix, glm::radians(local_euler.x), glm::vec3(1, 0, 0));
	local_matrix = glm::scale(local_matrix, local_scale);

	worldFromLocal();
}

static glm::vec3 getPoint(const Spline& s, const int p)
{
	if (s.loop)
		return s.points[(p + s.points.size()) % s.points.size()];
	else
		return s.points[p < 0 ? 0 : (p >= static_cast<int>(s.points.size()) ? static_cast<int>(s.points.size()) - 1 : p)];
}

glm::vec3 Spline::operator[](float t) const
{
	if (points.empty()) return glm::vec3{ 0, 0, 0 };
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
	float point_t_size = 1.0f / static_cast<float>(points.size());

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
