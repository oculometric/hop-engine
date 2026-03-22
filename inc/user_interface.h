#pragma once

#include <string>
#include <glm/glm.hpp>
#include "common.h"

namespace HopEngine
{

class UIStyle final : public Destructible
{
public:
    DELETE_NOT_ALL_CONSTRUCTORS(UIStyle);
    UIStyle();
    ~UIStyle() override;

    Ref<Shader> shader;
    Ref<Font> font;
    Ref<Texture> ui_atlas;

    Ref<Material> makeMaterial();
};

class UIRenderer final : public Destructible
{
private:
    Ref<UIStyle> style;
    Ref<Mesh> mesh;
    Ref<Material> material;

public:
    DELETE_CONSTRUCTORS(UIRenderer);
    UIRenderer(Ref<UIStyle> style);
    ~UIRenderer();

private:
    void clear();
    void addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent);
    void addText(const std::string& text, glm::vec2 position);
    void addNineSlice(glm::vec2 position, glm::vec2 size, int layer, glm::vec3 fill);
    void addSimple(glm::vec2 position, glm::vec2 size, glm::vec2 uv_base, glm::vec2 uv_size, int layer);
    void finalise();

    DrawCommand draw():
};


}