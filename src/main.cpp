#if !defined(STANDALONE)

#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

SceneFuncSet funcs;
WeakRef<NodeView> node_view;
Ref<Scene> node_scene;
Ref<Scene> main_scene;

void updateFunc(float delta)
{
    node_view->checkInput({ 0, node_scene->getViewportSize().y * 3.0f }, node_scene->getViewportSize());
    if (Input::getMousePosition().y < node_scene->getViewportSize().y * 3.0f)
        funcs.update_func(delta);
};

int main()
{
    Engine::init();

    Package::loadPackage("resources.hop");
    funcs = getAshaScene();
    main_scene = funcs.init_func();

    node_scene = Scene::create();
    node_view = node_scene->insertObject<NodeView>(NodeView::create());
    node_view->nodes.push_back(new NodeView::Node
        { "Camera",
        {
            { "main_cam", NodeView::ELEMENT_TEXT },
            { "slot: 0", NodeView::ELEMENT_TEXT },
            { "colour", NodeView::ELEMENT_OUTPUT },
            { "data_0", NodeView::ELEMENT_OUTPUT },
            { "data_1", NodeView::ELEMENT_OUTPUT },
            { "data_2", NodeView::ELEMENT_OUTPUT },
            { "depth", NodeView::ELEMENT_OUTPUT },
        }, glm::vec2{ -16, -5 }, glm::vec2{ 4, 1 }, glm::vec3{ 1, 0, 0 } });
    node_view->nodes.push_back(new NodeView::Node
        { "Camera",
        {
            { "right_cam", NodeView::ELEMENT_TEXT },
            { "slot: 1", NodeView::ELEMENT_TEXT },
            { "colour", NodeView::ELEMENT_OUTPUT },
            { "data_0", NodeView::ELEMENT_OUTPUT },
            { "data_1", NodeView::ELEMENT_OUTPUT },
            { "data_2", NodeView::ELEMENT_OUTPUT },
            { "depth", NodeView::ELEMENT_OUTPUT },
        }, glm::vec2{ -11, -4 }, glm::vec2{ 4, 1 }, glm::vec3{ 1, 0, 0 } });
    node_view->nodes.push_back(new NodeView::Node
        { "Camera",
        {
            { "ring_doorbell", NodeView::ELEMENT_TEXT },
            { "slot: 2", NodeView::ELEMENT_TEXT },
            { "colour", NodeView::ELEMENT_OUTPUT },
            { "data_0", NodeView::ELEMENT_OUTPUT },
            { "data_1", NodeView::ELEMENT_OUTPUT },
            { "data_2", NodeView::ELEMENT_OUTPUT },
            { "depth", NodeView::ELEMENT_OUTPUT },
        }, glm::vec2{ -6, -3 }, glm::vec2{ 5, 1 }, glm::vec3{ 1, 0, 0 } });
    node_view->nodes.push_back(new NodeView::Node
        { "Shader",
        {
            { "ssao", NodeView::ELEMENT_TEXT },
            { "shader", NodeView::ELEMENT_INPUT, 3 },
            { "tex_norm", NodeView::ELEMENT_INPUT },
            { "tex_depth", NodeView::ELEMENT_INPUT },
            { "colour", NodeView::ELEMENT_OUTPUT },
        }, glm::vec2{ 0, -1 }, glm::vec2{ 4, 1 }, glm::vec3{ 0, 1, 0.8f } });
    node_view->nodes.push_back(new NodeView::Node
        { "Shader",
        {
            { "multi_composite", NodeView::ELEMENT_TEXT },
            { "shader", NodeView::ELEMENT_INPUT, 3 },
            { "tex_a", NodeView::ELEMENT_INPUT },
            { "tex_b", NodeView::ELEMENT_INPUT },
            { "tex_c", NodeView::ELEMENT_INPUT },
            { "colour", NodeView::ELEMENT_OUTPUT },
        }, glm::vec2{ 5, -3 }, glm::vec2{ 6, 1 }, glm::vec3{ 0, 1, 0.8f } });
    node_view->nodes.push_back(new NodeView::Node
        { "Shader",
        {
            { "final_pass", NodeView::ELEMENT_TEXT },
            { "shader", NodeView::ELEMENT_INPUT, 3 },
            { "tex", NodeView::ELEMENT_INPUT },
            { "colour", NodeView::ELEMENT_OUTPUT },
        }, glm::vec2{ 12, 0 }, glm::vec2{ 5, 1 }, glm::vec3{ 0, 1, 0.8f } });
    node_view->nodes.push_back(new NodeView::Node
        { "Screen",
        {
            { "texture", NodeView::ELEMENT_INPUT },
        }, glm::vec2{ 18, 3 }, glm::vec2{ 4, 1 }, glm::vec3{ 0.7f, 0.2f, 0 } });
    node_view->setStyle(node_view->getStyle());
    
    RenderServer::setMultiScene(
        {
            { main_scene, { 0, 0 }, { 1, 0.75f } },
            { node_scene, { 0, 0.75f }, { 1, 0.25f } }
        });

    Engine::setImGuiFunc(funcs.imgui_func);
    Engine::start(&updateFunc);

    node_scene = nullptr;
    main_scene = nullptr;

    Engine::destroy();
    
    return 0;
}

#endif