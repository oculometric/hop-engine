#pragma once

// provides early forward definitions for most outward-facing classes

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

class Font;
class UIRenderer;
class UIStyle;
class UICanvas;
class UICanvasElement;
class UIContextMenu;

// misc

class Application;
class Engine;
class Package;
class Input;
class EventServer;

// generic placeholder type for various API-specific resource handle types
typedef void* GPUHandle;
// typedef for a block of data, expressed as a managed byte array
typedef std::vector<uint8_t> DataBlock;

}