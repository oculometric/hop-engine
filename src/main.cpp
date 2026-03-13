#if !defined(STANDALONE)

#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

class ProtoEditorApp : public Application
{
private:
    WeakRef<NodeView> node_view;
    Ref<Scene> node_scene;
    Ref<Scene> main_scene;
    Ref<AshaApp> asha_app;

public:
    ProtoEditorApp()
    {
        asha_app = new AshaApp();
        main_scene = Engine::getScene();

        node_scene = Scene::create();
        auto camera_obj = node_scene->addObject("camera");
        camera_obj->addComponent<CameraComponent>();
        auto node_obj = node_scene->addObject("node view");
        node_view = node_obj->addComponent<NodeView>();
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

        main_scene->setSkybox(Engine::loadTexture("res://engine/samples/nasa_goddard_gaia_dr2_deep_star_map.png"));

        RenderServer::setTitle("Demo Scene - Main");
    }

    void update(float delta_time) override
    {
        node_view->checkInput({ 0, node_scene->getViewportSize().y * 3.0f }, node_scene->getViewportSize());
        if (Input::getMousePosition().y < node_scene->getViewportSize().y * 3.0f)
            asha_app->update(delta_time);
    }

    void drawImGui() override
    {
        asha_app->drawImGui();
    }
};

int main()
{
    Engine::init();

    Package::importPackage("resources.hop");

    Engine::runApplication<ProtoEditorApp>();

    Engine::destroy();
    
    return 0;
}

#endif