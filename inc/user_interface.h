#pragma once

#include <string>
#include <glm/glm.hpp>
#include <functional>

#include "common.h"
#include "scene.h"

namespace HopEngine
{

// TODO: move font in here

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
public:
    enum TextAlign : int8_t
    {
        LEFT   = -1,
        CENTER =  0,
        RIGHT  =  1
    };

    enum TextFlags : uint8_t
    {
        NONE          = 0,
        BOLD          = 1,
        ITALIC        = 2,
        UNDERLINE     = 4,
        STRIKETHROUGH = 8
    };

    struct TextFormatting final
    {
        TextAlign align = LEFT;
        TextFlags flags = NONE;
        bool wrap = false;
        glm::ivec2 clip_bounds = { 0, 0 };
    };

    struct BackingData final
    {
        friend class UIRenderer;
    private:
        uint16_t first_vertex;
        uint16_t vertex_count;

        uint16_t first_index;
        uint16_t index_count;
        float z;
    };

private:
    Ref<UIStyle> style;
    Ref<Mesh> mesh;
    Ref<Material> material;
    std::vector<Mesh::Vertex> vertices;
    std::map<float, std::vector<uint16_t>> indices;

public:
    DELETE_CONSTRUCTORS(UIRenderer);
    UIRenderer(Ref<UIStyle> style);
    ~UIRenderer() override;

    void               clear();
    BackingData      addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float z, glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent);
    BackingData      addText(glm::vec2 position, float z, TextFormatting formatting, const std::string& text, glm::vec3 colour);
    BackingData addNineSlice(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec3 fill);
    BackingData    addSimple(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec2 uv_base, glm::vec2 uv_size);
    void            finalise();

    void          updateQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float z, glm::vec2 uv_tl, glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent, BackingData backing);
    void          updateText(glm::vec2 position, float z, TextFormatting formatting, const std::string& text, glm::vec3 colour, BackingData backing);
    void     updateNineSlice(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec3 fill, BackingData backing);
    void        updateSimple(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec2 uv_base, glm::vec2 uv_size, BackingData backing);

    // TODO: return structures to be able to update colour, size, position, etc, at later point without clearing and recreating vertices

    DrawCommand draw() const { return DrawCommand(material, mesh); }
};

class UIContextMenu final : public Destructible
{
private:
    Ref<UIRenderer> renderer;
    glm::vec2 top_corner;
    std::vector<std::tuple<std::string, bool, std::function<void()>>> elements;

public:
    DELETE_CONSTRUCTORS(UIContextMenu);
    UIContextMenu(glm::vec2 position);

    void addText(const std::string& text);
    void addButton(const std::string& text, std::function<void()> callback);
    // TODO: submenus
    void done();

    DrawCommand draw() const { return renderer->draw(); }
    bool checkInput();
};

}