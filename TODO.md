# TODO

# v0.54
- input actions, and general input event cleanup (expand on pressed_since_checked etc)

- package manager improvements
    - data blocks should know which package they came from
    - package manager should not have to load the entire file from disk at once, only an index of contents
    - when you try to load an entry, search through the indexes of loaded packages, don't keep a copy of dat
    - you can ask for an entry to be preloaded, where a copy of it IS kept in memory, but only until a call to load it properly
    - needs ability to create directories (including automatically when writing a file)
    - needs to be able to search a directory for res files too (we dont need to rebuild the hop file ALL the time)
    - metadata (date/time, author)

- document serial formats
    - render graph
    - shader format
    - scene
    - font
    - node style

- material serial spec needs to support changing the render pass
- split framebuffer from render pass?? merge image creation from swapchain to render pass

# v0.55
- the ui update
    - document user_interface.h
    - ui canvas
        - canvas can be in 'world' mode, or 'view' mode [2]
            - view mode is added to a global ui stack which is applied at composite time
            - world mode can have world space transforms applied, exists as a component
    - basic elements
        - label
        - button
        - dropdown
        - checkbox
        - radiobutton
        - textbox
        - slider
        - panel
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
- audio loading and output
- windowless offscreen rendering support
- .otf -> baked font converter (font oven)
- hover tooltips for nodes....
- frustrum culling [M]
- texture upload/download of data
- shader node editor
- render graph node editor
- animation timeline using nodes
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
