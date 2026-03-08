# TODO

## v0.6
- simplify sampler (incorporate into texture)
- simplify texture to be in one of several 'modes/types' (fix the way view aspects behave), overhaul initialisation
- create image view on image create
- overhaul some interfaces (ref project placeholder) (uniform creation, pipeline creation, draw calls, uniform updating happens during command buffer build)
- refactor swapchain code
- get/set whole window size
- fix weird mouse delta behaviour in small window
- fix crashing with renderdoc
- fix multi-pass rendering not working
- make the behaviour of cameras and scene drawing more sensible
- change the way refs work, and the way `new`ing works
- get debug builds working again (release mode, but with optimisation disabled)
- separate vulkan code from generic code e.g. OBJ loading
- interactive node editor
- audio loading and output
- actual editor (separate project)
- shader & other resource reloading at runtime [H]
- frustrum culling [M]
- improved text block rendering with wrapping, alignment, font, etc
- object duplicate function [L]
---
## v0.7
- shader node editor
- render graph node editor
- shadows
- textures should be loadable in linear mode?
- a proper procedural shading art workflow
---
- more scene control using ImGui (modify the render graph, material uniforms) [M]
- improved gizmo, better control, rotation and scale support [M]
- bytecode node language
- tux-racer ripoff demo game [L]
