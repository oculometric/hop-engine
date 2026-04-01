#include "user_interface.h"

#include "engine.h"
#include "input.h"

using namespace HopEngine;

UIStyle::UIStyle()
{
    shader = Engine::loadShader("res://engine/shaders/user_interface.glsl");
    font = Font::deserialise("res://engine/IBM_font.hfnt");
    ui_atlas = Engine::loadTexture3D("res://engine/textures/ui_kit_98.png", 4, 4);
}

UIStyle::~UIStyle() { }

Ref<Material> UIStyle::makeMaterial()
{
    Ref mat = new Material(shader, Pipeline::Builder().cullMode(Pipeline::CULL_NONE).depthTest(false).depthWrite(false));
    mat->setTexture(1, font->getAtlas().strong());
    mat->setTexture(2, font->getBoldAtlas().strong());
    mat->setTexture(3, ui_atlas);
    mat->setSampler(1, Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_NEAREST)));
    mat->setSampler(2, Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_NEAREST)));
    mat->setSampler(3, Engine::makeSampler(Sampler::Builder().filter(Sampler::FILTER_NEAREST)));
    return mat;
}

UIContextMenu::UIContextMenu(glm::vec2 position)
{
    top_corner = position;
    renderer = new UIRenderer(new UIStyle());
}

void UIContextMenu::addText(const std::string& text)
{
    elements.emplace_back(text, false, nullptr);
}

void UIContextMenu::addButton(const std::string& text, std::function<void()> callback)
{
    elements.emplace_back(text, true, callback);
}

void UIContextMenu::done()
{
    renderer->clear();
    renderer->addNineSlice(top_corner - glm::vec2{ 5, 5 }, -1.0f, glm::vec2{ 220, elements.size() * 32 } + glm::vec2{ 10, 10 }, 1, glm::vec3{ 0.8f, 0.8f, 0.8f });
    size_t element_index = 0;
    for (const auto& elem : elements)
    {
        if (std::get<bool>(elem))
            renderer->addNineSlice(top_corner + glm::vec2{ 0, (element_index * 32) + 0 }, 0.0f, glm::vec2{ 220, 32 }, 0, glm::vec3{ 0.8f, 0.8f, 0.8f });
        renderer->addText(top_corner + glm::vec2{ 5, (element_index * 32) + 5 }, 1.0f, { UIRenderer::TEXT_ALIGN_LEFT, UIRenderer::TEXT_FLAGS_NONE }, std::get<std::string>(elem), glm::vec3{ 0, 0, 0 });

        ++element_index;
    }

    renderer->addText(glm::vec2{ 0, 0 }, 2.0f,
        UIRenderer::TextFormatting{
            .align = UIRenderer::TEXT_ALIGN_CENTER,
            .flags = UIRenderer::TextFlags(UIRenderer::TEXT_FLAGS_UNDERLINE | UIRenderer::TEXT_FLAGS_STRIKETHROUGH),
            .wrap = true,
            .clip_bounds = { 450, 0 }
        }, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nam bibendum massa ut urna laoreet vehicula.", { 0, 0, 0 });

    renderer->finalise();
}

bool UIContextMenu::checkInput()
{
    glm::vec2 bounds_min = top_corner;
    glm::vec2 bounds_max = top_corner + glm::vec2{ 220, (elements.size() * 32) };
    glm::vec2 mouse_pos = Input::getMousePosition();

    if (mouse_pos.x < bounds_min.x || mouse_pos.x > bounds_max.x
     || mouse_pos.y < bounds_min.y || mouse_pos.y > bounds_max.y)
        return false;

    return true;
}
