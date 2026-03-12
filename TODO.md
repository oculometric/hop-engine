# TODO

call draw on scene with a viewport size, it should return a passthrough material
scene resizes render graph
scene collects draw commands and camera infos and lights and calls draw on render graph (doesn't need to know about scene)


## v0.52
- switch to entity/component system
    - scene drawing function
    - render graph refactoring
    - reimplement basic components
    - fix deserialise
    - fix demo scenes
    - get camera resolution FROM THE RENDER GRAPH REMEMBER
- overhaul engine debug interfaces
- uniforms should know what set they bind to
- multi-scene mode in the engine

- simplify texture to be in one of several 'modes/types' (fix the way view aspects behave), overhaul initialisation, overhaul renderpass accordingly
- refactor mesh loading as well
- create image view on image create

- scene window-to-viewport function for mouse position etc
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
- data blocks should know which package they came from
---
- more scene control using ImGui (modify the render graph, material uniforms) [M]
- improved gizmo, better control, rotation and scale support [M]
- bytecode node language
- tux-racer ripoff demo game [L]
- VR interfaces