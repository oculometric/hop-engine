# TODO

## v0.51
- make builders return references
- actually add comments to everything
- fix crash with empty object file [H]
- better support for object transforms, including world-space transform operations and quaternion support [H]
- improve scene tree so that objects know about their scene, and children, and can be removed [M]
---
## v0.6
- separate engine from game resources [H]
- binary repackaging (i.e. read in OBJs and turn them into binary meshes to reduce load times)
- implement imgui disable-enable
- shader & other resource reloading at runtime [H]
- audio loading and output
- custom shader format to keep stuff in one file [L]
- improved text block rendering with wrapping, alignment, font, etc
- deprecate getGizmoMaterial and similar family functions
---
- eliminate vulkan types being exposed outside of class implementations entirely, replace stuff like VkFormat etc
- more scene control using ImGui (modify the render graph, material uniforms) [M]
- improved gizmo, better control, rotation and scale support [M]
- tux-racer ripoff demo game [L]
- frustrum culling [M]
- shadows
- bytecode node language
- shader node editor
- render graph node editor