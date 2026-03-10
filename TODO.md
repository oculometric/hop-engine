# TODO

## v0.52
- overhaul some interfaces (ref project placeholder) (uniform creation into renderserver, pipeline creation into shader, draw calls, uniform updating happens during command buffer build)
- separate vulkan code from generic code e.g. OBJ loading
- multi-scene mode in the engine
- refactor swapchain code
- simplify texture to be in one of several 'modes/types' (fix the way view aspects behave), overhaul initialisation
- simplify sampler (remove rebuilding)
- create image view on image create
- fix crashing with renderdoc
- fix multi-pass rendering not working
- make the behaviour of cameras and scene drawing more sensible (draw calls)
- change the way refs work, and the way `new`ing works (classes only construct via ::create, remove keepLoaded)
- Application class which contains game update loop, etc, which users override
---
# v0.6
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
