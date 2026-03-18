#include "editor.h"

using namespace HopEngine;

Editor::Editor()
{
    view_3d = Scene::create("3D View");
    {
        auto obj = view_3d->addObject("bunny");
        auto comp = obj->addComponent<StaticMeshComponent>();
        comp->mesh = Engine::loadMesh("res://engine/samples/bunny.obj");
        comp->material = new Material(
            Engine::loadShader("res://engine/samples/psx.glsl"),
            Pipeline::Builder().cullMode(Pipeline::CULL_NONE));
        comp->material->setTexture("albedo", Engine::loadTexture("res://engine/samples/bunny.png"));
        comp->material->setSampler("albedo", Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_NEAREST)));
        
        obj = view_3d->addObject("camera");
        auto cam = obj->addComponent<CameraComponent>();
        cam->clear_colour = { 0.05f, 0.05f, 0.05f };
        view_3d->setSkybox(Engine::loadTexture("res://engine/textures/basic_skybox.png"));
        obj->transform.lookAt(glm::vec3(0.2f, -0.2f, 0.2f),
                              glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 0.0f, 1.0f));
    }

    view_nodes = Scene::create("Node Editor");
    {
        auto obj = view_nodes->addObject("nodes");
        node_view = obj->addComponent<NodeView>();
        node_view->nodes.push_back(new NodeView::Node{
            "Camera",
            {
              { "main_cam", NodeView::ELEMENT_TEXT },
              { "slot: 0", NodeView::ELEMENT_TEXT },
              { "colour", NodeView::ELEMENT_OUTPUT },
              { "data_0", NodeView::ELEMENT_OUTPUT },
              { "data_1", NodeView::ELEMENT_OUTPUT },
              { "data_2", NodeView::ELEMENT_OUTPUT },
              { "depth", NodeView::ELEMENT_OUTPUT },
              },
            glm::vec2{ -16, -5 },
            glm::vec2{ 4, 1 },
            glm::vec3{ 1, 0, 0 }
        });
        node_view->nodes.push_back(new NodeView::Node{
            "Camera",
            {
              { "right_cam", NodeView::ELEMENT_TEXT },
              { "slot: 1", NodeView::ELEMENT_TEXT },
              { "colour", NodeView::ELEMENT_OUTPUT },
              { "data_0", NodeView::ELEMENT_OUTPUT },
              { "data_1", NodeView::ELEMENT_OUTPUT },
              { "data_2", NodeView::ELEMENT_OUTPUT },
              { "depth", NodeView::ELEMENT_OUTPUT },
              },
            glm::vec2{ -11, -4 },
            glm::vec2{ 4, 1 },
            glm::vec3{ 1, 0, 0 }
        });
        node_view->nodes.push_back(new NodeView::Node{
            "Camera",
            {
              { "ring_doorbell", NodeView::ELEMENT_TEXT },
              { "slot: 2", NodeView::ELEMENT_TEXT },
              { "colour", NodeView::ELEMENT_OUTPUT },
              { "data_0", NodeView::ELEMENT_OUTPUT },
              { "data_1", NodeView::ELEMENT_OUTPUT },
              { "data_2", NodeView::ELEMENT_OUTPUT },
              { "depth", NodeView::ELEMENT_OUTPUT },
              },
            glm::vec2{ -6, -3 },
            glm::vec2{ 5, 1 },
            glm::vec3{ 1, 0, 0 }
        });
        node_view->nodes.push_back(new NodeView::Node{
            "Shader",
            {
              { "ssao", NodeView::ELEMENT_TEXT },
              { "shader", NodeView::ELEMENT_INPUT, 3 },
              { "tex_norm", NodeView::ELEMENT_INPUT },
              { "tex_depth", NodeView::ELEMENT_INPUT },
              { "colour", NodeView::ELEMENT_OUTPUT },
              },
            glm::vec2{ 0, -1 },
            glm::vec2{ 4, 1 },
            glm::vec3{ 0, 1, 0.8f }
        });
        node_view->nodes.push_back(new NodeView::Node{
            "Shader",
            {
              { "multi_composite", NodeView::ELEMENT_TEXT },
              { "shader", NodeView::ELEMENT_INPUT, 3 },
              { "tex_a", NodeView::ELEMENT_INPUT },
              { "tex_b", NodeView::ELEMENT_INPUT },
              { "tex_c", NodeView::ELEMENT_INPUT },
              { "colour", NodeView::ELEMENT_OUTPUT },
              },
            glm::vec2{ 5, -3 },
            glm::vec2{ 6, 1 },
            glm::vec3{ 0, 1, 0.8f }
        });
        node_view->nodes.push_back(new NodeView::Node{
            "Shader",
            {
              { "final_pass", NodeView::ELEMENT_TEXT },
              { "shader", NodeView::ELEMENT_INPUT, 3 },
              { "tex", NodeView::ELEMENT_INPUT },
              { "colour", NodeView::ELEMENT_OUTPUT },
              },
            glm::vec2{ 12, 0 },
            glm::vec2{ 5, 1 },
            glm::vec3{ 0, 1, 0.8f }
        });
        node_view->nodes.push_back(new NodeView::Node{
            "Screen",
            {
              { "texture", NodeView::ELEMENT_INPUT },
              },
            glm::vec2{ 18, 3 },
            glm::vec2{ 4, 1 },
            glm::vec3{ 0.7f, 0.2f, 0 }
        });
        node_view->setStyle(node_view->getStyle());

        obj = view_nodes->addObject("camera");
        obj->addComponent<CameraComponent>();
    }

    Engine::setScene(view_3d);

    RenderServer::setMultiScene(
        {
            { view_3d,    { 0.0f, 0.0f }, { 0.8f, 0.7f } },
            { view_nodes, { 0.0f, 0.7f }, { 0.8f, 0.3f } },
        }
    );
}

void Editor::update(float delta_time)
{
    node_view->checkInput({ 0, RenderServer::getFramebufferSize().y * 0.7f }, view_nodes->getViewportSize());
    Engine::debugCamera(view_3d->findObject("camera"));
}
