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

Ref<Object> asha;

void initScene(Ref<Scene> scene)
{
    Package::init();
    Package::loadPackage("resources.hop");

    Ref<Shader> shader = new Shader("res://psx", false);
    Ref<Sampler> sampler = new Sampler(VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
    asha = new Object(
        new Mesh("res://asha/asha.obj"),
        new Material(
            shader, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL
        ));
    asha->material->setTexture("albedo", new Texture("res://asha/asha.png"));
    asha->material->setSampler("albedo", sampler);
    asha->transform.setLocalPosition({ 0, 0, -0.9f });
    scene->objects.push_back(asha);
    Ref<Object> bunny = new Object(
        new Mesh("res://bunny.obj"),
        new Material(
            shader, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL
        ));
    bunny->material->setTexture("albedo", new Texture("res://bunny.png"));
    bunny->material->setSampler("albedo", sampler);
    bunny->parent = asha; // TODO: setParent function
    bunny->transform.updateWorldMatrix();
    bunny->transform.setLocalPosition({ 0, -0.5f, 0.9f });
    bunny->transform.scaleLocal({ 2, 2, 2 });
    scene->objects.push_back(bunny);

    scene->camera->transform.lookAt(glm::vec3(1.5f, -1.5f, 0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
}

void updateScene(Ref<Scene> scene)
{
    asha->transform.rotateLocal({ 0, 0.01f, 0 });
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

    asha = nullptr;

    ge = nullptr;
    window = nullptr;
    Window::terminateEnvironment();

    return 0;
}