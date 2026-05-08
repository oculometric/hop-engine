/*
 * HopEngine graphics engine toolkit.
 * Copyright (C) 2025  cassette costen

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "common.h"
#include "input.h"
#include "mesh.h"
#include "scene.h"
#include "basic_components.h"

#include <functional>
#include <glm/glm.hpp>
#include <string>

namespace HopEngine
{

/**
 * @brief contains information for rendering text from a font atlas
 */
class Font final : public Destructible
{
private:
    Ref<Texture> atlas      = nullptr; // texture containing glyph bitmaps
    Ref<Texture> bold_atlas = nullptr; // texture containing glyph bitmaps
    glm::ivec2 glyph_size;             // size of each glyph in pixels
    glm::ivec2 chars_resolution;       // number of glyphs in the texture
    glm::vec2 char_uv_size;            // size of a glyph, as a fraction of the texture

public:
    DELETE_CONSTRUCTORS(Font);
    Font(const std::string& atlas_name, glm::ivec2 glyph_size_pixels,
        const std::string& bold_atlas_name = "");
    Font(std::vector<Ref<Texture>> atlases, glm::ivec2 glyph_size_pixels);
    ~Font() override;

    WeakRef<Texture> getAtlas() const { return atlas; }
    WeakRef<Texture> getBoldAtlas() const { return bold_atlas; }
    glm::vec2 getGlyphSize() const { return glyph_size; }
    glm::vec2 getGlyphUVOffset(char c) const;
    glm::vec2 getGlyphUVSize() const { return char_uv_size; }

    static Ref<Font> deserialise(const std::string& path);

private:
    void createFromAtlases(const std::vector<Ref<Texture>>& atlases, glm::ivec2 glyph_size_pixels);
};

class UIStyle final : public Destructible
{
public:
    DELETE_NOT_ALL_CONSTRUCTORS(UIStyle);
    UIStyle();
    ~UIStyle() override;

    Ref<Shader> shader;
    Ref<Font> font;
    Ref<Texture> ui_atlas;

    Ref<Material> makeMaterial(bool world_space);
};

class UIRenderer final : public Destructible
{
public:
    enum TextAlign : int8_t
    {
        TEXT_ALIGN_LEFT   = -1,
        TEXT_ALIGN_CENTER = 0,
        TEXT_ALIGN_RIGHT  = 1
    };

    enum TextFlags : uint8_t
    {
        TEXT_FLAGS_NONE          = 0,
        TEXT_FLAGS_BOLD          = 1,
        TEXT_FLAGS_ITALIC        = 2,
        TEXT_FLAGS_UNDERLINE     = 4,
        TEXT_FLAGS_STRIKETHROUGH = 8
    };

    struct TextFormatting final
    {
        TextAlign align           = TEXT_ALIGN_LEFT;
        TextFlags flags           = TEXT_FLAGS_NONE;
        bool terminate_at_newline = false;
        bool wrap                 = false;
        glm::ivec2 clip_bounds    = { 0, 0 };
        int spacing               = 0;
    };

    struct BackingData final
    {
        friend class UIRenderer;

    private:
        uint32_t id = 0;
    };

private:
    struct BackingDataInternal final
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
    std::map<uint32_t, BackingDataInternal> backing_datas;
    uint32_t next_id = 0;

public:
    DELETE_CONSTRUCTORS(UIRenderer);
    UIRenderer(Ref<UIStyle> style);
    ~UIRenderer() override;

    void clear();
    void addQuad(float z, BackingData& backing_ref);
    void addQuad(float z);
    void addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float z, glm::vec2 uv_tl,
        glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent, BackingData& backing_ref);
    void addQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, float z, glm::vec2 uv_tl,
        glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent);
    glm::vec2 addText(glm::vec2 position, float z, TextFormatting formatting, const std::string& text,
        glm::vec3 colour, BackingData& backing);
    glm::vec2 addText(glm::vec2 position, float z, TextFormatting formatting, const std::string& text,
        glm::vec3 colour);
    void addNineSlice(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec3 fill,
        BackingData& backing);
    void addNineSlice(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec3 fill);
    void addSimple(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec2 uv_base,
        glm::vec2 uv_size, BackingData& backing);
    void addSimple(glm::vec2 position, float z, glm::vec2 size, int layer, glm::vec2 uv_base,
        glm::vec2 uv_size);
    void finalise();

    void setWorldSpace(bool world_space);

    DrawCommand draw() const { return DrawCommand(material, mesh); }

private:
    bool isBackingValid(const BackingData& backing_ref);
    void addBacking(BackingData& backing_ref, BackingDataInternal backing);
    void updateTextSingleLine(glm::vec2 position, TextFormatting formatting, const std::string& text,
        glm::vec3 colour, BackingDataInternal backing);
    void updateQuad(glm::vec2 p1, glm::vec2 p2, glm::vec2 p3, glm::vec2 p4, glm::vec2 uv_tl,
        glm::vec2 uv_br, glm::vec4 colour, glm::vec4 normal, glm::vec4 tangent,
        BackingDataInternal backing);
};

struct UITransform final
{
    enum Anchor
    {
        ANCHOR_TOP_LEFT,
        ANCHOR_TOP_CENTER,
        ANCHOR_TOP_RIGHT,
        ANCHOR_MIDDLE_LEFT,
        ANCHOR_MIDDLE_CENTER,
        ANCHOR_MIDDLE_RIGHT,
        ANCHOR_BOTTOM_LEFT,
        ANCHOR_BOTTOM_CENTER,
        ANCHOR_BOTTOM_RIGHT
    };

    enum Scaling
    {
        SCALING_NONE            = 0b00,
        SCALING_FILL_HORIZONTAL = 0b01,
        SCALING_FILL_VERTICAL   = 0b10,
        SCALING_FILL_BOTH       = 0b11
    };

    glm::vec2 offset       = { 0, 0 };
    Anchor external_anchor = ANCHOR_TOP_LEFT;
    Anchor internal_anchor = ANCHOR_TOP_LEFT;

    glm::vec2 size  = { 10, 10 };
    Scaling scaling = SCALING_NONE;

    float rotation         = 0;
    Anchor rotation_anchor = ANCHOR_MIDDLE_CENTER;

    glm::mat3 transform = glm::mat3(1);
};

struct UIHierarchy final : public Destructible
{
    Ref<UICanvasElement> element;
    WeakRef<UIHierarchy> parent;
    std::vector<Ref<UIHierarchy>> children;

    DELETE_NOT_ALL_CONSTRUCTORS(UIHierarchy);
    UIHierarchy()           = default;
    ~UIHierarchy() override = default;
};

class UICanvasElement : public Destructible
{
    friend class UICanvas;

private:
    UITransform transform;
    Ref<UIRenderer> renderer;
    WeakRef<UIHierarchy> hierarchy;
    glm::vec2 last_parent_size;
    glm::mat3 last_parent_transform;
    bool needs_rebuild = true;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(UICanvasElement);
    UICanvasElement() = default;
    ~UICanvasElement() override;

    void setPosition(glm::vec2 position)
    {
        transform.offset = position;
        layout(last_parent_size, last_parent_transform);
    }
    glm::vec2 getPosition() const { return transform.offset; }
    void setSize(glm::vec2 size)
    {
        transform.size = size;
        layout(last_parent_size, last_parent_transform);
    }
    glm::vec2 getSize() const { return transform.size; }
    void setRotation(float degrees)
    {
        transform.rotation = degrees;
        layout(last_parent_size, last_parent_transform);
    }
    float getRotation() const { return transform.rotation; }
    void setExternalAnchor(UITransform::Anchor anchor)
    {
        transform.external_anchor = anchor;
        layout(last_parent_size, last_parent_transform);
    }
    UITransform::Anchor getExternalAnchor() const { return transform.external_anchor; }
    void setInternalAnchor(UITransform::Anchor anchor)
    {
        transform.internal_anchor = anchor;
        layout(last_parent_size, last_parent_transform);
    }
    UITransform::Anchor getInternalAnchor() const { return transform.internal_anchor; }
    void setScaling(UITransform::Scaling scaling)
    {
        transform.scaling = scaling;
        layout(last_parent_size, last_parent_transform);
    }
    UITransform::Scaling getScaling() const { return transform.scaling; }
    void setRotationAnchor(UITransform::Anchor anchor)
    {
        transform.rotation_anchor = anchor;
        layout(last_parent_size, last_parent_transform);
    }
    UITransform::Anchor getRotationAnchor() const { return transform.rotation_anchor; }

protected:
    void setNeedsRebuild() { needs_rebuild = true; }
    glm::mat3 getTransform() const { return transform.transform; }
    WeakRef<UIRenderer> getRenderer() const { return renderer; }
    void layout(glm::vec2 parent_size, glm::mat3 parent_transform);

    virtual void build() {};
    virtual void onMouseEnter() {};
    virtual void onMouseMove(glm::vec2 local_pos, glm::vec2 delta) {};
    virtual void onMouseExit() {};
    virtual void onMouseDown(Input::MouseButton button, glm::vec2 local_pos) {};
    virtual void onMouseDrag(Input::MouseButton button, glm::vec2 local_pos, glm::vec2 delta) {};
    virtual void onMouseUp(Input::MouseButton button, glm::vec2 local_pos) {};
    virtual void onMouseClick(Input::MouseButton button, glm::vec2 local_pos) {};
    virtual void onKeyDown(Input::KeyboardKey key) {};
    virtual void onKeyUp(Input::KeyboardKey key) {};
};

class UICanvas final : public Destructible
{
private:
    Ref<UIRenderer> renderer;
    Ref<UIHierarchy> hierarchy;
    std::vector<Ref<UICanvasElement>> elements;
    glm::vec2 canvas_size;

public:
    DELETE_CONSTRUCTORS(UICanvas);
    UICanvas(glm::vec2 size);
    ~UICanvas() override = default;

    void build();
    void layout();
    template<class T> WeakRef<T> addElement();
    template<class T, class Q> WeakRef<T> addChild(WeakRef<Q> parent);

    bool checkInput();
    void resize(glm::vec2 new_size);
    DrawCommand draw() const { return renderer->draw(); }
    glm::vec2 getSize() const { return canvas_size; }
    void setWorldSpace(bool world_space) { renderer->setWorldSpace(world_space); }
};

template<class T> inline WeakRef<T> UICanvas::addElement()
{
    static_assert(std::is_convertible_v<T*, UICanvasElement*>,
        "T must be a HopEngine::UICanvasElement subclass");
    return addChild<T, UICanvasElement>(hierarchy->element);
}

template<class T, class Q> inline WeakRef<T> UICanvas::addChild(WeakRef<Q> parent)
{
    static_assert(std::is_convertible_v<T*, UICanvasElement*>,
        "T must be a HopEngine::UICanvasElement subclass");
    static_assert(std::is_convertible_v<Q*, UICanvasElement*>,
        "Q must be a HopEngine::UICanvasElement subclass");

    // if (!elements.find(parent))
    // {
    //     DBG_ERROR("UICanvas hierarchy does not contain the specified parent!");
    //     return nullptr;
    // }

    // create new element of specified type
    Ref<T> element    = new T();
    element->renderer = renderer;
    // assign element into new hierarchy, as a child of the root hierarchy
    Ref<UIHierarchy> h = new UIHierarchy();
    h->element         = element.template cast<UICanvasElement>();
    element->hierarchy = h;
    h->parent          = parent->hierarchy;
    parent->hierarchy->children.push_back(h);
    // add to mapping
    elements.push_back(h->element);

    element->layout(parent->transform.size, parent->transform.transform);

    return element;
}

class UIManager final
{
    friend class InitMachine;

private:
    std::vector<std::pair<Ref<UICanvas>, glm::vec2>> canvases;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(UIManager);

    static Ref<UICanvas> push(Ref<UICanvas> canvas, glm::vec2 offset);
    static void pop();
    static Ref<UICanvas> peek();

    static void draw(WeakRef<DrawCommandBuffer> command_buffer);

private:
    UIManager() = default;

    static void init();
    static void destroy();
};

class UICanvasComponent final : public StaticMeshComponent
{
private:
    Ref<UICanvas> canvas;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(UICanvasComponent);
    UICanvasComponent()           = default;
    ~UICanvasComponent() override = default;

    void awake() override;

    std::vector<DrawCommand> getDrawCommands() override;
    Ref<UICanvas> getCanvas() const { return canvas; }
};

class UILabel final : public UICanvasElement
{
private:
    std::string text;
    UIRenderer::BackingData text_backing;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(UILabel);
    UILabel() = default;

    void setText(const std::string& new_text)
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

    void build() override;
};

class UIPanel final : public UICanvasElement
{
private:
    glm::vec4 colour;
    UIRenderer::BackingData panel_backing;

public:
    DELETE_NOT_ALL_CONSTRUCTORS(UIPanel);
    UIPanel() = default;

    void setColour(glm::vec4 new_colour)
    {
        colour = new_colour;
        build();
    }

    void build() override;
};

} // namespace HopEngine