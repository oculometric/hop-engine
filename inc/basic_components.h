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
#include "material.h"
#include "scene.h"
#include "user_interface.h"

#include <glm/glm.hpp>

namespace HopEngine
{

/**
 * @brief camera scene component. needed to render the scene.
 */
class CameraComponent final : public Component
{
private:
    Ref<UniformBlock> uniforms;        // uniform buffer, initialised to match `SceneUniforms` for set 0
    Ref<UniformBlock> object_uniforms; // uniform buffer, initialised to match `ObjectUniforms` for set 1
    Ref<Material> camera_gizmo;

public:
    float fov       = 90.0f;  // vertical field of view
    float near_clip = 0.01f;  // distance of the near clip plane
    float far_clip  = 100.0f; // distance of the far clip plane
    // index of the slot into which the camera will render, used by the render graph
    size_t camera_slot = 0;
    // background fill colour (ignored if the target render pass is transparent)
    glm::vec3 clear_colour = { 0.004f, 0.509f, 0.506f };

public:
    DELETE_NOT_ALL_CONSTRUCTORS(CameraComponent);
    CameraComponent()           = default;
    ~CameraComponent() override = default;

    void awake() override;

    /**
     * @brief updates and fetches the scene/camera uniform buffer needed for rendering from the camera's
     * perspective.
     * @param viewport_size size of the target viewport in pixels.
     * @param lights array of lights which should affect objects rendered by this camera.
     * @param ambient ambient light colour of the scene.
     * @returns uniform buffer which all parameters updated correctly.
     */
    WeakRef<UniformBlock> getUniforms(glm::ivec2 viewport_size, const std::vector<LightParams>& lights,
        glm::vec4 ambient);
    /**
     * @brief calculates the transformation matrix which transforms world-space points into clip space.
     * @returns 4x4 world-to-clip matrix.
     */
    glm::mat4 getWorldToScreenMatrix();

    std::vector<DrawCommand> getDrawCommands() override;

    void drawImGuiDebug() override;
};

/**
 * @brief basic mesh scene component. used to render basic meshes with materials.
 */
class StaticMeshComponent : public Component
{
private:
    Ref<UniformBlock> uniforms; // uniform buffer, initialised to match `ObjectUniforms` for set 1

public:
    Ref<Mesh> mesh;                    // mesh which will be rendered
    Ref<Material> material;            // material which will be used to render the mesh
    uint32_t camera_mask = 0x000000FF; // bitflag mask for which camera slots this mesh will be visible in

public:
    DELETE_NOT_ALL_CONSTRUCTORS(StaticMeshComponent);
    StaticMeshComponent()           = default;
    ~StaticMeshComponent() override = default;

    void awake() override;

    std::vector<DrawCommand> getDrawCommands() override;
    BoundingBox getLocalBounds() const override;

    void drawImGuiDebug() override;
};

/**
 * @brief basic light scene component. passed into `SceneUniforms` uniform buffer for shaders to use.
 */
class LightComponent final : public Component
{
private:
    Ref<UniformBlock> object_uniforms; // uniform buffer, initialised to match `ObjectUniforms` for set 1
    Ref<Material> light_gizmo;

public:
    /**
     * @brief enumerates types of lighting behaviour.
     */
    enum LightType
    {
        POINT,      // point light, emitting in all directions
        SPOT,       // spotlight, emitting in a cone with half-angle `spot_angle`
        DIRECTIONAL // global directional light with parallel rays and no falloff
    };

public:
    LightType type;                 // rendering behaviour of light
    glm::vec3 colour = { 1, 0, 0 }; // colour of the light emitted
    float strength   = 1.0f;        // multiplier for light emission colour
    float spot_angle = 0.0f;        // half-angle of spotlights, ignored otherwise

public:
    DELETE_NOT_ALL_CONSTRUCTORS(LightComponent);
    LightComponent()           = default;
    ~LightComponent() override = default;

    LightParams getParamsStructure() const;

    void awake() override;
    std::vector<DrawCommand> getDrawCommands() override;

    void drawImGuiDebug() override;
};

/**
 * @brief simple scene component for drawing world-space text.
 */
class TextComponent final : public StaticMeshComponent
{
private:
    std::string text;             // text to render
    Ref<Font> font;               // font to use for text mesh generation and rendering
    glm::vec3 tint = { 0, 0, 0 }; // colour of the text

public:
    DELETE_NOT_ALL_CONSTRUCTORS(TextComponent);
    TextComponent()           = default;
    ~TextComponent() override = default;

    void awake() override;

    std::string getText() const { return text; }
    void setText(const std::string& value)
    {
        text = value;
        updateGeometry();
    }
    void setTint(const glm::vec3& value)
    {
        tint = value;
        updateGeometry();
    }

    void drawImGuiDebug() override;

private:
    void updateGeometry();
};

} // namespace HopEngine
