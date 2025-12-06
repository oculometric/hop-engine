#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>

#include <iostream>

#include "hop_engine.h"

using namespace HopEngine;

void initScene(Ref<Scene> scene)
{
    Package::init();
    Package::loadPackage("resources.hop");

    Ref<Shader> shader = new Shader("res://psx", false);
    Ref<Sampler> sampler = new Sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    Ref<Object> asha = new Object(
        new Mesh("res://asha/asha.obj"),
        new Material(
            shader, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL
        ));
    asha->material->setTexture("albedo", new Texture("res://asha/asha.png"));
    asha->material->setSampler("albedo", sampler);
    asha->position.z = -0.9f;
    scene->objects.push_back(asha);
    Ref<Object> bunny = new Object(
        new Mesh("res://bunny.obj"),
        new Material(
            shader, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL
        ));
    bunny->material->setTexture("albedo", new Texture("res://bunny.png"));
    bunny->material->setSampler("albedo", sampler);
    bunny->position.y = -0.5;
    bunny->scale = { 2, 2, 2 };
    scene->objects.push_back(bunny);
}

void updateScene(Ref<Scene> scene)
{

}

int main() {
    Window::initEnvironment();
    Ref<Window> window = new HopEngine::Window(1024, 1024, "hop!");
    Ref<GraphicsEnvironment> ge = new HopEngine::GraphicsEnvironment(window);

    ge->scene = new Scene();
    initScene(ge->scene);

    while (!window.get()->getShouldClose())
    {
        window.get()->pollEvents();
        ge.get()->drawFrame();
        updateScene(ge->scene);
    }

    ge = nullptr;
    window = nullptr;
    Window::terminateEnvironment();

    return 0;
}