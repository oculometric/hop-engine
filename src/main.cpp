#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>

#include <iostream>

#include "hop_engine.h"
#include "node_view.h"

using namespace HopEngine;

Ref<Object> asha;
Ref<Object> cube;

struct LightParams
{
    glm::vec4 position = { 0, -3, 0, 0 };
    glm::vec4 direction = { 0, 0, 0, 0 };
    glm::vec4 colour = { 1, 0, 0, 0 };
    float spot_angle = 0.0f;
    float constant_attenuation = 0.0f;
    float linear_attenuation = 0.0f;
    float quadratic_attenuation = 1.0f;
    int light_type = 0;
    bool enabled = true;
    glm::ivec2 padding;
};

struct MaterialParams
{
    glm::vec4 diffuse = { 1, 1, 1, 0 };
    glm::vec4 specular = { 1, 1, 1, 0 };
    glm::vec4 ambient = { 1, 1, 1, 0 };
    glm::vec4 emissive = { 0, 0, 0, 0 };
    float specular_exponent = 32.0f;
    glm::vec3 padding;
};

void initScene(Ref<Scene> scene)
{
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
        new Material(shader, VK_CULL_MODE_NONE, VK_POLYGON_MODE_FILL)
    );
    bunny->material->setTexture("albedo", new Texture("res://bunny.png"));
    bunny->material->setSampler("albedo", sampler);
    bunny->parent = asha; // TODO: setParent function
    bunny->transform.updateWorldMatrix();
    bunny->transform.setLocalPosition({ 0, -0.5f, 0.9f });
    bunny->transform.scaleLocal({ 2, 2, 2 });
    scene->objects.push_back(bunny);

    cube = new Object(
        new Mesh("res://cube.obj"), 
        new Material(new Shader("res://split", false), VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL)
    );
    cube->material->setTexture("tex", new Texture("res://stone.png"));
    cube->material->setTexture("tex2", new Texture("res://tex2.png"));
    cube->transform.scaleLocal({ 0.5f, 0.5f, 0.5f });
    MaterialParams material;
    LightParams light;
    glm::vec4 ambient_colour = { 0.1f, 0.1f, 0.1f, 0.0f };
    cube->material->setUniform("material", &material, sizeof(MaterialParams));
    cube->material->setUniform("light", &light, sizeof(LightParams));
    cube->material->setVec4Uniform("ambient_colour", ambient_colour);
    scene->objects.push_back(cube);

    scene->camera->transform.lookAt(glm::vec3(0.5f, -1.5f, 0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
}

void updateScene(Ref<Scene> scene)
{
    asha->transform.rotateLocal({ 0, 0.01f, 0 });
    cube->transform.rotateLocal({ 0, 0, 0.03f });

    glm::vec2 mouse_delta = Input::getMouseDelta() * 0.004f;
    if (Input::isMouseDown(GLFW_MOUSE_BUTTON_2))
        scene->camera->transform.rotateLocal({ -mouse_delta.y, 0, -mouse_delta.x });

    glm::mat4 camera_matrix = scene->camera->transform.getMatrix();
    glm::vec3 local_move_vector = glm::vec3{
        Input::getAxis('A', 'D'),
        Input::getAxis('Q', 'E'),
        Input::getAxis('W', 'S')
    } * 0.02f;
    if (Input::isKeyDown(GLFW_KEY_LEFT_SHIFT))
        local_move_vector *= 3.0f;
    scene->camera->transform.translateLocal(camera_matrix * glm::vec4(local_move_vector, 0));
}

void initNodeScene(Ref<Scene> scene)
{
    Ref<NodeView> node_view = new NodeView();
    node_view->boxes.push_back({ "multiply", "this node has a description. yea, indeed it does. it even wraps automatically! what the hell am i doing here", { 0, 0 }, { 18, 28 }});
    //node_view->boxes.push_back({ { 0, 5 }, { 4, 2 } });
    node_view->updateMesh();
    node_view->transform.setLocalScale({ 0.25f, 0.25f, 1.0f });
    scene->objects.push_back(node_view.cast<Object>());

    scene->camera->transform.lookAt({ 0, -1, 1 }, { 0, 0, 0 }, { 0, 0, 1 });
}

void updateNodeScene(Ref<Scene> scene)
{
    glm::vec2 mouse_delta = Input::getMouseDelta() * 0.004f;
    if (Input::isMouseDown(GLFW_MOUSE_BUTTON_2))
        scene->camera->transform.rotateLocal({ -mouse_delta.y, 0, -mouse_delta.x });

    glm::mat4 camera_matrix = scene->camera->transform.getMatrix();
    glm::vec3 local_move_vector = glm::vec3{
        Input::getAxis('A', 'D'),
        Input::getAxis('Q', 'E'),
        Input::getAxis('W', 'S')
    } *0.02f;
    if (Input::isKeyDown(GLFW_KEY_LEFT_SHIFT))
        local_move_vector *= 3.0f;
    scene->camera->transform.translateLocal(camera_matrix * glm::vec4(local_move_vector, 0));
}

int main() {
    Window::initEnvironment();
    Ref<Window> window = new Window(1024, 1024, "hop!");
    Input::init(window->getWindow());
    Ref<GraphicsEnvironment> ge = new GraphicsEnvironment(window);
    Package::init();
    Package::loadPackage("resources.hop");

    ge->scene = new Scene();
    initNodeScene(ge->scene);

    while (!window->getShouldClose())
    {
        window->pollEvents();
        ge->drawFrame();
        updateNodeScene(ge->scene);
    }

    cube = nullptr;
    asha = nullptr;

    ge = nullptr;
    window = nullptr;
    Window::terminateEnvironment();

    return 0;
}