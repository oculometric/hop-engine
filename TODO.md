# TODO

# v0.55
- package manager improvements [240]
    - data blocks should know which package they came from
    - package manager should not have to load the entire file from disk at once, only an index of contents
    - when you try to load an entry, search through the indexes of loaded packages, don't keep a copy of dat
    - you can ask for an entry to be preloaded, where a copy of it IS kept in memory, but only until a call to load it properly
    - needs to be able to search a directory for res files too (we dont need to rebuild the hop file ALL the time)
    - metadata (date/time, author)
    - async preloading with 'is_loading' flag to wait on
---

# v0.56
- input actions, and general input event cleanup (expand on pressed_since_checked etc)
- the ui update
    - figure out view space transform (need to be able to specify canvas size and position in pixels in 0-width space)
    - transform needs to be applied by the RENDERER

    - document user_interface.h
    - basic elements
        - button
        - dropdown
        - checkbox
        - radiobutton
        - textbox
        - slider
    - ui input event system [4]
        - mousedown/mouseup/mouseclick/mousedrag
        - mouseenter/mouseexit/mousemove
        - keydown/keyup
        - elements automatically get interactions passed to them from the event system (mouse position in local space)
        - multi-element intersection testing, i.e. we end up with a stack of elements which were intersected
        - fix mouse locking not behaving correctly on linux
    - refactor nodes to use the ui renderer
        - no longer their own scene
        - pack everything into a single texture
        - use the generic user interface shader

- make local/global transform from forward/up vectors (generally complete the transform functionality) [H]
---

# v0.60
- interactive node editor
    - need to be able to move/disconnect existing links
    - dirty flag system for updating the mesh
    - auto-size nodes horizontally
    - fix rendering behaviour when header is at bottom/node order is flipped
    - box selection
    - right-click menu to allow adding more nodes
    - move overlays (temp link, right click menu, selection box) into their own mesh
- object duplicate function
- actual editor
    - resizeable/swappable views
    - editor build target
    - full-screen-node file/project manager
    - full-screen-node properties view
    - full-screen-node log view
    - transform gizmos
    - debug meshes for cameras, lights
---

# future
- node editors
    - shader
    - texture
    - mesh
    - render graph
    - scene graph
    - animation timeline
    - audio synthesiser
- fix errors on frame timeout and query pool results not ready
- audio loading and output
- windowless offscreen rendering support
- .otf -> baked font converter (font oven)
- hover tooltips for nodes....
- frustrum culling [M]
- texture upload/download of data
- shadows
- textures should be loadable in linear mode?
- a proper procedural shading art workflow
- bytecode node language
- VR interfaces
- 3D editor
- geometry shader support
- skinned mesh support
- networking kit
- Steamworks integration
- animated images
- horizon-based ambient occlusion
- support multiple materials per mesh