# TODO

call draw on scene with a viewport size, it should return a passthrough material
scene resizes render graph
scene collects draw commands and camera infos and lights and calls draw on render graph (doesn't need to know about scene)


## v0.52
- make most classes `final`
- overhaul engine debug interfaces
- shortcut function to make object/component creation easier (creates an object and gives it a component)

- fullscreen support
- fix skybox side being flipped
- make fullscreen quad into a fullscreen tri
- scene window-to-viewport function for mouse position etc
- change the way refs work, and the way `new`ing works (classes only construct via ::create, remove keepLoaded)

- simplify texture to be in one of several 'modes/types' (fix the way view aspects behave), overhaul initialisation, overhaul renderpass accordingly
- refactor mesh loading as well
- create image view on image create

- fix multi-pass rendering not working
- multi-scene mode in the engine
---
# v0.6
- interactive node editor
- audio loading and output
- actual editor (separate project)
- shader & other resource reloading at runtime [H]
- frustrum culling [M]
- improved text block rendering with wrapping, alignment, font, etc
- object duplicate function [L]
- enable/disable components
---
## v0.7
- shader node editor
- render graph node editor
- shadows
- textures should be loadable in linear mode?
- a proper procedural shading art workflow
- data blocks should know which package they came from
---
- improved gizmo, better control, rotation and scale support [M]
- bytecode node language
- VR interfaces
- Steamworks integration
- 3D editor