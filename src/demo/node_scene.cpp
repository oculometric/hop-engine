#if !defined(STANDALONE)

#include <imgui/imgui.h>

#include "hop_engine.h"
#include "../main.h"

using namespace HopEngine;

static WeakRef<NodeView> node_view;
static WeakRef<NodeView::Style> style;

static Ref<Scene> initNodeScene()
{
    Ref<Scene> scene = Scene::create();
    // node_view = scene->insertObject<NodeView>(NodeView::create());
    // style = node_view->getStyle();
    // node_view->nodes.push_back(new NodeView::Node
    //     { "Hello, World!",
    //     {
    //         NodeView::NodeElement("Outputs on right", NodeView::ELEMENT_OUTPUT, 0),
    //         NodeView::NodeElement("text 6px inwards", NodeView::ELEMENT_OUTPUT, 1, false),
    //         NodeView::NodeElement("text 4px down", NodeView::ELEMENT_OUTPUT, 2),
    //         NodeView::NodeElement("Inputs on the left", NodeView::ELEMENT_INPUT),
    //         NodeView::NodeElement("", NodeView::ELEMENT_SPACE),
    //         NodeView::NodeElement("above is a banner", NodeView::ELEMENT_TEXT),
    //         NodeView::NodeElement("extra bottom spacing", NodeView::ELEMENT_TEXT),
    //     }, glm::vec2{ 0, 0 }, glm::vec2{ 8, 1 } });
    //     node_view->nodes.push_back(new NodeView::Node
    //         { "multiply",
    //         {
    //             { "result", NodeView::ELEMENT_OUTPUT },
    //             { "input a", NodeView::ELEMENT_INPUT },
    //             { "input b", NodeView::ELEMENT_INPUT },
    //         }, { 13, 4 } });
    //     node_view->nodes.push_back(new NodeView::Node
    //         { "add",
    //         {
    //             { "result", NodeView::ELEMENT_OUTPUT },
    //             { "input a", NodeView::ELEMENT_INPUT },
    //             { "input b", NodeView::ELEMENT_INPUT },
    //         }, { -14, 0 }, { 6, 0 }, { 1, 0, 0 }, { { node_view->nodes[0], 0 }}});
    //     node_view->nodes.push_back(new NodeView::Node
    //         { "multiply add",
    //         {
    //             { "result", NodeView::ELEMENT_OUTPUT },
    //             { "input a", NodeView::ELEMENT_INPUT },
    //             { "input b", NodeView::ELEMENT_INPUT },
    //             { "input c", NodeView::ELEMENT_INPUT },
    //         }, { -6, 10 } });
    //     node_view->nodes.push_back(new NodeView::Node
    //         { "make vec3",
    //         {
    //             { "vector", NodeView::ELEMENT_OUTPUT, 1 },
    //             { "length", NodeView::ELEMENT_OUTPUT },
    //             { "normalised", NodeView::ELEMENT_OUTPUT, 3, false },
    //             { "x", NodeView::ELEMENT_INPUT, 0, false },
    //             { "y", NodeView::ELEMENT_INPUT, 0, false },
    //             { "z", NodeView::ELEMENT_INPUT, 0, false },
    //         }, { -6, -10 } });
    //     node_view->nodes.push_back(new NodeView::Node
    //         { "kill john lennon",
    //         {
    //             { "", NodeView::ELEMENT_INPUT, 4, false },
    //             { "execution?", NodeView::ELEMENT_OUTPUT, 5 },
    //             { "hello", NodeView::ELEMENT_INPUT, 0, false },
    //         }, { -6, -15 } });

    // node_view->updateMesh();

    // auto style = node_view->getStyle();
    // node_view->setStyle(style);

    // scene->getCamera(0)->transform.lookAt({ 0, 0, 6 }, { 0, 0, 0 }, { 0, 1, 0 });
    
    Engine::setScene(scene);
    RenderServer::setTitle("Demo Scene - Nodes");

    return scene;
}

static void updateNodeScene(float delta_time)
{
    // node_view->checkInput({ 0, 0 }, Engine::getScene()->getViewportSize());
}

void imguiNodeScene()
{
    // ImGui::Begin("style controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    // {
    //     if (ImGui::CollapsingHeader("header", ImGuiTreeNodeFlags_DefaultOpen))
    //     {
    //         ImGui::InputInt("header_align", &style->header_align, 1, 1);
    //         ImGui::Checkbox("header_at_top", &style->header_at_top);
    //         ImGui::Checkbox("header_fill", &style->header_fill);
    //         ImGui::InputInt("after_header_spacing", &style->after_header_spacing, 1, 1);
    //         ImGui::Spacing();
    //     }

    //     if (ImGui::CollapsingHeader("text", ImGuiTreeNodeFlags_DefaultOpen))
    //     {
    //         ImGui::SliderFloat2("text_offset", (float*)&style->text_offset, -10.0f, 10.0f);
    //         ImGui::ColorEdit3("text_colour", (float*)&style->text_colour);
    //         ImGui::SliderFloat("text_spacing", &style->text_spacing, -2.0f, 4.0f);
    //         ImGui::Spacing();
    //     }

    //     if (ImGui::CollapsingHeader("outline", ImGuiTreeNodeFlags_DefaultOpen))
    //     {
    //         ImGui::Combo("outline_style", (int*)&style->outline_style, "HIDDEN\0PRESET_COLOUR\0NODE_COLOUR\0MODULATE_NODE_COLOUR\0");
    //         ImGui::ColorEdit3("outline_colour", (float*)&style->outline_colour);
    //         ImGui::ColorEdit3("outline_colour_highlight", (float*)&style->outline_colour_highlight);
    //         ImGui::SliderFloat("outline_colour_mult", &style->outline_colour_mult, 0.0f, 1.0f);
    //         ImGui::Spacing();
    //     }

    //     if (ImGui::CollapsingHeader("fill", ImGuiTreeNodeFlags_DefaultOpen))
    //     {
    //         ImGui::Checkbox("fill_modulate_colour", &style->fill_modulate_colour);
    //         ImGui::ColorEdit3("fill_colour", (float*)&style->fill_colour);
    //         ImGui::SliderFloat("fill_colour_mult", &style->fill_colour_mult, 0.0f, 1.0f);
    //         ImGui::Spacing();
    //     }

    //     if (ImGui::CollapsingHeader("background", ImGuiTreeNodeFlags_DefaultOpen))
    //     {
    //         ImGui::ColorEdit3("background_colour", (float*)&style->background_colour);
    //         ImGui::Checkbox("show_grid", &style->show_grid);
    //         ImGui::InputInt("grid_scale", &style->grid_scale, 1);
    //         ImGui::ColorEdit3("grid_colour", (float*)&style->grid_colour);
    //         ImGui::SliderFloat("grid_dots_modulate", &style->grid_dots_modulate, 0.0f, 10.0f);
    //         ImGui::Spacing();
    //     }

    //     if (ImGui::CollapsingHeader("elements", ImGuiTreeNodeFlags_DefaultOpen))
    //     {
    //         ImGui::InputFloat("pin_offset", &style->pin_offset, 1.0f, 1.0f);
    //         ImGui::Checkbox("reverse_element_order", &style->reverse_element_order);
    //         ImGui::Checkbox("center_text_elements", &style->center_text_elements);
    //         ImGui::InputInt("after_elements_spacing", &style->after_elements_spacing, 1, 1);
    //         ImGui::Spacing();
    //     }

    //     ImGui::Checkbox("shadows", &style->shadows);
    //     ImGui::SliderFloat2("shadow_offset", (float*)&style->shadow_offset, -12, 12);
    //     ImGui::ColorEdit3("shadow_colour", (float*)&style->shadow_colour);

    //     ImGui::Spacing();
    //     if (ImGui::Button("update style"))
    //         node_view->setStyle(style.strong());

    //     ImGui::End();
    // }
}

SceneFuncSet getNodeScene()
{
    return { L"nodes", initNodeScene, updateNodeScene, imguiNodeScene };
}

#endif