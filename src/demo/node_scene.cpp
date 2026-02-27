#if !defined(STANDALONE)

#include <imgui.h>

#include "hop_engine.h"
#include "../main.h"

using namespace HopEngine;

static WeakRef<NodeView> node_view;
static WeakRef<NodeView::Node> selected_node;

static Ref<Scene> initNodeScene()
{
    Ref<Scene> scene = Scene::create();
    node_view = scene->insertObject<NodeView>(NodeView::create());
    node_view->nodes.push_back(new NodeView::Node
        { "Hello, World!",
        {
            NodeView::NodeElement("Outputs on right", NodeView::ELEMENT_OUTPUT),
            NodeView::NodeElement( "text 6px inwards", NodeView::ELEMENT_OUTPUT),
            NodeView::NodeElement( "text 4px down", NodeView::ELEMENT_OUTPUT),
            NodeView::NodeElement( "Inputs on the left", NodeView::ELEMENT_INPUT),
            NodeView::NodeElement( "", NodeView::ELEMENT_SPACE),
            NodeView::NodeElement( "above is a banner", NodeView::ELEMENT_TEXT),
            NodeView::NodeElement( "extra bottom spacing", NodeView::ELEMENT_TEXT),
        }, glm::vec2{ 0, 0 } * 24.0f });
    // node_view->nodes.push_back(new NodeView::Node
    //     { "multiply",
    //     {
    //         { "result", NodeView::ELEMENT_OUTPUT },
    //         { "input a", NodeView::ELEMENT_INPUT },
    //         { "input b", NodeView::ELEMENT_INPUT },
    //     }, { 13, 4 } });
    // node_view->nodes.push_back(new NodeView::Node
    //     { "add",
    //     {
    //         { "result", NodeView::ELEMENT_OUTPUT },
    //         { "input a", NodeView::ELEMENT_INPUT },
    //         { "input b", NodeView::ELEMENT_INPUT },
    //     }, { -6, 0 } });
    // node_view->nodes.push_back(new NodeView::Node
    //     { "multiply add",
    //     {
    //         { "result", NodeView::ELEMENT_OUTPUT },
    //         { "input a", NodeView::ELEMENT_INPUT },
    //         { "input b", NodeView::ELEMENT_INPUT },
    //         { "input c", NodeView::ELEMENT_INPUT },
    //     }, { -6, 10 } });
    // node_view->nodes.push_back(new NodeView::Node
    //     { "make vec3",
    //     {
    //         { "vector", NodeView::ELEMENT_OUTPUT, 1 },
    //         { "length", NodeView::ELEMENT_OUTPUT },
    //         { "normalised", NodeView::ELEMENT_OUTPUT, 3, false },
    //         { "x", NodeView::ELEMENT_INPUT, 0, false },
    //         { "y", NodeView::ELEMENT_INPUT, 0, false },
    //         { "z", NodeView::ELEMENT_INPUT, 0, false },
    //     }, { -6, -10 } });
    // node_view->nodes.push_back(new NodeView::Node
    //     { "kill john lennon",
    //     {
    //         { "", NodeView::ELEMENT_INPUT, 4, false },
    //         { "execution?", NodeView::ELEMENT_OUTPUT, 5 },
    //         { "hello", NodeView::ELEMENT_INPUT, 0, false },
    //     }, { -6, -15 } });

    node_view->updateMesh();

    auto style = node_view->getStyle();

    scene->getCamera(0)->transform.lookAt({ 0, 0, 6 }, { 0, 0, 0 }, { 0, 1, 0 });
    scene->getCamera(0)->clear_colour = {0, 0, 0};
    
    Engine::setScene(scene);
    return scene;
}

static void updateNodeScene(Ref<Scene> scene, float delta_time)
{
    static bool node_view_dirty = true;
    //
    // if (Input::wasMousePressed(Input::MOUSE_LEFT))
    // {
    //     if (selected_node)
    //         selected_node->highlighted = false;
    //     glm::vec2 camera_pos = scene->getCamera(0)->transform.getLocalPosition();
    //     glm::vec2 mouse_screen_pos = Input::getMousePosition() - (RenderServer::getFramebufferSize() * 0.5f);
    //     glm::vec2 mouse_world_pos = mouse_screen_pos + (camera_pos * RenderServer::getFramebufferSize() * 0.5f);
    //     selected_node = node_view->select(mouse_world_pos);
    //     if (selected_node)
    //         selected_node->highlighted = true;
    //     node_view_dirty = true;
    // }
    //
    // glm::vec2 mouse_delta = Input::getMouseDelta() * 0.025f;
    // float move_x = Input::getAxis(Input::KEY_LEFT, Input::KEY_RIGHT);
    // float move_y = Input::getAxis(Input::KEY_UP, Input::KEY_DOWN);
    // if (Input::isMouseDown(Input::MOUSE_RIGHT))
    // {
    //     glm::vec2 mouse_world_delta = glm::vec2{ -mouse_delta.x, -mouse_delta.y };// / RenderServer::getFramebufferSize();
    //     scene->getCamera(0)->transform.translateLocal({mouse_world_delta.x, mouse_world_delta.y, 0});
    // }
    // else if (Input::isMouseDown(Input::MOUSE_LEFT))
    // {
    //     move_x = mouse_delta.x * 20.0f;
    //     move_y = mouse_delta.y * 20.0f;
    //     node_view_dirty = true;
    // }
    //
    // if (move_x != 0 || move_y != 0)
    // {
    //     if (selected_node)
    //     {
    //         selected_node->position += glm::vec2{ move_x, move_y } * 0.5f;
    //         node_view_dirty = true;
    //     }
    // }
    //
    if (node_view_dirty)
        node_view->updateMesh();
    node_view_dirty = false;
    Input::resetMouseDelta();
}

void imguiNodeScene(Ref<Scene> scene, float delta_time)
{
    ImGui::Begin("style controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    NodeView::Style style = node_view->getStyle();
    
    if (ImGui::CollapsingHeader("header", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::InputInt("header_align", &style.header_align, 1, 1);
        ImGui::Checkbox("header_at_top", &style.header_at_top);
        ImGui::Checkbox("header_fill", &style.header_fill);
        ImGui::Checkbox("header_outline", &style.header_outline);
        ImGui::Spacing();
    }
    
    if (ImGui::CollapsingHeader("text", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat2("text_offset", (float*)&style.text_offset, -10.0f, 10.0f);
        ImGui::ColorEdit3("text_colour", (float*)&style.text_colour);
        ImGui::SliderFloat("text_spacing", &style.text_spacing, 0.0f, 4.0f);
        ImGui::Spacing();
    }
    
    if (ImGui::CollapsingHeader("outline", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::SliderFloat("outline_colour_mult", &style.outline_colour_mult, 0.0f, 1.0f);
        ImGui::Spacing();
    }
    
    ImGui::ColorEdit3("grid_colour", (float*)&style.grid_colour);
    
    node_view->setStyle(style);
    ImGui::End();
}

SceneFuncSet getNodeScene()
{
    return { L"nodes", initNodeScene, updateNodeScene, imguiNodeScene };
}

#endif