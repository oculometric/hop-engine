#include "random.h"

#include "glm/glm.hpp"

#include <random>

using namespace HopEngine;

static std::mt19937 mt_rd((std::random_device())());

float Random::range(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, std::nextafter(max, __FLT_MAX__));
    return dist(mt_rd);
}

float Random::normalDistribution(float mean, float deviation)
{
    std::normal_distribution<float> dist(mean, deviation);
    return dist(mt_rd);
}

void Random::randomiseSeed()
{
    std::random_device rd;
    mt_rd = std::mt19937(rd());
}

void Random::setSeed(size_t seed) { mt_rd.seed(seed); }

size_t Random::limit(size_t limit)
{
    std::uniform_int_distribution<size_t> dist(0, limit - 1);
    return dist(mt_rd);
}

glm::vec3 Random::hemisphere(glm::vec3 vector)
{
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    const float y     = dist(mt_rd);
    const float r     = glm::sqrt(1.0f - (y * y));
    const float l     = dist(mt_rd) * M_PI;
    const glm::vec3 v = { glm::sin(l) * r, glm::cos(l) * r, y };

    if (glm::dot(v, vector) < 0.0f) return v * -glm::length(vector);
    else
        return v * glm::length(vector);
}

glm::vec3 Random::normal()
{
    glm::vec3 v = {
        Random::range(-1.0f, 1.0f),
        Random::range(-1.0f, 1.0f),
        Random::range(-1.0f, 1.0f),
    };
    return glm::normalize(v);
}

size_t Random::next() { return mt_rd(); }
