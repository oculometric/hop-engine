#if !defined(STANDALONE)

#include "hop_engine.h"
#include "main.h"

using namespace HopEngine;

static WeakRef<NodeView> node_view;
static WeakRef<NodeView::Node> selected_node;

static void initNodeScene()
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
    
    Engine::setScene(scene);
}

static void updateNodeScene(Ref<Scene> scene, float delta_time)
{
    bool node_view_dirty = false;

    if (Input::wasMousePressed(Input::MOUSE_LEFT))
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
    float move_x = Input::getAxis(Input::KEY_LEFT, Input::KEY_RIGHT);
    float move_y = Input::getAxis(Input::KEY_UP, Input::KEY_DOWN);
    if (Input::isMouseDown(Input::MOUSE_RIGHT))
    {
        glm::vec2 mouse_world_delta = glm::vec2{ -mouse_delta.x, -mouse_delta.y };// / RenderServer::getFramebufferSize();
        scene->getCamera(0)->transform.translateLocal({mouse_world_delta.x, mouse_world_delta.y, 0});
    }
    else if (Input::isMouseDown(Input::MOUSE_LEFT))
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

SceneFuncSet getNodeScene()
{
    return { L"nodes", initNodeScene, updateNodeScene, nullptr };
}

#endif