# TODO

## v0.51
- eliminate vulkan types being exposed outside of class implementations entirely, replace stuff like VkFormat etc
- improve scene tree so that objects know about their scene, and children, and can be removed [M]
- custom shader format to keep stuff in one file [L]
- object duplicate function [L]
---
## v0.6
- multiple panels/scenes (i.e. nested shader graphs, decoupling shader graph from swapchain)
- shader & other resource reloading at runtime [H]
- frustrum culling [M]
- audio loading and output
- improved text block rendering with wrapping, alignment, font, etc
- re-evaluate whether everything needs FRAMES_IN_FLIGHT copies of stuff
---
## v0.7
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
