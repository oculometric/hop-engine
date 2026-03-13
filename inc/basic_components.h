#pragma once

#include <glm/glm.hpp>

#include "common.h"
#include "scene.h"
#include "material.h"
#include "font.h"

namespace HopEngine
{

class CameraComponent final : public Component
{
private:
	Ref<UniformBlock> uniforms;

public:
	float fov = 90.0f;
	float near_clip = 0.01f;
	float far_clip = 100.0f;
	glm::vec3 clear_colour = { 0.004f, 0.509f, 0.506f };
	size_t camera_slot = 0;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(CameraComponent);
	CameraComponent() = default;
	~CameraComponent() override = default;

	void awake() override;

	WeakRef<UniformBlock> getUniforms(glm::ivec2 viewport_size, const std::vector<LightParams>& lights, glm::vec4 ambient);	
	glm::mat4 getWorldToScreenMatrix();
	
	void drawImGuiDebug() override;
};

class StaticMeshComponent : public Component
{
private:
	Ref<UniformBlock> uniforms;

public:
	Ref<Mesh> mesh;
	Ref<Material> material;
	uint32_t camera_mask = 0x000000FF;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(StaticMeshComponent);
	StaticMeshComponent() = default;
	~StaticMeshComponent() override = default;

	void awake() override;

	std::vector<DrawCommand> getDrawCommands() override;
	BoundingBox getLocalBounds() const override;
	
	void drawImGuiDebug() override;
};

class LightComponent final : public Component
{
public:
	enum LightType
	{
		POINT,
		SPOT,
		DIRECTIONAL
	};

public:
	LightType type;
	glm::vec3 colour = { 1, 0, 0 };
	float strength = 1.0f;
	float spot_angle = 0.0f;

public:
	DELETE_NOT_ALL_CONSTRUCTORS(LightComponent);
	LightComponent() = default;
	~LightComponent() override = default;

	LightParams getParamsStructure() const;
	
	void drawImGuiDebug() override;
};

class TextComponent final : public StaticMeshComponent
{
private:
    std::string text;
    Ref<Font> font;
    glm::vec3 tint = { 0, 0, 0 };
    
public:
    DELETE_NOT_ALL_CONSTRUCTORS(TextComponent);
	TextComponent() = default;
	~TextComponent() override = default;

	void awake() override;
    
    std::string getText() const { return text; }
    void setText(const std::string& value) { text = value; updateGeometry(); }
    void setTint(const glm::vec3& value) { tint = value; updateGeometry(); }
    
	void drawImGuiDebug() override;
    
private:
    void updateGeometry();
};

}
