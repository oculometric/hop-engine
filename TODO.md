# TODO

# v0.60
- windows 95 node style
- interactive node editor
    - links should be BACKWARDS not forwards, add makeLink function
    - box selection
    - right-click menu to allow adding more nodes
    - move overlays (temp link, right click menu, selection box) into their own mesh
- transparent window support
- switch to glslang for shader compilation
- ui input event system
    - mousedown/mouseup/mouseclick/mousedrag
    - keydown/keyup
    - multi-element intersection testing, i.e. we end up with a stack of elements which were intersected
- object duplicate function
- actual editor
    - resizeable/swappable views
    - editor build target
    - full-screen-node file/project manager
    - full-screen-node properties view
    - full-screen-node log view
    - transform gizmos
    - debug meshes for cameras, lights
- ui rendering refactor
    - general purpose text rendering (wrapping, alignment, font, underline, bold, italic, strikethrough)
    - general purpose 9-slice, image, and icon rendering kits
    - refactor node view code somewhat
    - ui interaction toolkit
    - ui elements - label, button, dropdown, checkbox, textbox, slider
    - right click menu builder
---

# v0.61
- document everything
- audio loading and output
- shader & other resource reloading at runtime
- windowless offscreen rendering support
---

## v0.7
- hover tooltips for nodes....
- frustrum culling [M]
- texture upload/download of data
- shader node editor
- render graph node editor
- animation timeline using nodes
- shadows
- textures should be loadable in linear mode?
- a proper procedural shading art workflow
- data blocks should know which package they came from
- bytecode node language
- VR interfaces
- Steamworks integration
- 3D editor