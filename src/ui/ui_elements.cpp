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

void UIPanel::setColour(glm::vec3 new_colour)
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
