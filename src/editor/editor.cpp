#include "editor.h"

using namespace HopEngine;

void Editor::awake()
{
    Engine::setShowGizmos(true);
    RenderServer::setTitle("HopEngine");

    view_3d = Scene::create("3D View");
    {
        auto comp      = view_3d->addObject<StaticMeshComponent>("bunny");
        comp->mesh     = Engine::loadMesh("res://engine/meshes/bunny.obj");
        comp->material = Engine::loadMaterial("res://engine/materials/bunny.hmat");

        auto obj          = view_3d->addObject("camera");
        auto cam          = obj->addComponent<CameraComponent>();
        cam->clear_colour = { 0.05f, 0.05f, 0.05f };
        view_3d->sky      = new Sky(Engine::loadTexture("res://engine/textures/basic_skybox.png"));
        obj->getTransform().lookAt(glm::vec3(0.2f, -0.2f, 0.2f), glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f));

        obj                                               = view_3d->addObject("camera2");
        obj->addComponent<CameraComponent>()->camera_slot = 1;
        obj->getTransform().setPosition({ 1, 0, 0 });

        obj = view_3d->addObject("lamp");
        obj->addComponent<LightComponent>();
        obj->getTransform().setPosition({ 0, 0, 2 });

        obj            = view_3d->addObject("cube");
        cube           = obj->addComponent<StaticMeshComponent>();
        cube->mesh     = Engine::loadMesh("res://engine/meshes/cube.obj");
        cube->material = new Material(Engine::loadShader("test_shader.glsl"));
        cube->material->setTexture("albedo", Engine::loadTexture("res://engine/icon.png"));
    }

    // view_nodes = Scene::create("Node Editor");
    // {
    //     node_view = view_nodes->addObject<NodeView>("nodes");
    //     node_view->makeNode("Camera", { 1, 0, 0 }, 4)
    //         ->text("main_cam")
    //         ->text("slot: 0")
    //         ->output("colour")
    //         ->output("data_0")
    //         ->output("data_1")
    //         ->output("data_2")
    //         ->output("depth")
    //         ->position = { -16, -5 };
    //     node_view->makeNode("Camera", { 1, 0, 0 }, 4)
    //         ->text("right_cam")
    //         ->text("slot: 1")
    //         ->output("colour")
    //         ->output("data_0")
    //         ->output("data_1")
    //         ->output("data_2")
    //         ->output("depth")
    //         ->position = { -11, -4 };
    //     node_view->makeNode("Camera", { 1, 0, 0 }, 5)
    //         ->text("ring_doorbell")
    //         ->text("slot: 2")
    //         ->output("colour")
    //         ->output("data_0")
    //         ->output("data_1")
    //         ->output("data_2")
    //         ->output("depth")
    //         ->position = { -6, -3 };
    //     node_view->makeNode("Shader", { 0, 1, 0.8f }, 4)
    //         ->text("ssao")
    //         ->input("shader")
    //         ->input("tex_norm")
    //         ->input("tex_depth")
    //         ->output("colour")
    //         ->position = { 0, -1 };
    //     node_view->makeNode("Shader", { 0, 1, 0.8f }, 6)
    //         ->text("multi_composite")
    //         ->input("shader")
    //         ->input("tex_a")
    //         ->input("tex_b")
    //         ->input("tex_c")
    //         ->output("colour")
    //         ->position = { 5, -3 };
    //     node_view->makeNode("Shader", { 0, 1, 0.8f }, 5)
    //         ->text("final_pass")
    //         ->input("shader")
    //         ->input("tex")
    //         ->output("colour")
    //         ->position                                                                  = { 12, 0 };
    //     node_view->makeNode("Screen", { 0.7f, 0.2f, 0 }, 4)->input("texture")->position = { 18, 3 };

    //     node_view->updateMesh();

    //     view_nodes->addObject<CameraComponent>("camera");
    // }

    Engine::setScene(view_3d);

    RenderServer::setMultiScene({
        { view_3d, { 0.0f, 0.0f }, { 0.8f, 0.7f } },
        //{ view_nodes, { 0.0f, 0.7f }, { 0.8f, 0.3f } },
    });

    auto canvas = UIManager::push();
    auto panel  = canvas->addElement<UIPanel>();
    panel->setColour({ 1, 0, 0 });
    panel->setSize({ 300, 300 });
    auto label = canvas->addChild<UILabel>(panel);
    label->setText("hello, World!");
    label->setFormatting(
        { .align = UIRenderer::TEXT_ALIGN_CENTER, .flags = UIRenderer::TEXT_FLAGS_UNDERLINE });
    label->setExternalAnchor(UITransform::ANCHOR_MIDDLE_CENTER);

    auto canvas2 = view_3d->addObject<UICanvasComponent>("canvas_test");
    panel        = canvas2->getCanvas()->addElement<UIPanel>();
    panel->setColour({ 1.0f, 0.6f, 0 });
    panel->setScaling(UITransform::SCALING_FILL_BOTH);
    panel->setStyle(0);
    canvas2->getCanvas()->addElement<UILabel>()->setText("this is in world space");
    canvas2->getTransform().setLocalScale(
        glm::vec3(glm::vec2(1.0f) / canvas2->getCanvas()->getSize(), 1.0f));

    main_canvas      = UIManager::push();
    auto right_panel = main_canvas->addElement<UIPanel>();
    right_panel->setExternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
    right_panel->setInternalAnchor(UITransform::ANCHOR_TOP_RIGHT);
    right_panel->setSize({ 0.2f * RenderServer::getFramebufferSize().x, 1 });
    right_panel->setScaling(UITransform::SCALING_FILL_VERTICAL);
    auto title_label = main_canvas->addChild<UILabel>(right_panel);
    title_label->setText("PROPERTIES");
    title_label->setFormatting({ .align = UIRenderer::TEXT_ALIGN_CENTER });
    title_label->setExternalAnchor(UITransform::ANCHOR_TOP_CENTER);
    title_label->setPosition({ 0, 10 });
    auto lorem_label = main_canvas->addChild<UILabel>(right_panel);
    lorem_label->setText(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla at erat eleifend, placerat quam et, feugiat libero. Nunc sollicitudin lacus est, id venenatis justo bibendum vitae. Class aptent taciti sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos. Aliquam quis convallis turpis. Sed in convallis lectus, non mollis ligula. Aenean bibendum metus in metus bibendum porttitor. Fusce malesuada pulvinar orci at fermentum.");
    lorem_label->setScaling(UITransform::SCALING_FILL_HORIZONTAL);
    lorem_label->setColour({ 1, 0, 0 });
    lorem_label->setFormatting({ .flags = UIRenderer::TEXT_FLAGS_ITALIC, .wrap = true, .clip = true });
    lorem_label->setPosition({ 0, 60 });
    lorem_label->setSize({ 0, 800 });
    auto button = main_canvas->addElement<UIButton>();
    button->setExternalAnchor(UITransform::ANCHOR_BOTTOM_CENTER);
    button->setInternalAnchor(UITransform::ANCHOR_BOTTOM_CENTER);
    button->setSize({ 160, 32 });
}

void Editor::update(float delta_time)
{
    // if (!node_view->checkInput({ 0, RenderServer::getFramebufferSize().y * 0.7f },
    // view_nodes->getViewportSize()))
    Engine::debugCamera(view_3d->findObject("camera"));

    // cube->material->getShader()->reload();
}

void Editor::drawImGui()
{
    // node_view->drawImGuiDebug();
    // Engine::drawImGuiDebug();
}
