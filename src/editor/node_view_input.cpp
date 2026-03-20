#include "node_view.h"

#include "input.h"

using namespace HopEngine;
using namespace std;

enum InputState
{
    IDLE,
    MOUSE_PRESSING,
    MOUSE_DRAGGING,
    MOUSE_RELEASING
};

enum UIElement
{
    NONE,
    NODE,
    MINIMISE_BUT,
    INPUT_PIN,
    OUTPUT_PIN
};

// TODO: refactor this into an input event system!! (in the input class)
void NodeView::checkInput(glm::ivec2 rect_min, glm::ivec2 rect_size)
{
    // TODO: this is not allowed to be static!!
    static InputState state = IDLE;
    static Input::MouseButton button_upon_press;
    static glm::vec2 mouse_pos_upon_press;
    static UIElement element_type_upon_press = NONE;
    static auto node_upon_press = nodes.rend();

    bool needs_update = false;

    glm::vec2 mouse_pos = Input::getMousePosition();
    if (mouse_pos.x < rect_min.x || mouse_pos.y < rect_min.y ||
        mouse_pos.x > rect_min.x + rect_size.x || mouse_pos.y > rect_min.y + rect_size.y)
        return;
    glm::vec2 node_space_pos = (mouse_pos - glm::vec2(rect_min)) - (glm::vec2(rect_size) / 2.0f);

    // TODO: right mouse support
    if (state == IDLE && Input::wasMousePressed(Input::MOUSE_LEFT))
    {
        mouse_pos_upon_press = node_space_pos;
        button_upon_press = Input::MOUSE_LEFT;
        state = MOUSE_PRESSING;

        // a click event is starting, find element under the mouse when it was pressed
        node_upon_press = nodes.rend();
        element_type_upon_press = NONE;
        for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
        {
            auto node          = *it;
            glm::vec2 node_min = node->position * style->grid_size;
            glm::vec2 node_max = node->minimised ?
                node_min + (glm::vec2{ node->size.x, 1 } * style->grid_size) :
                node_min + (node->size * style->grid_size);
            if (node_space_pos.x < node_min.x || node_space_pos.y < node_min.y ||
                node_space_pos.x > node_max.x || node_space_pos.y > node_max.y)
                continue;
            node_upon_press = it;
            element_type_upon_press = NODE;

            glm::vec2 minibox_min = (node->position + glm::vec2{ node->size.x - 1, 0 }) * style->grid_size;
            glm::vec2 minibox_max = minibox_min + style->grid_size;
            if (!(node_space_pos.x < minibox_min.x || node_space_pos.y < minibox_min.y ||
                  node_space_pos.x > minibox_max.x || node_space_pos.y > minibox_max.y))
            {
                element_type_upon_press = MINIMISE_BUT;
            }
            else if (!node->minimised)
            {
                int index = style->header_at_top ? style->after_header_spacing : style->after_elements_spacing - 1;
                for (const auto& element : node->elements)
                {
                    ++index;
                    glm::vec2 selbox_min;
                    glm::vec2 selbox_max;
                    if (element.type == ELEMENT_OUTPUT)
                    {
                        selbox_min = (node->position + glm::vec2{ node->size.x - 1, index }) * style->grid_size;
                        selbox_max = selbox_min + style->grid_size;
                    }
                    else if (element.type == ELEMENT_INPUT)
                    {
                        selbox_min = (node->position + glm::vec2{ 0, index }) * style->grid_size;
                        selbox_max = selbox_min + style->grid_size;
                    }
                    else
                        continue;

                    if (node_space_pos.x < selbox_min.x || node_space_pos.x > selbox_max.x ||
                        node_space_pos.y < selbox_min.y || node_space_pos.y > selbox_max.y)
                        continue;
                    
                    if (element.type == ELEMENT_OUTPUT)
                    {
                        element_type_upon_press = OUTPUT_PIN;
                        temp_link_start = (selbox_min / style->grid_size) + glm::vec2{ 1.0f, 0.5f };
                    }
                    else
                    {
                        element_type_upon_press = INPUT_PIN;
                        temp_link_start = (selbox_min / style->grid_size) + glm::vec2{ 0.0f, 0.5f };
                    }
                    draw_temp_link = true;
                    break;
                }
            }
            break;
        }
    }
    else if (state == MOUSE_PRESSING && Input::isMouseDown(Input::MOUSE_LEFT))
    {
        if (glm::length(node_space_pos - mouse_pos_upon_press) > 2.0f)
        {
            state = MOUSE_DRAGGING;
        }
    }
    else if (state == MOUSE_PRESSING && !Input::isMouseDown(Input::MOUSE_LEFT))
    {
        state = MOUSE_RELEASING;

        // a click event finished, select whatever is under the mouse
        if (element_type_upon_press == NONE)
        {
            // if mouse was clicked in space, deselect all
            for (auto& n : nodes) n->highlighted = false;
        }
        else if (element_type_upon_press == MINIMISE_BUT)
        {
            auto node = *node_upon_press;
            node->minimised = !node->minimised;
        }
        else
        {
            // if mouse was clicked on a node, select it (possibly deselecting all others)
            auto node = *node_upon_press;
            if (!Input::isKeyDown(Input::KEY_LEFT_SHIFT))
            {
                for (auto& n : nodes) n->highlighted = false;
                node->highlighted = true;
            }
            else
                node->highlighted = !node->highlighted;
            // move just-clicked node to front
            nodes.erase((node_upon_press + 1).base());
            nodes.insert(nodes.end(), node);
            node_upon_press = nodes.rbegin();
        }
        needs_update = true;
    }
    else if (state == MOUSE_DRAGGING && Input::isMouseDown(Input::MOUSE_LEFT))
    {
        // a drag event is occurring, move whatever was under the mouse upon press, or drag select
        if (element_type_upon_press == NONE)
        {
            // TODO: if mouse was dragged from space, perform box select
        }
        else if (element_type_upon_press == OUTPUT_PIN || element_type_upon_press == INPUT_PIN)
        {
            temp_link_end = node_space_pos / style->grid_size;
            needs_update = true;
        }
        else
        {
            // if mouse was dragged from a node, move some nodes!
            auto node = *node_upon_press;
            // if the node under the mouse is not highlighted, and shift is not held, then highlight it and deselect others
            if (!node->highlighted && !Input::isKeyDown(Input::KEY_LEFT_SHIFT))
            {
                for (auto& n : nodes) n->highlighted = false;
                node->highlighted = true;
                nodes.erase((node_upon_press + 1).base());
                nodes.insert(nodes.end(), node);
                node_upon_press = nodes.rbegin();
            }

            for (auto& n : nodes)
                if (n->highlighted) n->position += Input::getMouseDelta() / style->grid_size;
            needs_update = true;
        }
    }
    else if (state == MOUSE_DRAGGING && !Input::isMouseDown(Input::MOUSE_LEFT))
    {
        state = MOUSE_RELEASING;

        // a drag event is ending
        for (auto& n : nodes)
            if (n->highlighted) n->position = glm::round(n->position);
        needs_update = true;
    }
    else if (state == MOUSE_RELEASING)
    {
        state = IDLE;
        if (draw_temp_link)
        {
            draw_temp_link = false;
            needs_update = true;
        }
    }

    if (needs_update) updateMesh();
}
