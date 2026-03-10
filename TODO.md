# TODO

## v0.52
- refactor init/destroy/construct/destruct
- make frame index accessible
- get debug builds working again (release mode, but with optimisation disabled)
- get build time down!
---
## v0.53
- separate vulkan code from generic code e.g. OBJ loading
- multi-scene mode in the engine
- refactor the way engine loop/setup works
- don't make post-process shaders go into wireframe mode
- refactor swapchain code
- overhaul some interfaces (ref project placeholder) (uniform creation into renderserver, pipeline creation into shader, draw calls, uniform updating happens during command buffer build)
- simplify texture to be in one of several 'modes/types' (fix the way view aspects behave), overhaul initialisation
- simplify sampler
---
## v0.54
- create image view on image create
- fix crashing with renderdoc
- fix multi-pass rendering not working
- make the behaviour of cameras and scene drawing more sensible (draw calls)
- change the way refs work, and the way `new`ing works (classes only construct via ::create, remove keepLoaded)
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
