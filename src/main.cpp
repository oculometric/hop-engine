#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <vulkan/vulkan.h>
#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <string>
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
#include "main.h"

using namespace HopEngine;

static WeakRef<StaticMesh> asha;
static WeakRef<StaticMesh> obj;
static WeakRef<StaticMesh> obj2;
static WeakRef<StaticMesh> obj3;
static WeakRef<StaticMesh> obj4;
static WeakRef<NodeView> node_view;
static WeakRef<Gizmo> gizmo;
static WeakRef<NodeView::Node> selected_node;
static WeakRef<Material> cc_material;

static Ref<Scene> initScene()
{
    Ref<Scene> scene = new Scene();
    Ref<Shader> shader = Engine::loadShader("res://psx");
    Ref<Sampler> sampler = new Sampler(SamplerBuilder().filter(VK_FILTER_NEAREST));
    asha = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::loadMesh("res://samples/asha.obj"),
        Engine::keepLoaded(new Material(
            shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE).stencilWrite(1)
        ))));
    asha->material->setTexture("albedo", Engine::loadTexture("res://samples/asha.png"));
    asha->material->setSampler("albedo", sampler);
    asha->transform.setLocalPosition({ 0, 0, -0.9f });

    Ref<StaticMesh> bunny = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::loadMesh("res://samples/bunny.obj"),
        Engine::keepLoaded(new Material(shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE).stencilWrite(2)
        ))));
    bunny->material->setTexture("albedo", Engine::loadTexture("res://samples/bunny.png"));
    bunny->material->setSampler("albedo", sampler);
    bunny->setParent(asha);
    bunny->transform.setLocalPosition({ 0, -0.5f, 0.9f });
    bunny->transform.scaleLocal({ 2, 2, 2 });

    Ref<StaticMesh> tux = scene->insertObject<StaticMesh>(new StaticMesh(
        Engine::loadMesh("res://tux.obj"),
        Engine::keepLoaded(new Material(shader, PipelineBuilder().cullMode(VK_CULL_MODE_NONE)))
    ));
    tux->material->setTexture("albedo", Engine::loadTexture("res://tux.png"));
    tux->material->setSampler("albedo", sampler);
    tux->transform.translateLocal({ 2, 0, 0 });

    auto sun_lamp = scene->insertObject<Light>(new Light(Light::DIRECTIONAL));
    sun_lamp->transform.rotateLocal({ -17.0f, -34.0f, -189.0f });
    sun_lamp->colour = { 1.0f, 1.0f, 1.0f };

    scene->getCamera(0)->transform.lookAt(glm::vec3(0.5f, -1.5f, 0.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f));

    gizmo = scene->insertObject<Gizmo>(new Gizmo());
    scene->skybox = Engine::loadTexture("res://samples/nasa_goddard_gaia_dr2_deep_star_map.png");

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

    scene->render_graph = RenderGraph::deserialise("res://test_graph.hrgr");

    glm::vec4 samples[24];
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine rand;
    for (int i = 0; i < 24; ++i)
    {
        glm::vec4 s = glm::vec4((dist(rand) * 2.0f) - 1.0f, (dist(rand) * 2.0f) - 1.0f, -dist(rand), 0.0f);
        const float fi = static_cast<float>(i) / 24.0f;
        glm::vec4 v = glm::normalize(s * (0.05f + ((1.0f - 0.05f) * fi * fi)));
        samples[i] = v;
    }
    scene->render_graph->getMaterialForStep(3)->setUniform("samples", samples, sizeof(glm::vec4) * 16);
    cc_material = scene->render_graph->getMaterialForStep(5);
    cc_material->setFloatUniform("gamma", 1.0f);
    cc_material->setFloatUniform("exposure", 1.0f);
    cc_material->setFloatUniform("offset", 0.0f);
    
    Engine::debugClearSelection(asha.cast<Object>(), asha->material, scene->getCamera(0));
    return scene;
}

static void updateScene(Ref<Scene> scene, float delta_time)
{
    static float total_time = 0;
    total_time += delta_time;

    Engine::debugCamera(delta_time);

    if (obj)
        obj->transform.rotateLocal({ 0, 0, 20 * delta_time });
    if (obj2)
        obj2->transform.rotateLocal({ 35 * delta_time, 0, 0 });
    if (obj3)
        obj3->transform.rotateLocal({ 0, 0, 10 * delta_time });
    if (obj4)
        obj4->transform.rotateLocal({ 0, 0, -16 * delta_time });

    if (gizmo)
        gizmo->trackObject(Engine::getDebugSelection(), scene->getCamera(0));

    Input::resetMouseDelta();
}

static Ref<Scene> initNodeScene()
{
    Ref<Scene> scene = new Scene();
    node_view = scene->insertObject<NodeView>(new NodeView());
    node_view->nodes.push_back(new NodeView::Node
        { "Hello, World!",
        {
            NodeView::NodeElement("Outputs on right", NodeView::ELEMENT_OUTPUT),
            NodeView::NodeElement( "text 6px inwards", NodeView::ELEMENT_OUTPUT),
            NodeView::NodeElement( "text 4px down", NodeView::ELEMENT_OUTPUT),
            NodeView::NodeElement( "Inputs on the left", NodeView::ELEMENT_INPUT),
            NodeView::NodeElement( "", NodeView::ELEMENT_SPACE),
            NodeView::NodeElement( "mixed-width font!", NodeView::ELEMENT_BLOCK),
            NodeView::NodeElement( "above is a banner", NodeView::ELEMENT_TEXT),
            NodeView::NodeElement( "extra bottom spacing", NodeView::ELEMENT_TEXT),
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
    
    return scene;
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

Ref<Scene> initMuseumScene()
{
    Debug::setLogLevel(Debug::DEBUG_WARNING);
    Ref<Scene> scene = Scene::deserialise("res://museum/Museum.hscn");
    if (!scene) return scene;
    
    obj2 = scene->findObject<StaticMesh>("orrery_mid");
    obj = scene->findObject<StaticMesh>("orrery_core");
    obj3 = scene->findObject<StaticMesh>("orrery_orbit_a");
    obj4 = scene->findObject<StaticMesh>("orrery_orbit_b");
    
    auto fog_mat = scene->render_graph->getMaterialForStep(2);
    fog_mat->setFloatUniform("fog_start", 4.0f);
    fog_mat->setFloatUniform("fog_end", 24.0f);
    fog_mat->setFloatUniform("fog_exponent", 0.5f);
    fog_mat->setVec4Uniform("fog_colour", { 0.005, 0.006, 0.002, 0 });
    
    constexpr int SAMPLES_COUNT = 24;
    glm::vec4 samples[SAMPLES_COUNT];
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    std::default_random_engine rand;
    for (int i = 0; i < SAMPLES_COUNT; ++i)
    {
        glm::vec3 s = glm::vec3(
            (dist(rand) * 2.0f) - 1.0f,
            (dist(rand) * 2.0f) - 1.0f,
            dist(rand));
        s = glm::normalize(s);
        const float fi = static_cast<float>(i) / SAMPLES_COUNT;
        glm::vec3 v = s * glm::mix(0.1f, 1.0f, fi * fi);
        samples[i] = glm::vec4(v, 0.0f);
    }
    scene->render_graph->getMaterialForStep(3)->setUniform("samples", samples, sizeof(glm::vec4) * SAMPLES_COUNT);
    
    cc_material = scene->render_graph->getMaterialForStep(5);
    cc_material->setFloatUniform("gamma", 1.0f);
    cc_material->setFloatUniform("exposure", 1.0f);
    cc_material->setFloatUniform("offset", 0.0f);
    cc_material->setTexture("lut", Engine::loadTexture3D("res://museum/lut.png", 8, 8));
    cc_material->setSampler("lut", new Sampler(SamplerBuilder().address(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)));
    cc_material->setFloatUniform("use_lut", 1);
    Engine::debugClearSelection(WeakRef<Object>(), WeakRef<Material>(), scene->getCamera(0));

    return scene;
}

static int selected_scene = 0;

void imGuiDrawFunc(Ref<Scene> scene, float delta_time)
{
    if (cc_material)
    {
        ImGui::Begin("colour correction", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        static float gamma = 0;
        static float new_gamma = 1.0f;
        static float exposure = 0;
        static float new_exposure = 1.0f;
        static float offset = 0;
        static float new_offset = 0.0f;
        ImGui::SliderFloat("gamma", &new_gamma, 0.001f, 4.0f);
        ImGui::SliderFloat("exposure", &new_exposure, 0.001f, 16.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
        ImGui::SliderFloat("offset", &new_offset, -2.0f, 2.0f);
        if (gamma != new_gamma)
            cc_material->setFloatUniform("gamma", new_gamma);
        if (exposure != new_exposure)
            cc_material->setFloatUniform("exposure", new_exposure);
        if (offset != new_offset)
            cc_material->setFloatUniform("offset", new_offset);
        gamma = new_gamma;
        exposure = new_exposure;
        offset = new_offset;
        ImGui::End();
    }
}

static std::vector<SceneFuncSet> scenes =
{
    { L"bunnygirl", initScene, updateScene, imGuiDrawFunc },
    { L"nodes", initNodeScene, updateNodeScene, imGuiDrawFunc },
    { L"museum", initMuseumScene, updateScene, imGuiDrawFunc },
};

SceneFuncSet getScene(int i)
{
    return scenes[i];
}

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
    Engine::init();
    
    selected_scene = 2;
// #if defined(_WIN32)
//     DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG1), NULL, dialogFunc);
// #endif
    Engine::debugClearSelection();

    const auto& scene = getScene(selected_scene);

    Engine::setup(scene.init_func, scene.update_func, scene.imgui_func);

    Engine::mainLoop();

    Engine::debugClearSelection();
    Engine::destroy();

    return 0;
}