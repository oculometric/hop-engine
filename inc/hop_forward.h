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

// provides early forward definitions for most outward-facing classes

namespace HopEngine
{

// graphics classes

class RenderServer;
class TransientCommandBuffer;
class DrawCommandBuffer;
class Swapchain;
class Framebuffer;
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
class UIManager;

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