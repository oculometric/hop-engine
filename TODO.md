# TODO

## v0.51
- better support for object transforms, including world-space transform operations and quaternion support [H]
- improve scene tree so that objects know about their scene, and children, and can be removed [M]
---
## v0.6
- separate engine from game resources [M]
- restructure project [H]
- build as shared library (so/dll) [H]
- shader & other resource reloading at runtime [H]
- audio loading and output
- binary repackaging (i.e. read in OBJs and turn them into binary meshes to reduce load times)
- custom shader format to keep stuff in one file [L]
- improved text block rendering with wrapping, alignment, font, etc
- deprecate getGizmoMaterial and similar family functions
---
- more scene control using ImGui (modify the render graph, material uniforms) [M]
- improved gizmo, better control, rotation and scale support [M]
- tux-racer ripoff demo game [L]
- frustrum culling [M]
- shadows
- bytecode node language
- shader node editor
- render graph node editor