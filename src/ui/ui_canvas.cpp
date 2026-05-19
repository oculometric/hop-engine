#include "command_buffer.h"
#include "engine.h"
#include "material.h"
#include "render_server.h"
#include "user_interface.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>

using namespace HopEngine;

UIStyle::UIStyle()
{
    shader   = Engine::loadShader("res://engine/shaders/user_interface.glsl");
    font     = Font::deserialise("res://engine/IBM_font.hfnt");
    ui_atlas = Engine::loadTexture3D("res://engine/textures/ui_kit_98.png", 4, 4);
}

UIStyle::~UIStyle() {}

Ref<Material> UIStyle::makeMaterial(bool world_space)
{
    Ref mat = new Material(shader,
        Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthTest(world_space).depthWrite(false),
        world_space ? Framebuffer::getDefaultConfig() : Framebuffer::getSwapchainConfig());
    mat->setTextureSampler("text_atlas", font->getAtlas().strong(),
        Engine::getSampler(Sampler::FILTER_NEAREST));
    mat->setTextureSampler("text_bold_atlas", font->getBoldAtlas().strong(),
        Engine::getSampler(Sampler::FILTER_NEAREST));
    mat->setTextureSampler("ui_atlas", ui_atlas, Engine::getSampler(Sampler::FILTER_NEAREST));
    mat->setBoolUniform("world_space", world_space);
    return mat;
}

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

    layout();
}

UICanvas::UICanvas() : UICanvas({ 10, 10 }) {}

void UICanvas::build()
{
    bool rebuild_needed = false;
    for (const auto& elem : elements)
    {
        rebuild_needed |= elem->needs_rebuild;
        elem->needs_rebuild = false;
    }

    if (rebuild_needed)
    {
        renderer->clear();
        for (const auto& elem : elements) elem->build();
    }

    renderer->finalise();
}

void UICanvas::layout() { hierarchy->element->layout(canvas_size, glm::mat3(1)); }

void UICanvas::resize(glm::vec2 new_size)
{
    canvas_size = new_size;
    layout();
}

Ref<UICanvas> UIManager::push(Ref<UICanvas> canvas)
{
    getInstance()->canvases.push_back(canvas);
    return canvas;
}

Ref<UICanvas> UIManager::push() { return UIManager::push(new UICanvas()); }

void UIManager::pop()
{
    if (getInstance()->canvases.empty()) return;
    getInstance()->canvases.pop_back();
}

Ref<UICanvas> UIManager::peek()
{
    if (getInstance()->canvases.empty()) return nullptr;

    return *(getInstance()->canvases.rbegin());
}

void UIManager::draw(WeakRef<DrawCommandBuffer> command_buffer)
{
    if (!getInstance()) return;
    command_buffer->setScissorViewport(glm::vec2(0.0f), glm::vec2(1.0f));
    for (auto& canvas : getInstance()->canvases)
    {
        if (canvas->getSize() != RenderServer::getFramebufferSize())
            canvas->resize(RenderServer::getFramebufferSize());
        canvas->build();
        auto command = canvas->draw();
        command.material->bind(command_buffer);
        command.mesh->draw(command_buffer);
    }
}

void UICanvasComponent::awake()
{
    StaticMeshComponent::awake();
    canvas = new UICanvas({ 100, 100 });
    canvas->setWorldSpace(true);
}

std::vector<DrawCommand> UICanvasComponent::getDrawCommands()
{
    canvas->build();
    auto command = canvas->draw();
    mesh         = command.mesh.strong();
    material     = command.material.strong();
    return StaticMeshComponent::getDrawCommands();
}
