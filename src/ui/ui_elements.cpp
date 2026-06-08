#include "user_interface.h"

using namespace HopEngine;

void UILabel::setText(const std::string& new_text)
{
    if (text.size() != new_text.size())
    {
        text = new_text;
        setNeedsRebuild();
    }
    else
    {
        text = new_text;
        build();
    }
}

void UILabel::setFormatting(const UIRenderer::TextFormatting& new_formatting)
{
    formatting = new_formatting;
    build();
}

void UILabel::setColour(glm::vec3 new_colour)
{
    colour = new_colour;
    build();
}

void UILabel::build()
{
    getRenderer()->setTransformation(getTransform());
    formatting.clip_bounds = getSize();
    getRenderer()->addText({ 0, 0 }, 0.0f, formatting, text, colour, text_backing);
}

void UIPanel::setColour(glm::vec4 new_colour)
{
    colour = new_colour;
    build();
}

void UIPanel::setStyle(int new_style)
{
    style = new_style;
    build();
}

void UIPanel::build()
{
    getRenderer()->setTransformation(getTransform());
    getRenderer()->addNineSlice({ 0, 0 }, 0.0f, getSize(), style, colour, panel_backing);
}

void UIButton::setText(const std::string& new_text)
{
    if (text.size() != new_text.size())
    {
        text = new_text;
        setNeedsRebuild();
    }
    else
    {
        text = new_text;
        build();
    }
}

void UIButton::setFormatting(const UIRenderer::TextFormatting& new_formatting)
{
    formatting = new_formatting;
    build();
}

void UIButton::setTextColour(glm::vec3 new_colour)
{
    text_colour = new_colour;
    build();
}

void UIButton::setBackgroundColour(glm::vec3 new_colour)
{
    colour = new_colour;
    build();
}

void UIButton::setIcon(bool show_icon, int new_icon_index)
{
    if (show_icon != icon)
    {
        icon       = show_icon;
        icon_index = new_icon_index;
        setNeedsRebuild();
    }
    else
    {
        icon_index = new_icon_index;
        build();
    }
}

void UIButton::build()
{
    getRenderer()->setTransformation(getTransform());
    getRenderer()->addNineSlice({ 0, 0 }, 0.0f, getSize(), 1, { colour, 1 }, background_backing);
    float icon_size  = 12.0f;
    float icon_inset = (getSize().y - icon_size) / 2.0f;
    float text_inset = (getSize().y - 22.0f) / 2.0f;
    getRenderer()->addText({ icon_size + icon_inset + 4.0f, text_inset }, 0.0f, formatting, text,
        text_colour, text_backing);
    getRenderer()->addSimple(glm::vec2(icon_inset), 0.0f, glm::vec2(icon_size), (icon_index % 4) + 8,
        glm::vec2((icon_index % 2) * 0.5f, ((icon_index % 4) / 2) * 0.5f), { 0.5f, 0.5f }, icon_backing);
}

void UIIcon::setIcon(int new_icon_index, bool big)
{
    icon_index = new_icon_index;
    icon_big = big;
    build();
}

void UIIcon::build()
{
    getRenderer()->setTransformation(getTransform());
    //float icon_size  = icon_big ? 24.0f : 12.0f;
    int layer = icon_big ? icon_index : icon_index % 4;
    glm::vec2 uv_base = icon_big ? glm::vec2(0.0f) : glm::vec2((icon_index % 2) * 0.5f, ((icon_index % 4) / 2) * 0.5f);
    glm::vec2 uv_size = icon_big ? glm::vec2(1.0f) : glm::vec2(0.5f);
    getRenderer()->addSimple({ 0.0f, 0.0f }, 0.0f, getSize(), layer + 8,
        uv_base, uv_size, icon_backing);
}