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
#include <random>
#include <thread>
#include <string>
#include <map>
#if defined(_WIN32)
#undef APIENTRY
#include <Windows.h>
#endif
#include "../resource.h"

#include "hop_engine.h"
#include "node_view.h"

#include "token_file.h"
#include "pbr.h"
#include "gizmo.h"

using namespace HopEngine;

WeakRef<StaticMesh> asha;
WeakRef<StaticMesh> obj;
WeakRef<NodeView> node_view;
WeakRef<Gizmo> gizmo;
WeakRef<NodeView::Node> selected_node;

Spline camera_spline;

void initScene(Ref<Scene> scene)
{
    Ref<Shader> shader = new Shader("res://psx", false);
    Ref<Sampler> sampler = new Sampler(SamplerBuilder().filter(VK_FILTER_NEAREST));
    asha = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::keepLoaded(new Mesh("res://asha/asha.obj")),
        Engine::keepLoaded(new Material(
            shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE)
        ))));
    asha->material->setTexture("albedo", new Texture("res://asha/asha.png"));
    asha->material->setSampler("albedo", sampler);
    asha->transform.setLocalPosition({ 0, 0, -0.9f });

    Ref<StaticMesh> bunny = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::keepLoaded(new Mesh("res://bunny.obj")),
        Engine::keepLoaded(new Material(shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE)))
    ));
    bunny->material->setTexture("albedo", new Texture("res://bunny.png"));
    bunny->material->setSampler("albedo", sampler);
    bunny->setParent(asha);
    bunny->transform.setLocalPosition({ 0, -0.5f, 0.9f });
    bunny->transform.scaleLocal({ 2, 2, 2 });

    Ref<StaticMesh> tux = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::keepLoaded(new Mesh("res://tux.obj")),
        Engine::keepLoaded(new Material(shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE)))
    ));
    tux->material->setTexture("albedo", new Texture("res://tux.png"));
    tux->material->setSampler("albedo", sampler);
    tux->transform.translateLocal({ 2, 0, 0 });

    auto sun_lamp = scene->insertObject<Light>(new Light(Light::DIRECTIONAL));
    sun_lamp->transform.rotateLocal({ -17.0f, -34.0f, -189.0f });
    sun_lamp->colour = { 1.0f, 1.0f, 1.0f, 0.0f };

    camera_spline.loop = true;
    camera_spline.points = { { 0, -1, 0.5f }, { 1, 0, 0.5f }, { 0, 1, 0.5f }, { -1, 0, 0.5f } };

    scene->getCamera(0)->transform.lookAt(glm::vec3(0.5f, -1.5f, 0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));

    gizmo = scene->insertObject<Gizmo>(new Gizmo());
    scene->skybox = Engine::keepLoaded(new Texture("res://nasa_goddard_gaia_dr2_deep_star_map.png"));

    Ref<Camera> second_cam = new Camera();
    scene->insertObject(second_cam);
    scene->setCameraSlot(second_cam, 1);
    second_cam->transform.lookAt({ 0, -2.0f, 0.7f }, { 0, 0, 0.7f }, { 0, 0, 1 });
    second_cam->fov = 20.0f;

    Ref<Camera> third_cam = new Camera();
    scene->insertObject(third_cam);
    scene->setCameraSlot(third_cam, 2);
    third_cam->transform.lookAt({ 0, -0.2f, 0.8f }, { 0, 0, 0.7f }, { 0, 0, 1 });
    third_cam->fov = 120.0f;

    scene->render_graph = new RenderGraph(RenderGraphBuilder()
        .addCamera(0)
        .addCamera(1)
        .addCamera(2, 0.1f)
        .addPostProcess(new Shader("res://engine/ssao", false), {
            { 0, RenderTextureBinding(0, 1) },
            { 1, RenderTextureBinding(0, 4) }
            }, 0.25f)
        .addPostProcess(new Shader("res://half_and_half", false), {
            { 0, RenderTextureBinding(0, 0) },
            { 1, RenderTextureBinding(1, 1).address(VK_SAMPLER_ADDRESS_MODE_REPEAT) },
            { 2, RenderTextureBinding(2, 0).address(VK_SAMPLER_ADDRESS_MODE_REPEAT).filter(VK_FILTER_NEAREST) } })
        .addPostProcess(new Shader("res://engine/colour_correct", false), {
            { 0, RenderTextureBinding(4, 0).filter(VK_FILTER_NEAREST) } })
    );

    glm::vec4 samples[64];
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine rand;
    for (int i = 0; i < 64; ++i)
    {
        glm::vec4 s = glm::vec4((dist(rand) * 2.0f) - 1.0f, (dist(rand) * 2.0f) - 1.0f, -dist(rand), 0.0f);
        float fi = (float)i / (float)64;
        glm::vec4 v = glm::normalize(s * (0.05f + ((1.0f - 0.05f) * fi * fi)));
        samples[i] = v;
    }
    scene->render_graph->getMaterialForStep(3)->setUniform("samples", samples, sizeof(glm::vec4) * 64);
    scene->render_graph->getMaterialForStep(5)->setFloatUniform("gamma", 2.2f);
    scene->render_graph->getMaterialForStep(5)->setFloatUniform("exposure", 16.0f);
    scene->render_graph->getMaterialForStep(5)->setFloatUniform("offset", 0.0f);

    Engine::debugClearSelection(asha.cast<Object>(), asha->material);
}

void updateScene(Ref<Scene> scene, float delta_time)
{
    static float total_time = 0;
    total_time += delta_time;

    Engine::debugCamera(delta_time);

    if (obj)
        obj->transform.rotateLocal({ 0, 0, 20 * delta_time });

    gizmo->trackObject(Engine::getDebugSelection(), scene->getCamera(0));

    Input::resetMouseDelta();
}

void initNodeScene(Ref<Scene> scene)
{
    node_view = scene->insertObject<NodeView>(new NodeView());
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

    node_view->links.push_back({ node_view->nodes[4], 1, node_view->nodes[1], 1, 1 });
    node_view->links.push_back({ node_view->nodes[2], 0, node_view->nodes[1], 0, 2 });
    node_view->links.push_back({ node_view->nodes[5], 0, node_view->nodes[0], 0, 3 });

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
    style.palette =
    {
        { 0.010f, 0.010f, 0.010f },
        { 0.091f, 0.610f, 0.973f },
        { 1.000f, 1.000f, 1.000f },
        { 0.930f, 0.392f, 0.479f }
    };
    node_view->setStyle(style);

    scene->getCamera(0)->transform.lookAt({ 0, 0, 6 }, { 0, 0, 0 }, { 0, 1, 0 });
    scene->getCamera(0)->clear_colour = {0, 0, 0};
}

void updateNodeScene(Ref<Scene> scene, float delta_time)
{
    bool node_view_dirty = false;

    if (Input::wasMousePressed(GLFW_MOUSE_BUTTON_LEFT))
    {
        if (selected_node)
            selected_node->highlighted = false;
        glm::vec2 camera_pos = scene->getCamera(0)->transform.getLocalPosition();
        glm::vec2 mouse_screen_pos = Input::getMousePosition() - (RenderServer::getFramebufferSize() * 0.5f);
        glm::vec2 mouse_world_pos = mouse_screen_pos + (camera_pos * RenderServer::getFramebufferSize() * 0.5f);
        selected_node = node_view->select(mouse_world_pos);
        if (selected_node)
            selected_node->highlighted = true;
        node_view_dirty = true;
    }

    glm::vec2 mouse_delta = Input::getMouseDelta() * 0.025f;
    float move_x = Input::getAxis(GLFW_KEY_LEFT, GLFW_KEY_RIGHT);
    float move_y = Input::getAxis(GLFW_KEY_UP, GLFW_KEY_DOWN);
    if (Input::isMouseDown(GLFW_MOUSE_BUTTON_RIGHT))
    {
        glm::vec2 mouse_world_delta = glm::vec2{ -mouse_delta.x, -mouse_delta.y };// / RenderServer::getFramebufferSize();
        scene->getCamera(0)->transform.translateLocal({mouse_world_delta.x, mouse_world_delta.y, 0});
    }
    else if (Input::isMouseDown(GLFW_MOUSE_BUTTON_LEFT))
    {
        move_x = mouse_delta.x * 20.0f;
        move_y = mouse_delta.y * 20.0f;
        node_view_dirty = true;
    }

    if (move_x != 0 || move_y != 0)
    {
        if (selected_node)
        {
            selected_node->position += glm::vec2{ move_x, move_y } * 0.5f;
            node_view_dirty = true;
        }
    }

    if (node_view_dirty)
        node_view->updateMesh();
    Input::resetMouseDelta();
}

void initMaterialScene(Ref<Scene> scene)
{
    Ref<Shader> shader = new Shader("res://pbr", false);
    Ref<Sampler> sampler = new Sampler(SamplerBuilder().filter(VK_FILTER_NEAREST));
    obj = scene->insertObject<StaticMesh>(new StaticMesh(
        new Mesh("res://cube.obj"),
        new Material(shader)
    ));
    obj->material->setTexture("albedo_tex", new Texture("res://icon.png"));
    obj->material->setTexture("normal_map", new Texture("res://BlackBricks_n.png"));
    MaterialParams material;
    obj->material->setUniform("material", &material, sizeof(MaterialParams));

    auto sun_lamp = scene->insertObject<Light>(new Light(Light::DIRECTIONAL));
    sun_lamp->transform.setLocalPosition({ 1, 0, 2 });
    sun_lamp->transform.rotateLocal({ 20.0f, 30.0f, 0.0f });
    sun_lamp->colour = { 0.4f, 0.4f, 0.4f, 0.0f };

    auto other_lamp = scene->insertObject<Light>(new Light(Light::SPOT));
    other_lamp->transform.setLocalPosition({ 0, -2, 2 });
    other_lamp->colour = { 200.0f, 0.0f, 0.0f, 0.0f };
    other_lamp->spot_angle = 15.0f;
    other_lamp->transform.rotateLocal({ 45.0f, 0.0f, 0.0f });

    scene->getCamera(0)->transform.lookAt(glm::vec3(0.5f, -1.5f, 0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));
}

static int selected_scene = 0;

void imGuiDrawFunc(Ref<Scene> scene, float delta_time)
{
    ImGui::Begin("colour correction", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    if (selected_scene == 0)
    {
        WeakRef<Material> mat = scene->render_graph->getMaterialForStep(5);
        if (mat)
        {
            static float gamma = 1.2f;
            static float exposure = 1.5f;
            static float offset = 0.0f;
            ImGui::SliderFloat("gamma", &gamma, 0.001f, 4.0f);
            ImGui::SliderFloat("exposure", &exposure, 0.001f, 16.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat("offset", &offset, -2.0f, 2.0f);
            mat->setFloatUniform("gamma", gamma);
            mat->setFloatUniform("exposure", exposure);
            mat->setFloatUniform("offset", offset);
        }
        else
            ImGui::Text("couldn't find the colour correction step!");
    }
    ImGui::End();

    Engine::drawImGuiDebug(delta_time);
}

struct SceneFuncSet
{
    std::wstring name;
    void(*init_func)(Ref<Scene>);
    void(*update_func)(Ref<Scene>, float);
    void(*imgui_func)(Ref<Scene>, float);
};

static std::vector<SceneFuncSet> scenes =
{
    { L"bunnygirl", initScene, updateScene, imGuiDrawFunc },
    { L"nodes", initNodeScene, updateNodeScene, imGuiDrawFunc },
    { L"material", initMaterialScene, updateScene, imGuiDrawFunc },
};

#if defined(_WIN32)
INT_PTR dialogFunc(HWND handle, UINT message, WPARAM unnamedParam3, LPARAM unnamedParam4)
{
    HWND list = GetDlgItem(handle, IDC_LIST1);
    switch (message)
    {
    case WM_INITDIALOG:
        for (const auto& scene : scenes)
        {
            SendMessage(list, LB_ADDSTRING, 0, (LPARAM)scene.name.c_str());
        }
        SendMessage(handle, WM_SETICON, ICON_SMALL, (LPARAM)MAKEINTRESOURCE(IDI_ICON1));
        SendMessage(handle, WM_SETICON, ICON_BIG, (LPARAM)MAKEINTRESOURCE(IDI_ICON1));
        return true;
    case WM_COMMAND:
        if (unnamedParam3 == 2)
            exit(0);
        if (unnamedParam3 == 1)
            EndDialog(handle, true);
        else
        {
            auto param = (unnamedParam3 & 0xFFFF0000) >> 16;
            if (param == 1)
                selected_scene = (int)SendMessage(list, LB_GETCURSEL, 0, 0);
            else if (param == 2)
                EndDialog(handle, true);
        }
        return true;
    }

    return false;
}
#endif

int main()
{
#if defined(_WIN32)
    system(".\\package-builder\\bin\\x64\\Release\\package-builder.exe res -c resources.hop");
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    while (true)
    {
#if defined(_WIN32)
        DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, dialogFunc);
#endif
        Engine::init();
        Engine::debugClearSelection();

        const auto& scene = scenes[selected_scene];

        Engine::setup(scene.init_func, scene.update_func, scene.imgui_func);

        Engine::mainLoop();

        Engine::debugClearSelection();
        Engine::destroy();

        return 0;
    }

    return 0;
}