#include "node_view.h"

#include "input.h"
#include "user_interface.h"

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
    OUTPUT_PIN,
    RIGHT_RESIZE,
    LEFT_RESIZE,
};

static bool checkNodeBox(WeakRef<NodeView::Node> node, glm::vec2 pos, bool extended)
{
    glm::vec2 node_min = node->position;
    glm::vec2 node_max = node->minimised ?
        node_min + glm::vec2{ node->size.x, 1 } :
        node_min + node->size + glm::vec2{ 0, 1 };
    if (extended)
    {
        node_min.x -= 0.5f;
        node_max.x += 0.5f;
    }
    if (pos.x < node_min.x || pos.y < node_min.y ||
        pos.x > node_max.x || pos.y > node_max.y)
        return false;
    return true;
}

static bool checkPinBox(WeakRef<NodeView::Node> node, int element, glm::vec2 pos, WeakRef<NodeView::Style> style, glm::vec2& selbox_min)
{
    if (element < 0 || static_cast<size_t>(element) >= node->elements.size())
        return false;
    glm::vec2 selbox_max;
    int index = (style->header_at_top ? style->after_header_spacing + 1 : style->after_elements_spacing) + element;
    if (node->elements[element].type == NodeView::ELEMENT_OUTPUT)
    {
        selbox_min = (node->position + glm::vec2{ node->size.x - 0.5f, index });
        selbox_max = selbox_min + glm::vec2{ 1, 1 };
    }
    else if (node->elements[element].type == NodeView::ELEMENT_INPUT)
    {
        selbox_min = (node->position + glm::vec2{ -0.5f, index });
        selbox_max = selbox_min + glm::vec2{ 1, 1 };
    }
    else
        return false;
    if (pos.x < selbox_min.x || pos.x > selbox_max.x ||
        pos.y < selbox_min.y || pos.y > selbox_max.y)
        return false;
    return true;
}

// TODO: refactor this into an input event system!! (in the input class)
bool NodeView::checkInput(glm::ivec2 rect_min, glm::ivec2 rect_size)
{
    // TODO: this is not allowed to be static!!
    static InputState state = IDLE;
    static glm::vec2 mouse_pos_upon_press;
    static UIElement element_type_upon_press = NONE;
    static int input_output_element_index = 0;
    static auto node_upon_press = nodes.rend();

    bool needs_update = false;

    glm::vec2 mouse_pos = Input::getMousePosition();
    if (mouse_pos.x < rect_min.x || mouse_pos.y < rect_min.y ||
        mouse_pos.x > rect_min.x + rect_size.x || mouse_pos.y > rect_min.y + rect_size.y)
        return false;
    glm::vec2 offset = getTransform().getLocalPosition();
    glm::vec2 node_space_pos = ((mouse_pos - glm::vec2(rect_min)) - (glm::vec2(rect_size) / 2.0f)) - offset;

    // if (context_menu)
    // {
    //     if ((!context_menu->checkInput() && Input::isMouseDown(Input::MOUSE_LEFT)) || Input::wasKeyPressed(Input::KEY_ESCAPE))
    //         context_menu = nullptr;
    //     return true;
    // }

    if (Input::isKeyDown('A') && Input::isKeyDown(Input::KEY_LEFT_CONTROL))
    {
        for (auto& node : nodes)
            node->highlighted = true;
        needs_update = true;
    }
    else if (Input::isKeyDown(Input::KEY_ESCAPE))
    {
        for (auto& node : nodes)
            node->highlighted = false;
        needs_update = true;
    }

    // if (Input::wasMousePressed(Input::MOUSE_RIGHT))
    // {
    //     context_menu = new UIContextMenu(node_space_pos);
    //     context_menu->addButton("a button", nullptr);
    //     context_menu->addButton("another button", nullptr);
    //     context_menu->addText("text stuff");
    //     context_menu->addButton("final button", nullptr);
    //     context_menu->done();
    //     return true;
    // }

    if (Input::isMouseDown(Input::MOUSE_MIDDLE))
    {
        getTransform().translateLocal(glm::vec3(Input::getMouseDelta(), 0));
        Input::lockMouseToRectangle(rect_min, rect_min + rect_size);
        return true;
    }
    else
        Input::unlockMouse();

    // TODO: right mouse support
    if (state == IDLE)
    {
        // a click event is starting, find element under the mouse when it was pressed
        node_upon_press = nodes.rend();
        element_type_upon_press = NONE;
        for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
        {
            auto node          = *it;
            if (!checkNodeBox(node, node_space_pos / style->grid_size, true))
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
                // if node is not minimised, check if the user is dragging from a pin
                int actual_index = -1;
                int output_index = -1;
                int input_index = -1;
                for (const auto& element : node->elements)
                {
                    if (element.type == ELEMENT_OUTPUT)
                        ++output_index;
                    else if (element.type == ELEMENT_INPUT)
                        ++input_index;
                    ++actual_index;
                    glm::vec2 selbox_min;
                    if (!checkPinBox(node, actual_index, node_space_pos / style->grid_size, style, selbox_min))
                        continue;
                    
                    temp_link_start = selbox_min + glm::vec2{ 0.5f, 0.5f };
                    if (element.type == ELEMENT_OUTPUT)
                    {
                        element_type_upon_press = OUTPUT_PIN;
                        input_output_element_index = output_index;
                    }
                    else
                    {
                        element_type_upon_press = INPUT_PIN;
                        input_output_element_index = input_index;
                    }
                    break;
                }
            }
            if (element_type_upon_press == NODE)
            {
                // if the user wasnt dragging a pin, then check if they're on an edge anyway
                glm::vec2 edgebox_min = (node->position + glm::vec2{ node->size.x - 0.5f, 0 }) * style->grid_size;
                glm::vec2 edgebox_max = edgebox_min + (glm::vec2{ 1.0f, node->size.y + 1.0f } * style->grid_size);
                if (node_space_pos.x >= edgebox_min.x && node_space_pos.x <= edgebox_max.x
                 && node_space_pos.y >= edgebox_min.y && node_space_pos.y <= edgebox_max.y)
                {
                    element_type_upon_press = RIGHT_RESIZE;
                }

                edgebox_min = (node->position + glm::vec2{ -0.5f, 0 }) * style->grid_size;
                edgebox_max = edgebox_min + (glm::vec2{ 1.0f, node->size.y + 1.0f } * style->grid_size);
                if (node_space_pos.x >= edgebox_min.x && node_space_pos.x <= edgebox_max.x
                 && node_space_pos.y >= edgebox_min.y && node_space_pos.y <= edgebox_max.y)
                {
                    element_type_upon_press = LEFT_RESIZE;
                }
            }
            if (element_type_upon_press == NODE && !checkNodeBox(node, node_space_pos / style->grid_size, false))
                continue;
            break;
        }

        switch (element_type_upon_press)
        {
        case NONE:
        case MINIMISE_BUT:
        case NODE:
            Input::setCursorImage(Input::CURSOR_NORMAL); break;
        case INPUT_PIN:
        case OUTPUT_PIN:
            Input::setCursorImage(Input::CURSOR_HAND); break;
        case RIGHT_RESIZE:
        case LEFT_RESIZE:
            Input::setCursorImage(Input::CURSOR_RESIZE_HORIZONTAL); break;
        }

        if (Input::wasMousePressed(Input::MOUSE_LEFT))
        {
            mouse_pos_upon_press = node_space_pos;
            state = MOUSE_PRESSING;
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
            draw_temp_link = true;
        }
        else if (element_type_upon_press == RIGHT_RESIZE)
        {
            auto node = *node_upon_press;
            node->size.x += Input::getMouseDelta().x / style->grid_size;
            needs_update = true;
        }
        else if (element_type_upon_press == LEFT_RESIZE)
        {
            auto node = *node_upon_press;
            node->size.x -= Input::getMouseDelta().x / style->grid_size;
            node->position.x += Input::getMouseDelta().x / style->grid_size;
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
        if (element_type_upon_press == OUTPUT_PIN)
        {
            auto cur = *node_upon_press;
            for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
            {
                auto node          = *it;
                if (!checkNodeBox(node, node_space_pos / style->grid_size, true))
                    continue;
                if (!node->minimised)
                {
                    int actual_index = -1;
                    int input_index = -1;
                    for (const auto& element : node->elements)
                    {
                        if (element.type == ELEMENT_INPUT)
                            ++input_index;
                        ++actual_index;
                        glm::vec2 selbox_min;
                        if (!checkPinBox(node, actual_index, node_space_pos / style->grid_size, style, selbox_min))
                            continue;
                        
                        if (element.type == ELEMENT_OUTPUT)
                            continue;

                        makeLink(cur, input_output_element_index, node, input_index);
                        break;
                    }
                }
                break;
            }
        }
        else if (element_type_upon_press == INPUT_PIN)
        {
            auto cur = *node_upon_press;
            for (auto it = nodes.rbegin(); it != nodes.rend(); ++it)
            {
                auto node          = *it;
                if (!checkNodeBox(node, node_space_pos / style->grid_size, true))
                    continue;
                if (!node->minimised)
                {
                    int actual_index = -1;
                    int output_index = -1;
                    for (const auto& element : node->elements)
                    {
                        if (element.type == ELEMENT_OUTPUT)
                            ++output_index;
                        ++actual_index;
                        glm::vec2 selbox_min;
                        if (!checkPinBox(node, actual_index, node_space_pos / style->grid_size, style, selbox_min))
                            continue;
                        
                        if (element.type == ELEMENT_INPUT)
                            continue;

                        makeLink(node, output_index, cur, input_output_element_index);
                        break;
                    }
                }
                break;
            }
        }
        else if (element_type_upon_press == RIGHT_RESIZE || element_type_upon_press == LEFT_RESIZE)
        {
            auto node = *node_upon_press;
            node->size.x = glm::round(node->size.x);
            node->position.x = glm::round(node->position.x);
        }
        else
        {
            for (auto& n : nodes)
                if (n->highlighted) n->position = glm::round(n->position);
        }
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

    return true;
}
