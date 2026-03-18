# TODO

# v0.6
- refactor deserialise to provide more utils
    - refactor files
    - provide functions to get args
    - provide function to turn a KEYWORD into an int/bool/enum
    - provide a simplified interface to list possible statements, whether they can/must have identifiers, whether they can/must have named arguments, how many/which arguments, handle errors automatically, returns which args were present and their values (a statement-reader class)
- node style deserialisation
- actual editor (separate project)
    - resizeable/swappable views
    - editor build target
    - file/project manager
    
- document everything
- eliminate the tautological compare thing
- interactive node editor
- refactor lots of node view into a UI builder class
- windows 95 node style
- hover tooltips for nodes....
- audio loading and output
- shortcut function to make object/component creation easier (creates an object and gives it a component)
- shader & other resource reloading at runtime [H]
- frustrum culling [M]
- improved text block rendering with wrapping, alignment, font, etc
- object duplicate function [L]
- enable/disable components
- texture upload/download of data
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