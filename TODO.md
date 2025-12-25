# TODO

## v0.5
- fix SSAO pass (currently doesn't look quite right and very inefficient, has to be rendered at quarter resolution) [H]
- ability to toggle skipping passes
- more scene control using ImGui (modify the render graph, material uniforms) [M]
---
## v0.6
- shader & other resource reloading at runtime [H]
- better support for object transforms, including world-space transform operations and quaternion support [H]
- deferred rendering demo scene, showing off the different light types, normal mapping, PBR materials [M]
- improved gizmo, better control, rotation and scale support [M]
- blur post process effect (demo in the deferred scene using stencil buffer??? michelangelo statue with blurred cock?) [L]
- origin variables for render graph, scene, better name generation for objects [L]
---
- separate engine from game resources [M]
- tux-racer ripoff demo game [L]
- custom shader format to keep stuff in one file [L]
