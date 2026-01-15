# TODO

## v0.5 (assignment)
- deferred rendering demo scene, showing off the different light types, normal mapping, PBR materials [M]
- proper text rendering demo [L]
- blur post process effect (demo in the deferred scene using stencil buffer??? michelangelo statue with blurred cock?) [L]
---
## v0.6
- more scene control using ImGui (modify the render graph, material uniforms) [M]
- shader & other resource reloading at runtime [H]
- better support for object transforms, including world-space transform operations and quaternion support [H]
- improved gizmo, better control, rotation and scale support [M]
- origin variables for render graph, scene, better name generation for objects [L]
---
- separate engine from game resources [M]
- tux-racer ripoff demo game [L]
- custom shader format to keep stuff in one file [L]
- ability to toggle skipping passes [L]
- occlusion culling [L]
- shadows
- backside lighting behaviour in pbr
- improve scene tree so that objects know about their scene, and children, and can be removed