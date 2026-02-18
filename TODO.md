# TODO

## v0.52
- improve scene tree so that objects know about their scene, and children, and can be removed [M]
- object duplicate function [L]
- shader & other resource reloading at runtime [H]
- binary repackaging for textures
- render graph final passthrough needs texture interpolation control
---
## v0.6
- eliminate vulkan types being exposed outside of class implementations entirely, replace stuff like VkFormat etc
- frustrum culling [M]
- audio loading and output
- custom shader format to keep stuff in one file [L]
- improved text block rendering with wrapping, alignment, font, etc
---
## v0.7
- multiple panels/scenes (i.e. nested shader graphs, decoupling shader graph from swapchain)
- interactive node editor
- shader node editor
- render graph node editor
- shadows
- a proper procedural shading art workflow
---
- more scene control using ImGui (modify the render graph, material uniforms) [M]
- improved gizmo, better control, rotation and scale support [M]
- bytecode node language
- tux-racer ripoff demo game [L]
