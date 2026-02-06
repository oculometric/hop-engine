# hop-engine
hello! welcome to my advanced graphics project. i went a bit further with this than the module requires, mostly in terms of turning this into a proper graphics engine. how to use, features, and limitations are listed below.

## Navigation
basic controls are what you'd expect:
- **W/S** - move forward/backward
- **A/D** - move left/right
- **E/Q** - move up/down
- **Right Mouse + Drag** - look around
- **Left Mouse** - select object under cursor (can be unintuitive)

in addition, the demo contains a bunch of debug stuff to play around with. there should be a bunch of ImGui windows on the screen; below is a summary of what each one does:
- resources window
  - showcases various resources currently loaded and managed by the engine
  - these are not automatically destroyed when no longer used
  - any which are currently unused can be purged using the `prune loaded resources` button
  - expand the different sections to see all currently loaded resources
  - use the `load resource` button to load an additional resource from the package file (you can then apply this resource in the other editors, such as switching which texture a material is using)
- performance window
  - shows the smoothed FPS and unsmoothed delta time
  - shows details for how long each part of the frame takes (building ImGui, updating uniform buffers, recording the command buffer, rendering the frame on the GPU, and running the scene update)
  - shows statistics about the frame including the number of draw call, number of times the Vulkan pipeline had to be rebound, number of triangles and vertices submitted, and the number of render passes executed
  - shows details for how long each render pass took on the GPU (for instance you can see that the SSAO pass takes the longest!)
  - number of cameras and lights participating
  - delta time histogram
- scene window
  - allows editing of basic scene properties (ambient light colour, skybox texture)
  - if no skybox texture is set (i.e. 0x0 is used) then the camera's clear colour will be shown
  - exploration and modification of the scene transform heirarchy
    - each object can be expanded to show its children
    - objects can be selected, which causes the object window to appear (see below)
    - when an object is selected, it can be removed from the tree, moved under a new parent object (preserving world transform), or have new child objects added
  - shows details of the render graph
    - allows you to select which render step is being shown on the screen (-1 automatically uses the last in the list)
    - allows you to select which attachment from that render step is shown on the screen (i.e. which G-buffer. some passes may have a different number of G-buffers in use!)
    - allows you to see details of all the render steps involved in the scene
    - allows you to enable/disable each render step. an attachment will be selected from a previous pass automatically if future passes are missing inputs as a result!
    - see the features section for more info!
- object window
  - only visible if an object is selected (either by clicking or via the scene window)
  - you can rename the object
  - you can alter the object's transform (all transforms are shown in local space)
  - depending on the object type, you can alter that object's parameters
    - cameras can have their clipping planes, FOV and clear colour modified, and you can also take first-person control of that camera (though it may not render if it isn't bound to a render step)
    - static meshes can have their mesh and material data modified; the material itself can be modified in detail by clicking `edit material` to open the material window
    - text blocks can have their text content and colour modified
- material window
  - only visible once you select a material by clicking the `edit material` button on a static mesh
  - shows pipeline parameters (culling, polygon mode, depth test/write/operation values)
  - shows information about which textures are bound to which descriptors, and what their filtering and addressing modes are
  - the texture in use, filtering mode, and addressing modes can all be modified (although filtering/addressing modes may use shared sampler objects, so beware when modifying this)
  - the uniform blocks specified by the shader, including each variable within it (this is only uniforms defined in descriptor set 2; set 0 and set 1 are used by the engine to provide scene-global and per-object uniform buffers respectively)
- colour correction window
  - allows you to modify the uniform parameters of one of the render steps, specifically the colour correction step
  - you can play with gamma, exposure, and offset (switching the LUT is not supported)
- spline control window
  - the first checkbox causes the camera to always point toward the planet in the back of the main room, which moves continuously around a looped Catmull-Rom spline
  - the second checkbox causes the camera to begin a 'guided tour' of the scene, where the camera will always face the point on the curve slightly ahead of its current position (overrides the first checkbox)
  - unchecking and rechecking the second checkbox restarts the tour
- menu bar
  - the file menu gives you the option to quit, or switch to the other scene ('bunnygirl')
    - this scene showcases stylised shading applied to several previous art projects of mine
    - this scene also demonstrates multi-camera rendering
  - view menu
    - allows you to toggle all engine ImGui windows (does not affect colour correction or spline control)
    - allows you to auto-arrange engine ImGui windows
    - allows you to enable wireframe mode for the entire scene, useful for debug

in addition to all that, the outputs of various render steps are overlaid at the top of the screen (left -> right):
1. scene albedo colour
2. scene normal
3. scene parameters (roughness, metallicness, emission)
4. scene specular colour
5. sccene depth
6. deferred shading applied
7. SSAO raw output (before blur)

if you want to see any of these bigger, you can use the render graph panel in the scene window to walk through every output of every render step!

## Features
- render graph
- scene heirarchy & nested transform tree
- SSAO
- colour grading and LUT post processing
- resource packaging
- deferred rendering
- PBR shading
- normal mapping
- blur effects
- detailed ImGui debug controls
- museum and bunnygirl demo scenes
- text rendering
- keyboard and controller input
- object picking via ray-OBB intersection testing
- scene, material, and render graph deserialisation from text representations
- automatic resource reference counting with custom reference counted type (weak and strong)
- easy material modification with named texture and uniform variables via shader reflection
- detailed console debug output with variable degrees of verbosity, including use of the Vulkan validation layers (if enabled)
- render-to-texture can be bound and shown on an object in the scene
- on-GPU frame duration information via query pool

// TODO: explain features and where they're implemented, and where they can be seen

## Project Structure
this turned into a really big project so the structure is a little complicated. the main places of note are `src`, `inc`, and `res`/`res_engine`. part of this complexity is tied with having tested the engine to build as a static library (for the purpose of the assignment this is irrelevant, and the Release/x64 configuration should be used).

- inc - contains nearly all header files
  - counted_ref.h - contains reference counting classes
  - window.h - encapsulates desktop window management
  - graphics_environment.h - singleton which manages configuring Vulkan and other similar tasks
  - vulkan_typedefs.h - typedefs and similar to avoid #including Vulkan in headers
  - swapchain.h - encapsulates swapchain creation and resizing
  - command_buffer.h - object for creating and submitting transient command buffers
  - buffer.h - encapsulates GPU buffer object creation and mapping
  - render_pass.h - encapsulates creation and resizing of render passes and their configurations
  - pipeline.h - encapsulates pipeline creation
  - shader.h - encapsulates shader compilation, linking, and descriptor set layout extraction via reflection
  - mesh.h - encapsulates loading and management of a mesh
  - texture.h - encapsulates loading and management of a texture/image
  - sampler.h - encapsulates sampler creation
  - uniform_block.h - encapsulates storage and management of uniform buffers
  - material.h - encapsulates creation of pipelines for shaders, and management of uniforms/texture bindings
  - render_graph.h - contains render graph system for multi-step rendering and post-processing
  - font.h - encapsulates font information (glyph atlas texture and glyph size information)
  - pbr.h - contains reflections of shader uniforms
  - scene.h - manages a tree of objects other information needed to complete rendering (render graph, lights, cameras, skybox, etc)
  - draw_command.h - represents a request for a specified mesh to be draw, using a specified material and specified object uniforms (gives flexibility for objects to submit multiple draw calls)
  - transform.h - describes the transformation of objects as a nested hierarchy, providing transform operations in local and world space
  - math_helpers.h - contains the ray-OBB intersection checker
  - object.h - describes a generic scene object with no special attributes, capable of having a parent (also includes key object types - camera, static mesh, light)
  - gizmo.h - subtype of scene object which can receive input to modify other objects (WIP and not relevant)
  - node_view.h - subtype of scene object which renders arbitrary nodes onto a 2D canvas (WIP and not relevant)
  - text_block.h - subtype of scene object which draws customisable text in world space using a bitmap font
  - debug.h - contains macros + singleton to handle logging to the console
  - engine.h - singleton which manages overall engine state and initialisation of subcomponents
  - package.h - singleton which manages loading/storing of packages and data from files within packages
  - input.h - singleton which handles receiving and polling input from keyboard, mouse, and gamepads
  - token_file.h - provides utility functions + classes for reading arbitrary syntax tree files (such as scene, material, and render graph files)
  - hop_engine.h - unifies all other headers into a single include
  - hop_forward.h - contains forward definitions for most types
  - common.h - contains some commonly used code which all (or nearly all) other files use
- src - contains implementations plus some header files which are not intended to be externally used
  - package-builder
    - package-builder.cpp - for the package builder subproject, results in a simple executable which takes command line arguments to turn a folder of files into a single '.hop' package file (which can be read back in by the engine)
  - window.cpp - partner to window.h
  - graphics_environment.cpp - partner to graphics_environment.h
  - swapchain.cpp - partner to swapchain.h
  - swapchain_vulkan.h - contains additional Vulkan-specific definitions
  - command_buffer.cpp - partner to command_buffer.h
  - buffer.cpp - partner to buffer.h
  - render_pass.cpp - partner to render_pass.h
  - pipeline.cpp - partner to pipeline.h
  - shader.cpp - partner to shader.h
  - mesh.cpp - partner to mesh.h
  - texture.cpp - partner to texture.h
  - sampler.cpp - partner to sampler.h
  - uniform_block.cpp - partner to uniform_block.h
  - material.cpp - partner to material.h
  - render_graph.cpp - partner to render_graph.h
  - font.cpp - partner to font.h
  - scene.cpp - partner to scene.h
  - transform.cpp - partner to transform.h
  - math_helpers.cpp - partner to math_helpers.h
  - object.cpp - partner to object.h
  - gizmo.cpp - partner to gizmo.h
  - node_view.cpp - partner to node_view.h
  - text_block.cpp - partner to text_block.h
  - debug.cpp - partner to debug.h
  - engine.cpp - partner to engine.h
  - imgui_functions.cpp - contains functions from various types (engine, mesh, material, etc) which draw ImGui debug windows describing particular those types
  - package.cpp - partner to package.h
  - input.cpp - partner to input.h
  - token_file.cpp - partner to token_file.h
  - deserialise.cpp - contains functions from various types (scene, object, render graph, material) which can deserialise those types from text representations
  - exec.cpp - contains a function for executing a command and capturing the output
  - asha_scene.cpp - contains functions which represent the startup, update, and draw-debug functions for the bunnygirl scene
  - museum_scene.cpp - contains functions which represent the startup, update, and draw-debug functions for the museum scene
  - node_scene.cpp - contains functions which represent the startup, update, and draw-debug functions for the node scene (WIP and not relevant)
  - main.cpp - contains the main function
  - main.h - contains definitions for the three demo scenes (only bunnygirl and museum are playable)
- res - contains resources for the demo scenes
  - half_and_half.frag/.vert - post-process shader which overlays other camera passes on top of the main pass (used in the bunnygirl scene)
  - icon.ico/.png - used for the app icon
  - test_graph.hrgr - render graph used in the bunnygirl scene
  - tux.obj - remodel of tux, from tuxracer of course!
  - tux.png - texture for tux
  - museum - contains all resources specific to the museum demo scene
    - Museum.hscn - scene description for the museum
    - RenderGraph.hrgr - render graph description for the museum. very complicated!
    - *.hmat - various material descriptions of materials used in the scene (bind textures and assign uniform variables)
    - *_t.png - albedo textures for various materials
    - *_n.png - normal maps for various materials
    - *.obj - meshes for various objects in the scene
    - blend.frag/.vert - post-processing shader which blends SSAO over-top of the scene
    - multi_panel.frag/.vert - post-processing shader which overlays various different render attachments on top of the main scene output for debug
- res_engine - contains resources which are needed by the engine, or are provided as defaults/samples
  - meshes
    - axes_gizmo.obj - mesh for the translation gizmo
    - rotate_gizmo.obj - mesh for the rotation gizmo
    - skybox.obj - cube with flipped normals used for skybox rendering
  - shaders
    - common.glsl - contains various standard functions, defines, and layout directives to standardise/simplify other shaders
    - dither.glsl - provides some dithering masks/functions
    - effects.glsl - provides various functions which perform effects (sampling a convolution kernel, computing SSAO, etc)
    - pbr_util.glsl - provides PBR suppport
    - default_shader.frag/.vert - pink error shader for when a shader cannot be loaded
    - deferred.frag/.vert - scene object shader which outputs information to various attachments to be later used by the deferred rendering pass
    - deferred_post.frag/.vert - PBR deferred rendering post-processing pass, expects inputs to be in the form of outputs from deferred.frag
    - forward.frag/.vert - PBR forward rendering scene object shader (uses same shading math as deferred_post.frag)
    - passthrough.frag/.vert - post-process effect which does nothing but blit to the screen (used internally by the engine)
    - skybox.frag/.vert - performs skybox shading
    - ssao.frag/.vert - post-process effect which computes screen-space ambient occlusion
    - fog.frag/.vert - post-process effect which applies depth fog
    - blur.frag/.vert - post-process effect which blurs the input texture
    - colour_correct.frag/.vert - post-process effect which applies LUT and basic colour correction
    - text.frag/.vert - shader used by the text block object material
    - gizmo.frag/.vert - shader used by the gizmo object material
  - textures
    - basic_lut.png - LUT texture with no colour alterations
  - samples
    - asha.obj - a character 3D model (a fanart made by me), the eponymous bunnygirl
    - asha.png - texture for asha
    - bunny.obj - a demo 3D model of a bunny (again by me), her name is Julia
    - bunny.png - texture for bunny
    - plane.obj - quad demo object
    - cube.obj - cube demo object
    - ico_sphere.obj - icosahedron demo object
    - uv_sphere.obj - UV sphere demo object
    - cylinder.obj - cylinder demo object
    - cone.obj - cone demo object
    - donut.obj - torus demo object
    - torus.obj - another torus demo object?
    - nasa_goddard_gaia_dr2_deep_star_map.png - NASA SVS Deep Star Maps 2020 skybox texture
    - psx.frag/.vert - stylised simple shader with a vertex snapping/warping effect (used by asha and the bunny)
  - font.bmp - basic bitmap font
  - icon.ico/.png - another copy of the app icon
  - newnodes.png - contains elements used for drawing the nodes in the node view
  - nodelinks.png - contains more elements used for drawing the nodes in the node view
  - node_shader.frag/.vert - shader which draws the nodes in the node view

## Limitations
- does not support deferred lighting (i.e. separating deferred rendering), although this would be simple to implement via the render graph system
- SSAO could definitely be better, including being replaced by HBAO
- more control of the scene via ImGui would be nice (changing the culling behaviour of materials, modifying uniform variables)
- a demonstration of tangent-space lighting would have been interesting (i tested this but settled on world-space since there seemed to be no performance benefits in my case, versus several maintainability concerns)

## Credits
the beautiful skybox is a slightly modified (adding the checkerboard floor) version of the NASA SVS Deep Star Maps 2020. credit for this:
> NASA/Goddard Space Flight Center Scientific Visualization Studio. Gaia DR2: ESA/Gaia/DPAC. Constellation figures based on those developed for the IAU by Alan MacRobert of Sky and Telescope magazine (Roger Sinnott and Rick Fienberg).

libraries used:
- Vulkan
- GLFW
- glm
- ImGui
- SPIRV-reflect
- STB Image
- base64

all other assets (models, materials, textures, shaders, font, and demo scenes) are made by me. and the code obviously.

**particular thanks to LearnOpenGL, the Vulkan Tutorial, and David White for the guidance to develop this.**

_additional thanks to Lucy and Louise for keeping me alive and sane. <3_
