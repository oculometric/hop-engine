#pragma once

namespace HopEngine
{

// graphics classes
class RenderServer;
class TransientCommandBuffer;
class DrawCommandBuffer;
class Swapchain;
class RenderPass;
class RenderGraph;
class Pipeline;
class Buffer;
class UniformBlock;
class Shader;
struct FrameStats;
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

// ui classes
class UIRenderer;
class UIStyle;
class UICanvas;
class UICanvasElement;
class UIContextMenu;

// misc
class Font;
class Application;
class Engine;
class EventServer;

}