# TODO

## v0.51
- actually add comments to everything
- improve scene tree so that objects know about their scene, and children, and can be removed [M]
---
## v0.52
- object duplicate function [L]
- separate engine from game resources [H]
- eliminate glslc.exe dependency [H]
- binary repackaging (i.e. read in OBJs and turn them into binary meshes to reduce load times)
- implement imgui disable-enable
- deprecate getGizmoMaterial and similar family functions
---
## v0.6
- shader & other resource reloading at runtime [H]
- eliminate vulkan types being exposed outside of class implementations entirely, replace stuff like VkFormat etc
- frustrum culling [M]
- audio loading and output
- custom shader format to keep stuff in one file [L]
- improved text block rendering with wrapping, alignment, font, etc
---
- more scene control using ImGui (modify the render graph, material uniforms) [M]
- improved gizmo, better control, rotation and scale support [M]
- tux-racer ripoff demo game [L]
- shadows
- bytecode node language
- shader node editor
- render graph node editor