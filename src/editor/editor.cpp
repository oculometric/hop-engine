#include "editor.h"

using namespace HopEngine;

void Editor::awake()
{
    Engine::setShowGizmos(true);

    view_3d = Scene::create("3D View");
    {
        auto comp = view_3d->addObject<StaticMeshComponent>("bunny");
        comp->mesh = Engine::loadMesh("res://engine/meshes/bunny.obj");
        comp->material = Engine::loadMaterial("res://engine/materials/bunny.hmat");
        
        auto obj = view_3d->addObject("camera");
        auto cam = obj->addComponent<CameraComponent>();
        cam->clear_colour = { 0.05f, 0.05f, 0.05f };
        view_3d->setSkybox(Engine::loadTexture("res://engine/textures/basic_skybox.png"));
        obj->getTransform().lookAt(glm::vec3(0.2f, -0.2f, 0.2f),
                              glm::vec3(0.0f, 0.0f, 0.0f),
                              glm::vec3(0.0f, 0.0f, 1.0f));

        obj = view_3d->addObject("camera2");
        obj->addComponent<CameraComponent>()->camera_slot = 1;
        obj->getTransform().setPosition({ 1, 0, 0 });

        obj = view_3d->addObject("lamp");
        obj->addComponent<LightComponent>();
        obj->getTransform().setPosition({ 0, 0, 2 });

        obj = view_3d->addObject("cube");
        cube = obj->addComponent<StaticMeshComponent>();
        cube->mesh = Engine::loadMesh("res://engine/meshes/cube.obj");
        cube->material = new Material(Engine::loadShader("test_shader.glsl"));
        cube->material->setTexture("albedo", Engine::loadTexture("res://engine/icon.png"));
    }

    view_nodes = Scene::create("Node Editor");
    {
        node_view = view_nodes->addObject<NodeView>("nodes");
        node_view->makeNode("Camera", { 1, 0, 0 }, 4)
            ->text("main_cam")
            ->text("slot: 0")
            ->output("colour")
            ->output("data_0")
            ->output("data_1")
            ->output("data_2")
            ->output("depth")
            ->position = { -16, -5 };
        node_view->makeNode("Camera", { 1, 0, 0 }, 4)
            ->text("right_cam")
            ->text("slot: 1")
            ->output("colour")
            ->output("data_0")
            ->output("data_1")
            ->output("data_2")
            ->output("depth")
            ->position = { -11, -4 };
        node_view->makeNode("Camera", { 1, 0, 0 }, 5)
            ->text("ring_doorbell")
            ->text("slot: 2")
            ->output("colour")
            ->output("data_0")
            ->output("data_1")
            ->output("data_2")
            ->output("depth")
            ->position = { -6, -3 };
        node_view->makeNode("Shader", { 0, 1, 0.8f }, 4)
            ->text("ssao")
            ->input("shader")
            ->input("tex_norm")
            ->input("tex_depth")
            ->output("colour")
            ->position = { 0, -1 };
        node_view->makeNode("Shader", { 0, 1, 0.8f }, 6)
            ->text("multi_composite")
            ->input("shader")
            ->input("tex_a")
            ->input("tex_b")
            ->input("tex_c")
            ->output("colour")
            ->position = { 5, -3 };
        node_view->makeNode("Shader", { 0, 1, 0.8f }, 5)
            ->text("final_pass")
            ->input("shader")
            ->input("tex")
            ->output("colour")
            ->position = { 12, 0 };
        node_view->makeNode("Screen", { 0.7f, 0.2f, 0 }, 4)
            ->input("texture")
            ->position = { 18, 3 };
            
        node_view->updateMesh();

        view_nodes->addObject<CameraComponent>("camera");
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
    if (!node_view->checkInput({ 0, RenderServer::getFramebufferSize().y * 0.7f }, view_nodes->getViewportSize()))
        Engine::debugCamera(view_3d->findObject("camera"));

    //cube->material->getShader()->reload();
}

void Editor::drawImGui()
{
    node_view->drawImGuiDebug();
}
