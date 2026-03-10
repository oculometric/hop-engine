# TODO

## v0.52
- overhaul some interfaces (ref project placeholder) (uniform creation into renderserver, pipeline creation into shader)
- refactor swapchain code
- make the behaviour of cameras and scene drawing more sensible (draw calls), render graph etc (weirdly jank and disorganised), reduce the aggressive draw call sorting, lots of 'viewport size' passthroughs (should just call scene-draw() which returns a texture)
- cameras should have a slot index within them, rather than the scene, deprecate setCameraSlot

- separate vulkan code from generic code e.g. OBJ loading
- multi-scene mode in the engine
- simplify texture to be in one of several 'modes/types' (fix the way view aspects behave), overhaul initialisation
- create image view on image create
- fix crashing with renderdoc
- fix multi-pass rendering not working
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
