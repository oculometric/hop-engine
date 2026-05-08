#include "user_interface.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "command_buffer.h"
#include "engine.h"
#include "input.h"
#include "material.h"
#include "render_server.h"

#include <glm/gtx/matrix_transform_2d.hpp>

using namespace HopEngine;

UIStyle::UIStyle()
{
    shader   = Engine::loadShader("res://engine/shaders/user_interface.glsl");
    font     = Font::deserialise("res://engine/IBM_font.hfnt");
    ui_atlas = Engine::loadTexture3D("res://engine/textures/ui_kit_98.png", 4, 4);
}

UIStyle::~UIStyle() {}

Ref<Material> UIStyle::makeMaterial()
{
    Ref mat = new Material(shader,
        Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthTest(false).depthWrite(false),
        Framebuffer::getSwapchainConfig());
    mat->setTextureSampler("text_atlas", font->getAtlas().strong(),
        Engine::getSampler(Sampler::FILTER_NEAREST));
    mat->setTextureSampler("text_bold_atlas", font->getBoldAtlas().strong(),
        Engine::getSampler(Sampler::FILTER_NEAREST));
    mat->setTextureSampler("ui_atlas", ui_atlas, Engine::getSampler(Sampler::FILTER_NEAREST));
    return mat;
}

UICanvasElement::~UICanvasElement() { DBG_INFO("destroying ui element"); }

void UICanvasElement::layout(glm::vec2 parent_size, glm::mat3 parent_transform)
{
    last_parent_size      = parent_size;
    last_parent_transform = parent_transform;

    glm::vec2 parent_anchor_pos = { 0, 0 };
    switch (transform.external_anchor)
    {
    case UITransform::ANCHOR_TOP_LEFT:      parent_anchor_pos = { 0, 0 }; break;
    case UITransform::ANCHOR_TOP_CENTER:    parent_anchor_pos = { parent_size.x / 2.0f, 0 }; break;
    case UITransform::ANCHOR_TOP_RIGHT:     parent_anchor_pos = { parent_size.x, 0 }; break;
    case UITransform::ANCHOR_MIDDLE_LEFT:   parent_anchor_pos = { 0, parent_size.y / 2.0f }; break;
    case UITransform::ANCHOR_MIDDLE_CENTER: parent_anchor_pos = parent_size / 2.0f; break;
    case UITransform::ANCHOR_MIDDLE_RIGHT:
        parent_anchor_pos = { parent_size.x, parent_size.y / 2.0f };
        break;
    case UITransform::ANCHOR_BOTTOM_LEFT: parent_anchor_pos = { 0, parent_size.y }; break;
    case UITransform::ANCHOR_BOTTOM_CENTER:
        parent_anchor_pos = { parent_size.x / 2.0f, parent_size.y };
        break;
    case UITransform::ANCHOR_BOTTOM_RIGHT: parent_anchor_pos = parent_size; break;
    }

    if (transform.scaling & UITransform::SCALING_FILL_HORIZONTAL) transform.size.x = parent_size.x;
    if (transform.scaling & UITransform::SCALING_FILL_VERTICAL) transform.size.y = parent_size.y;

    glm::vec2 self_anchor_pos = { 0, 0 };
    switch (transform.internal_anchor)
    {
    case UITransform::ANCHOR_TOP_LEFT:      self_anchor_pos = { 0, 0 }; break;
    case UITransform::ANCHOR_TOP_CENTER:    self_anchor_pos = { transform.size.x / 2.0f, 0 }; break;
    case UITransform::ANCHOR_TOP_RIGHT:     self_anchor_pos = { transform.size.x, 0 }; break;
    case UITransform::ANCHOR_MIDDLE_LEFT:   self_anchor_pos = { 0, transform.size.y / 2.0f }; break;
    case UITransform::ANCHOR_MIDDLE_CENTER: self_anchor_pos = transform.size / 2.0f; break;
    case UITransform::ANCHOR_MIDDLE_RIGHT:
        self_anchor_pos = { transform.size.x, transform.size.y / 2.0f };
        break;
    case UITransform::ANCHOR_BOTTOM_LEFT: self_anchor_pos = { 0, transform.size.y }; break;
    case UITransform::ANCHOR_BOTTOM_CENTER:
        self_anchor_pos = { transform.size.x / 2.0f, transform.size.y };
        break;
    case UITransform::ANCHOR_BOTTOM_RIGHT: self_anchor_pos = transform.size; break;
    }

    glm::vec2 translation = (parent_anchor_pos - self_anchor_pos) + transform.offset;

    glm::vec2 rotation_anchor_pos = { 0, 0 };
    switch (transform.rotation_anchor)
    {
    case UITransform::ANCHOR_TOP_LEFT:      rotation_anchor_pos = { 0, 0 }; break;
    case UITransform::ANCHOR_TOP_CENTER:    rotation_anchor_pos = { transform.size.x / 2.0f, 0 }; break;
    case UITransform::ANCHOR_TOP_RIGHT:     rotation_anchor_pos = { transform.size.x, 0 }; break;
    case UITransform::ANCHOR_MIDDLE_LEFT:   rotation_anchor_pos = { 0, transform.size.y / 2.0f }; break;
    case UITransform::ANCHOR_MIDDLE_CENTER: rotation_anchor_pos = transform.size / 2.0f; break;
    case UITransform::ANCHOR_MIDDLE_RIGHT:
        rotation_anchor_pos = { transform.size.x, transform.size.y / 2.0f };
        break;
    case UITransform::ANCHOR_BOTTOM_LEFT: rotation_anchor_pos = { 0, transform.size.y }; break;
    case UITransform::ANCHOR_BOTTOM_CENTER:
        rotation_anchor_pos = { transform.size.x / 2.0f, transform.size.y };
        break;
    case UITransform::ANCHOR_BOTTOM_RIGHT: rotation_anchor_pos = transform.size; break;
    }

    glm::mat3 matrix = glm::mat3(1);
    matrix           = glm::translate(matrix, translation);
    matrix           = glm::translate(matrix, rotation_anchor_pos);
    matrix           = glm::rotate(matrix, transform.rotation);
    matrix           = glm::translate(matrix, -rotation_anchor_pos);

    transform.transform = parent_transform * matrix;

    this->build();

    if (hierarchy)
    {
        for (const auto& child : hierarchy->children)
        {
            child->element->layout(transform.size, transform.transform);
            child->element->build();
        }
    }
}

UICanvas::UICanvas(glm::vec2 size)
{
    renderer                              = new UIRenderer(new UIStyle());
    hierarchy                             = new UIHierarchy();
    hierarchy->element                    = new UICanvasElement();
    hierarchy->element->hierarchy         = hierarchy;
    hierarchy->element->transform.scaling = UITransform::SCALING_FILL_BOTH;
    canvas_size                           = size;
    elements.push_back(hierarchy->element);
}

void UICanvas::build()
{
    bool rebuild_needed = false;
    for (const auto& elem : elements)
    {
        rebuild_needed |= elem->needs_rebuild;
        elem->needs_rebuild = false;
    }

    if (rebuild_needed) renderer->clear();

    for (const auto& elem : elements) elem->build();

    renderer->finalise();
}

void UICanvas::layout()
{ hierarchy->element->layout(canvas_size, glm::translate(glm::mat3(1), canvas_size / -2.0f)); }

// UIContextMenu::UIContextMenu(glm::vec2 position)
// {
//     top_corner = position;
//     renderer = new UIRenderer(new UIStyle());
// }

// void UIContextMenu::addText(const std::string& text)
// {
//     elements.emplace_back(text, false, nullptr);
// }

// void UIContextMenu::addButton(const std::string& text, std::function<void()> callback)
// {
//     elements.emplace_back(text, true, callback);
// }

// void UIContextMenu::done()
// {
//     renderer->clear();
//     renderer->addNineSlice(top_corner - glm::vec2{ 5, 5 }, -1.0f, glm::vec2{ 220, elements.size() * 32 }
//     + glm::vec2{ 10, 10 }, 1, glm::vec3{ 0.8f, 0.8f, 0.8f }); size_t element_index = 0; for (const auto&
//     elem : elements)
//     {
//         if (std::get<bool>(elem))
//             renderer->addNineSlice(top_corner + glm::vec2{ 0, (element_index * 32) + 0 }, 0.0f,
//             glm::vec2{ 220, 32 }, 0, glm::vec3{ 0.8f, 0.8f, 0.8f });
//         renderer->addText(top_corner + glm::vec2{ 5, (element_index * 32) + 5 }, 1.0f, {
//         UIRenderer::TEXT_ALIGN_LEFT, UIRenderer::TEXT_FLAGS_NONE }, std::get<std::string>(elem),
//         glm::vec3{ 0, 0, 0 });

//         ++element_index;
//     }

//     renderer->addText(glm::vec2{ 0, 0 }, 2.0f,
//         UIRenderer::TextFormatting{
//             .align = UIRenderer::TEXT_ALIGN_CENTER,
//             .flags = UIRenderer::TextFlags(UIRenderer::TEXT_FLAGS_ITALIC |
//             UIRenderer::TEXT_FLAGS_STRIKETHROUGH | UIRenderer::TEXT_FLAGS_UNDERLINE), .wrap = true,
//             .clip_bounds = { 250, 100 }
//         }, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam bibendum massa ut urna laoreet
//         vehicula.", { 0, 0, 0 });

//     renderer->finalise();
// }

// bool UIContextMenu::checkInput()
// {
//     glm::vec2 bounds_min = top_corner;
//     glm::vec2 bounds_max = top_corner + glm::vec2{ 220, (elements.size() * 32) };
//     glm::vec2 mouse_pos = Input::getMousePosition();

//     if (mouse_pos.x < bounds_min.x || mouse_pos.x > bounds_max.x
//      || mouse_pos.y < bounds_min.y || mouse_pos.y > bounds_max.y)
//         return false;

//     return true;
// }

static UIManager* manager;

Ref<UICanvas> UIManager::push(Ref<UICanvas> canvas, glm::vec2 offset)
{
    manager->canvases.emplace_back(canvas, offset);
    return canvas;
}

void UIManager::pop()
{
    if (manager->canvases.empty()) return;
    manager->canvases.pop_back();
}

Ref<UICanvas> UIManager::peek()
{
    if (manager->canvases.empty()) return nullptr;

    return manager->canvases.rbegin()->first;
}

void UIManager::draw(WeakRef<DrawCommandBuffer> command_buffer)
{
    if (!manager) return;
    for (const auto& [canvas, offset] : manager->canvases)
    {
        command_buffer->setScissorViewport(offset / RenderServer::getFramebufferSize(),
            canvas->getSize() / RenderServer::getFramebufferSize());
        canvas->build();
        auto command = canvas->draw();
        command.material->bind(command_buffer);
        command.mesh->draw(command_buffer);
    }
}

void UIManager::init() { manager = new UIManager(); }

void UIManager::destroy()
{
    delete manager;
    manager = nullptr;
}

void UILabel::build()
{
    getRenderer()->addText(glm::vec2{ 0, 0 }, 0.0f, UIRenderer::TextFormatting{}, text,
        glm::vec3{ 0, 1, 0 }, text_backing);
}
