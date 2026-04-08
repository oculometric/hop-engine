# TODO

# v0.54
- split framebuffer from render pass?? merge image creation from swapchain to render pass
- enforce proper error checking
- remove type parameter from buffer bind function
- document everything
    - basic_components.h
    - engine.h
    - material.h
    - mesh.h
    - render_graph.h
    - render_server.h
    - scene.h
    - texture.h
    - user_interface.h
    - eliminate vulkan_typedefs.h
    - license (happy bunny license) and copyright
- make specifically init/destroy functions private but exposed to the engine (clean up and make more consistent)
- input actions, and general input event cleanup (expand on pressed_since_checked etc)
- switch to glslang for shader compilation
- package manager improvements
    - data blocks should know which package they came from
    - package manager should not have to load the entire file from disk at once
    - package manager should reduce copying of data blocks
    - needs ability to create directories (including automatically when writing a file)

- the ui update
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
- fix mouse locking not behaving correctly on linux
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

# v0.61
- audio loading and output
- shader & other resource reloading at runtime
- windowless offscreen rendering support
- .otf -> baked font converter (font oven)
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
- bytecode node language
- VR interfaces
- Steamworks integration
- 3D editor