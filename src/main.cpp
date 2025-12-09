#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <Windows.h>

#include "hop_engine.h"
#include "node_view.h"

using namespace HopEngine;

Ref<Object> asha;
Ref<Object> cube;
size_t node_index = 0;
Ref<NodeView> node_view;

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
    node_view = new NodeView();
    node_view->nodes.push_back(
        { "Hello, World!",
        {
            { "Outputs on right", NodeView::ELEMENT_OUTPUT },
            { "text 6px inwards", NodeView::ELEMENT_OUTPUT },
            { "text 4px down", NodeView::ELEMENT_OUTPUT },
            { "Inputs on the left", NodeView::ELEMENT_INPUT },
            { "", NodeView::ELEMENT_SPACE },
            { "mixed-width font!", NodeView::ELEMENT_BLOCK },
            { "above is a banner", NodeView::ELEMENT_TEXT },
            { "extra bottom spacing", NodeView::ELEMENT_TEXT },
        }, { 0, 0 }, 1, true });
    node_view->nodes.push_back(
        { "multiply",
        {
            { "result", NodeView::ELEMENT_OUTPUT },
            { "input a", NodeView::ELEMENT_INPUT },
            { "input b", NodeView::ELEMENT_INPUT },
        }, { 13, 4 }, 2 });
    node_view->nodes.push_back(
        { "add",
        {
            { "result", NodeView::ELEMENT_OUTPUT },
            { "input a", NodeView::ELEMENT_INPUT },
            { "input b", NodeView::ELEMENT_INPUT },
        }, { -6, 0 }, 3 });
    node_view->nodes.push_back(
        { "multiply add",
        {
            { "result", NodeView::ELEMENT_OUTPUT },
            { "input a", NodeView::ELEMENT_INPUT },
            { "input b", NodeView::ELEMENT_INPUT },
            { "input c", NodeView::ELEMENT_INPUT },
        }, { -6, 10 }, 4 });
    node_view->nodes.push_back(
        { "make vec3",
        {
            { "vector", NodeView::ELEMENT_OUTPUT },
            { "length", NodeView::ELEMENT_OUTPUT },
            { "normalised", NodeView::ELEMENT_OUTPUT },
            { "x", NodeView::ELEMENT_INPUT },
            { "y", NodeView::ELEMENT_INPUT },
            { "z", NodeView::ELEMENT_INPUT },
        }, { -6, -10 }, 5 });
    node_view->material->setBoolUniform("debug_segments", false);
    node_view->updateMesh();
    scene->objects.push_back(node_view.cast<Object>());

    scene->camera->transform.lookAt({ 0, 0, 6 }, { 0, 0, 0 }, { 0, 1, 0 });
    scene->background_colour = { 0, 0, 0 };
}

void updateNodeScene(Ref<Scene> scene)
{
    glm::vec2 mouse_delta = Input::getMouseDelta() * 0.004f;
    float move_x = Input::getAxis(GLFW_KEY_LEFT, GLFW_KEY_RIGHT);
    float move_y = Input::getAxis(GLFW_KEY_UP, GLFW_KEY_DOWN);
    if (Input::isMouseDown(GLFW_MOUSE_BUTTON_RIGHT))
        scene->camera->transform.translateLocal({ -mouse_delta.x * 0.5f, -mouse_delta.y * 0.5f, 0 });
    else if (Input::isMouseDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        move_x = mouse_delta.x * 20.0f;
        move_y = mouse_delta.y * 20.0f;
    }

    if (move_x != 0 || move_y != 0)
    {
        node_view->nodes[node_index].position += glm::vec2{ move_x, move_y } * 0.5f;
        node_view->updateMesh();
    }

    if (Input::isKeyDown(GLFW_KEY_TAB))
    {
        node_view->nodes[node_index].highlighted = false;
        node_index = (node_index + 1) % node_view->nodes.size();
        node_view->nodes[node_index].highlighted = true;
        node_view->updateMesh();
    }
}

void imGuiDrawFunc()
{
    ImGui::Begin("test");
    ImGui::Text("yippee");
    ImGui::End();
}

int main()
{
    Debug::init(Debug::DEBUG_FAULT);
    Window::initEnvironment();
    Ref<Window> window = new Window(1024, 1024, "hop!");
    Input::init(window->getWindow());
    Ref<GraphicsEnvironment> ge = new GraphicsEnvironment(window);
    Package::init();
    Package::loadPackage("resources.hop");

    ge->draw_imgui_function = imGuiDrawFunc;
    ge->scene = new Scene();
    initNodeScene(ge->scene);

    while (!window->getShouldClose())
    {
        window->pollEvents();
        if (window->isMinified())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ge->drawFrame();
        updateNodeScene(ge->scene);
    }

    node_view = nullptr;
    cube = nullptr;
    asha = nullptr;

    ge = nullptr;
    window = nullptr;
    Window::terminateEnvironment();

    Debug::close();

    return 0;
}