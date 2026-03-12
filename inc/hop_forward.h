#pragma once

namespace HopEngine
{

// core graphics classes
class RenderServer;
class TransientCommandBuffer;
class DrawCommandBuffer;

// presentation graphics classes
class Swapchain;
class RenderPass;
class RenderGraph;

// rendering graphics classes
class Pipeline;
class Buffer;
class UniformBlock;
class Shader;

// resource graphics classes
class Material;
class Texture;
class Sampler;
class Mesh;

// scene tree classes
struct DrawCommand;
class Scene;
class Object;
class Component;

// component subclasses
class CameraComponent;
class StaticMeshComponent;
class LightComponent;
class TextComponent;
class NodeView;

// misc
class Font;

}