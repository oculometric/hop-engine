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
#include <string>

#include "hop_engine.h"
#include "node_view.h"

using namespace HopEngine;

Ref<Object> asha;
Ref<Object> cube;
Ref<NodeView> node_view;
Ref<NodeView::Node> selected_node;

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
    node_view->nodes.push_back(new NodeView::Node
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
        }, { 5, -10 }, 1 });
    node_view->nodes.push_back(new NodeView::Node
        { "multiply",
        {
            { "result", NodeView::ELEMENT_OUTPUT },
            { "input a", NodeView::ELEMENT_INPUT },
            { "input b", NodeView::ELEMENT_INPUT },
        }, { 13, 4 }, 2 });
    node_view->nodes.push_back(new NodeView::Node
        { "add",
        {
            { "result", NodeView::ELEMENT_OUTPUT },
            { "input a", NodeView::ELEMENT_INPUT },
            { "input b", NodeView::ELEMENT_INPUT },
        }, { -6, 0 }, 3 });
    node_view->nodes.push_back(new NodeView::Node
        { "multiply add",
        {
            { "result", NodeView::ELEMENT_OUTPUT },
            { "input a", NodeView::ELEMENT_INPUT },
            { "input b", NodeView::ELEMENT_INPUT },
            { "input c", NodeView::ELEMENT_INPUT },
        }, { -6, 10 }, 4 });
    node_view->nodes.push_back(new NodeView::Node
        { "make vec3",
        {
            { "vector", NodeView::ELEMENT_OUTPUT, 1 },
            { "length", NodeView::ELEMENT_OUTPUT },
            { "normalised", NodeView::ELEMENT_OUTPUT, 3, false },
            { "x", NodeView::ELEMENT_INPUT, 0, false },
            { "y", NodeView::ELEMENT_INPUT, 0, false },
            { "z", NodeView::ELEMENT_INPUT, 0, false },
        }, { -6, -10 }, 5 });
    node_view->nodes.push_back(new NodeView::Node
        { "kill john lennon",
        {
            { "", NodeView::ELEMENT_INPUT, 4, false },
            { "execution?", NodeView::ELEMENT_OUTPUT, 5 },
            { "hello", NodeView::ELEMENT_INPUT, 0, false },
        }, { -6, -15 }, 6 });

    node_view->links.push_back({ node_view->nodes[4], 1, node_view->nodes[1], 1 });
    node_view->links.push_back({ node_view->nodes[2], 0, node_view->nodes[1], 0 });
    node_view->links.push_back({ node_view->nodes[5], 0, node_view->nodes[0], 0 });

    node_view->material->setBoolUniform("debug_segments", false);
    node_view->updateMesh();

    auto style = node_view->getStyle();
    style.use_dynamic_background = true;
    /*style.palette =
    {
        { 0.018f, 0.018f, 0.018f },
        { 0.863f, 0.624f, 0.068f },
        { 0.694f, 0.091f, 0.019f },
        { 0.604f, 0.044f, 0.025f },
        { 0.337f, 0.025f, 0.058f },
        { 0.159f, 0.037f, 0.078f },
    };*/
    node_view->setStyle(style);

    scene->objects.push_back(node_view.cast<Object>());

    scene->camera->transform.lookAt({ 0, 0, 6 }, { 0, 0, 0 }, { 0, 1, 0 });
    scene->background_colour = { 0, 0, 0 };
}

void updateNodeScene(Ref<Scene> scene)
{
    bool node_view_dirty = false;

    if (Input::wasMousePressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        if (selected_node.isValid())
            selected_node->highlighted = false;
        glm::vec2 camera_pos = scene->camera->transform.getLocalPosition();
        glm::vec2 mouse_screen_pos = Input::getMousePosition() - (GraphicsEnvironment::get()->getFramebufferSize() * 0.5f);
        glm::vec2 mouse_world_pos = mouse_screen_pos + (camera_pos * GraphicsEnvironment::get()->getFramebufferSize() * 0.5f);
        selected_node = node_view->select(mouse_world_pos);
        if (selected_node.isValid())
            selected_node->highlighted = true;
        node_view_dirty = true;
    }

    glm::vec2 mouse_delta = Input::getMouseDelta() * 0.004f;
    float move_x = Input::getAxis(GLFW_KEY_LEFT, GLFW_KEY_RIGHT);
    float move_y = Input::getAxis(GLFW_KEY_UP, GLFW_KEY_DOWN);
    if (Input::isMouseDown(GLFW_MOUSE_BUTTON_RIGHT))
    {
        glm::vec2 mouse_world_delta = glm::vec2{ -mouse_delta.x * 512.0f, -mouse_delta.y * 512.0f } / GraphicsEnvironment::get()->getFramebufferSize();
        scene->camera->transform.translateLocal({ mouse_world_delta.x, mouse_world_delta.y, 0 });
    }
    else if (Input::isMouseDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        move_x = mouse_delta.x * 20.0f;
        move_y = mouse_delta.y * 20.0f;
        node_view_dirty = true;
    }

    if (move_x != 0 || move_y != 0)
    {
        if (selected_node.isValid())
        {
            selected_node->position += glm::vec2{ move_x, move_y } * 0.5f;
            node_view_dirty = true;
        }
    }

    if (node_view_dirty)
        node_view->updateMesh();

}

void imGuiDrawFunc()
{
    ImGui::Begin("test");
    ImGui::Text("yippee");
    ImGui::End();
}

int main()
{
    system("package-builder.exe res -c resources.hop");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Debug::init(Debug::DEBUG_FAULT);
    Window::initEnvironment();
    Ref<Window> window = new Window(1024, 1024, "hop!");
    Input::init(window->getWindow());
    Package::init();
    Package::loadPackage("resources.hop");
    Ref<GraphicsEnvironment> ge = new GraphicsEnvironment(window);

    ge->draw_imgui_function = imGuiDrawFunc;
    ge->scene = new Scene();
    initScene(ge->scene);

    while (!window->getShouldClose())
    {
        window->pollEvents();
        if (window->isMinified())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (window->isResized())
            ge->resizeSwapchain();
        ge->drawFrame();
        updateScene(ge->scene);
    }

    ge->scene = nullptr;

    node_view = nullptr;
    cube = nullptr;
    asha = nullptr;

    ge = nullptr;
    window = nullptr;
    Window::terminateEnvironment();

    Debug::close();

    return 0;
}