# hop-engine
hello! welcome to my advanced graphics project. i went a bit further with this than the module requires, mostly in terms of turning this into a proper graphics engine. how to use, features, project structure, limitations, and credits are listed below.

## Navigation
basic controls are what you'd expect:
- **W/S** - move forward/backward
- **A/D** - move left/right
- **E/Q** - move up/down
- **Right Mouse + Drag** - look around
- **Left Mouse** - select object under cursor (can be unintuitive)

in addition, the demo contains a bunch of debug stuff to play around with. there should be a bunch of ImGui windows on the screen; below is a summary of what each one does:
- **resources window**
  - showcases various resources currently loaded and managed by the engine
  - these are not automatically destroyed when no longer used
  - any which are currently unused can be purged using the `prune loaded resources` button
  - expand the different sections to see all currently loaded resources
  - use the `load resource` button to load an additional resource from the package file (you can then apply this resource in the other editors, such as switching which texture a material is using)
- **performance window**
  - shows the smoothed FPS and unsmoothed delta time
  - shows details for how long each part of the frame takes (building ImGui, updating uniform buffers, recording the command buffer, rendering the frame on the GPU, and running the scene update)
  - shows statistics about the frame including the number of draw call, number of times the Vulkan pipeline had to be rebound, number of triangles and vertices submitted, and the number of render passes executed
  - shows details for how long each render pass took on the GPU (for instance you can see that the SSAO pass takes the longest!)
  - number of cameras and lights participating
  - delta time histogram
- **scene window**
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
- **object window**
  - only visible if an object is selected (either by clicking or via the scene window)
  - you can rename the object
  - you can alter the object's transform (all transforms are shown in local space)
  - depending on the object type, you can alter that object's parameters
    - cameras can have their clipping planes, FOV and clear colour modified, and you can also take first-person control of that camera (though it may not render if it isn't bound to a render step)
    - static meshes can have their mesh and material data modified; the material itself can be modified in detail by clicking `edit material` to open the material window
    - text blocks can have their text content and colour modified
- **material window**
  - only visible once you select a material by clicking the `edit material` button on a static mesh
  - shows pipeline parameters (culling, polygon mode, depth test/write/operation values)
  - shows information about which textures are bound to which descriptors, and what their filtering and addressing modes are
  - the texture in use, filtering mode, and addressing modes can all be modified (although filtering/addressing modes may use shared sampler objects, so beware when modifying this)
  - the uniform blocks specified by the shader, including each variable within it (this is only uniforms defined in descriptor set 2; set 0 and set 1 are used by the engine to provide scene-global and per-object uniform buffers respectively)
- **colour correction window**
  - allows you to modify the uniform parameters of one of the render steps, specifically the colour correction step
  - you can play with gamma, exposure, and offset (switching the LUT is not supported)
- **spline control window**
  - the first checkbox causes the camera to always point toward the planet in the back of the main room, which moves continuously around a looped Catmull-Rom spline
  - the second checkbox causes the camera to begin a 'guided tour' of the scene, where the camera will always face the point on the curve slightly ahead of its current position (overrides the first checkbox)
  - unchecking and rechecking the second checkbox restarts the tour
- **menu bar**
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
- render graph - _a system for specifying a series of camera (scene rendering) or post-process (fullscreen-quad rendering) steps where the output attachments of previous steps can be chained as inputs to future steps (e.g. feeding the normal and depth buffers from a camera step into an SSAO post-process step). allows for complex multi-step post-processing, modifiable and debuggable at runtime!_ (see [inc/render_graph.h]())
- normal mapping - _normal maps are sampled and the results are transformed to world-space for lighting calculations_ (see [res_engine/shaders/pbr_util.glsl]())
- deferred rendering - _a two-pass deferred rendering approach is supported, taking 5 attachments as inputs (albedo, normal, roughness/metallicness/emission, specular, depth) and sampling multiple lights using PBR shading. this is used for all objects in the museum scene_ (see [res_engine/shaders/deferred.frag]() and [res_engine/shaders/deferred_post.frag]())
- PBR shading - _simple PBR shading is implemented using roughness and metallic parameters, in addition to surface albedo, normal, and specular colours. this shading is available both in deferred and forward rendering modes_ (see [res_engine/shaders/pbr_util.glsl]())
- SSAO - _screen space ambient occlusion is a great use of multi-step post-processing, and makes scenes feel much more realistic to look at_ (see [res_engine/shaders/effects.glsl]() and [res_engine/shaders/ssao.frag]())
- blur effects - _functionality for sampling generic convolution kernels is provided, as well as example implementations for 5x5 and 9x9 Gaussian blur kernels. this is used to smooth the SSAO result_ (see [res_engine/shaders/effects.glsl]() and [res_engine/shaders/blur.frag]())
- colour grading and LUT post processing - _a colour grading and LUT post-processing effect is included to demonstrate colour-based post-processing. makes the museum look lovely and sepia-toned. or you can ruin it by playing with the sliders in ImGui i guess..._ (see [res_engine/shaders/colour_correct.frag]())
- text rendering - _a bitmap text rendering object allows runtime-editable text rendering, providing in-game instructions for the museum demo!_ (see [res_engine/shaders/text.frag]() and [inc/text_block.h]())
- on-GPU frame duration information via query pool - _using the Vulkan `QueryPool` primitive, detailed statistics about how long the frame took to render on the GPU are recorded, including breakdowns of time spent in each render pass_ (see [src/graphics_environment.cpp]())
- render-to-texture can be bound and shown on an object in the scene
- museum and bunnygirl demo scenes - _two demo scenes are provided, the first being a much more comprehensive demonstration of graphics techniques. the other is still here for fun though!_ (see [res/museum](), [src/museum_scene.cpp](), and [src/asha_scene.cpp]())


- scene heirarchy & nested transform tree - _allows for all the transform behaviours you'd expect. see the orrery in the center of the museum scene for a great demo!_ (see [inc/scene.h]() and [inc/transform.h]())
- resource packaging - _instead of having a hundred individual files hanging around, the engine supports loading (and storing) '.hop' package files, from which resources can be loaded by referencing their file path prefixed by `res://`. supports ZIP compression, resource aliasing, and overwriting on load, allowing for easy modding of existing executables by simply loading an extra resource package!_ (see [src/package.cpp](), [src/package-builder/package-builder.cpp]() [res](), and [res_engine]())
- scene, material, and render graph deserialisation from text representations - _a minimal library for decoding abstract syntax trees and extracting statements is provided, which forms the basis for various deserialisers (more to come in the future)_ (see [src/deserialise.cpp]())
- automatic reference counting - _custom smart-pointer-like reference counted types are provided which are intended to wrap nearly all engine types. comes in strong and weak varieties, with constructors and assignment operators for minimal annoyance during usage!_ (see [inc/counted_ref.h]())
- keyboard and controller input - _a simple framework for grabbing keyboard and mouse input (both in terms of current state or 'pressed-since-last-checked'), as well as gamepad input. provides functions for treating pairs of keyboard keys as virtual gamepad axes. used for all input functionality by the demo scenes_ (see [inc/input.h]())
- detailed ImGui debug controls - _extensive ImGui interactivity allows you to see the states of most engine types, as well as modify most of their parameters. missing some support currently_ (see [src/imgui_functions.cpp]())
- object picking via ray-OBB intersection testing - _exactly what it says. would benefit from being able to see the bounding box of the object you have selected!_ (see [inc/math_helpers.h]())
- material abstraction via shader reflection - _uniform variables and textures can be easily assigned by name, updating of descriptor sets is managed automatically, and uniform type size checking is performed automatically_ (see [inc/material.h]())
- detailed console debug output - _variable degrees of verbosity; passthrough of the Vulkan validation layers (if enabled); outputs both to the console (with colours!!) and a log file. just makes life easier_ (see [inc/debug.h]())

## Project Structure
this turned into a really big project so the structure is a little complicated. the main places of note are `src`, `inc`, and `res`/`res_engine`. part of this complexity is tied with having tested the engine to build as a static library (for the purpose of the assignment this is irrelevant, and the Release/x64 configuration should be used).

- inc - _contains nearly all header files_
  - counted_ref.h - _contains reference counting classes_
  - window.h - _encapsulates desktop window management_
  - graphics_environment.h - _singleton which manages configuring Vulkan and other similar tasks_
  - vulkan_typedefs.h - _typedefs and similar to avoid `#include`-ing Vulkan in headers_
  - swapchain.h - _encapsulates swapchain creation and resizing_
  - command_buffer.h - _object for creating and submitting transient command buffers_
  - buffer.h - _encapsulates GPU buffer object creation and mapping_
  - render_pass.h - _encapsulates creation and resizing of render passes and their configurations_
  - pipeline.h - _encapsulates pipeline creation_
  - shader.h - _encapsulates shader compilation, linking, and descriptor set layout extraction via reflection_
  - mesh.h - _encapsulates loading and management of a mesh_
  - texture.h - _encapsulates loading and management of a texture/image_
  - sampler.h - _encapsulates sampler creation_
  - uniform_block.h - _encapsulates storage and management of uniform buffers_
  - material.h - _encapsulates creation of pipelines for shaders, and management of uniforms/texture bindings_
  - render_graph.h - _contains render graph system for multi-step rendering and post-processing_
  - font.h - _encapsulates font information (glyph atlas texture and glyph size information)_
  - pbr.h - _contains reflections of shader uniform structs_
  - scene.h - _manages a tree of objects other information needed to complete rendering (render graph, lights, cameras, skybox, etc)_
  - draw_command.h - _represents a request for a specified mesh to be draw, using a specified material and specified object uniforms (gives flexibility for objects to submit multiple draw calls)_
  - transform.h - _describes the transformation of objects as a nested hierarchy, providing transform operations in local and world space_
  - math_helpers.h - _contains the ray-OBB intersection checker_
  - object.h - _describes a generic scene object with no special attributes, capable of having a parent (also includes key object types - camera, static mesh, light)_
  - gizmo.h - _subtype of scene object which can receive input to modify other objects (WIP and not relevant)_
  - node_view.h - _subtype of scene object which renders arbitrary nodes onto a 2D canvas (WIP and not relevant)_
  - text_block.h - _subtype of scene object which draws customisable text in world space using a bitmap font_
  - debug.h - _contains macros + singleton to handle logging to the console_
  - engine.h - _singleton which manages overall engine state and initialisation of subcomponents_
  - package.h - _singleton which manages loading/storing of packages and data from files within packages_
  - input.h - _singleton which handles receiving and polling input from keyboard, mouse, and gamepads_
  - token_file.h - _provides utility functions + classes for reading arbitrary syntax tree files (such as scene, material, and render graph files)_
  - hop_engine.h - _unifies all other headers into a single include_
  - hop_forward.h - _contains forward definitions for most types_
  - common.h - _contains some commonly used code which all (or nearly all) other files use_
- src - _contains implementations, plus some header files which are not intended to be externally used_
  - package-builder
    - package-builder.cpp - _for the package builder subproject, results in a simple executable which takes command line arguments to turn a folder of files into a single '.hop' package file (which can be read back in by the engine)_
  - window.cpp - _partner to window.h_
  - graphics_environment.cpp - _partner to graphics_environment.h_
  - swapchain.cpp - _partner to swapchain.h_
  - swapchain_vulkan.h - _contains additional Vulkan-specific definitions_
  - command_buffer.cpp - _partner to command_buffer.h_
  - buffer.cpp - _partner to buffer.h_
  - render_pass.cpp - _partner to render_pass.h_
  - pipeline.cpp - _partner to pipeline.h_
  - shader.cpp - _partner to shader.h_
  - mesh.cpp - _partner to mesh.h_
  - texture.cpp - _partner to texture.h_
  - sampler.cpp - _partner to sampler.h_
  - uniform_block.cpp - _partner to uniform_block.h_
  - material.cpp - _partner to material.h_
  - render_graph.cpp - _partner to render_graph.h_
  - font.cpp - _partner to font.h_
  - scene.cpp - _partner to scene.h_
  - transform.cpp - _partner to transform.h_
  - math_helpers.cpp - _partner to math_helpers.h_
  - object.cpp - _partner to object.h_
  - gizmo.cpp - _partner to gizmo.h_
  - node_view.cpp - _partner to node_view.h_
  - text_block.cpp - _partner to text_block.h_
  - debug.cpp - _partner to debug.h_
  - engine.cpp - _partner to engine.h_
  - imgui_functions.cpp - _contains functions from various types (engine, mesh, material, etc) which draw ImGui debug windows describing particular those types_
  - package.cpp - _partner to package.h_
  - input.cpp - _partner to input.h_
  - token_file.cpp - _partner to token_file.h_
  - deserialise.cpp - _contains functions from various types (scene, object, render graph, material) which can deserialise those types from text representations_
  - exec.cpp - _contains a function for executing a command and capturing the output_
  - asha_scene.cpp - _contains functions which represent the startup, update, and draw-debug functions for the bunnygirl scene_
  - museum_scene.cpp - _contains functions which represent the startup, update, and draw-debug functions for the museum scene_
  - node_scene.cpp - _contains functions which represent the startup, update, and draw-debug functions for the node scene (WIP and not relevant)_
  - main.cpp - _contains the main function_
  - main.h - _contains definitions for the three demo scenes (only bunnygirl and museum are playable)_
- res - _contains resources for the demo scenes_
  - half_and_half.frag/.vert - _post-process shader which overlays other camera passes on top of the main pass (used in the bunnygirl scene)_
  - icon.ico/.png - _used for the app icon_
  - test_graph.hrgr - _render graph used in the bunnygirl scene_
  - tux.obj - _remodel of tux, from tuxracer of course!_
  - tux.png - _texture for tux_
  - museum - _contains all resources specific to the museum demo scene_
    - Museum.hscn - _scene description for the museum_
    - RenderGraph.hrgr - _render graph description for the museum. very complicated!_
    - *.hmat - _various material descriptions of materials used in the scene (bind textures and assign uniform variables)_
    - *_t.png - _albedo textures for various materials_
    - *_n.png - _normal maps for various materials_
    - *.obj - _meshes for various objects in the scene_
    - blend.frag/.vert - _post-processing shader which blends SSAO over-top of the scene_
    - multi_panel.frag/.vert - _post-processing shader which overlays various different render attachments on top of the main scene output for debug_
- res_engine - _contains resources which are needed by the engine, or are provided as defaults/samples_
  - meshes
    - axes_gizmo.obj - _mesh for the translation gizmo_
    - rotate_gizmo.obj - _mesh for the rotation gizmo_
    - skybox.obj - _cube with flipped normals used for skybox rendering_
  - shaders
    - common.glsl - _contains various standard functions, defines, and layout directives to standardise/simplify other shaders_
    - dither.glsl - _provides some dithering masks/functions_
    - effects.glsl - _provides various functions which perform effects (sampling a convolution kernel, computing SSAO, etc)_
    - pbr_util.glsl - _provides PBR suppport_
    - default_shader.frag/.vert - _pink error shader for when a shader cannot be loaded_
    - deferred.frag/.vert - _scene object shader which outputs information to various attachments to be later used by the deferred rendering pass_
    - deferred_post.frag/.vert - _PBR deferred rendering post-processing pass, expects inputs to be in the form of outputs from deferred.frag_
    - forward.frag/.vert - _PBR forward rendering scene object shader (uses same shading math as deferred_post.frag)_
    - passthrough.frag/.vert - _post-process effect which does nothing but blit to the screen (used internally by the engine)_
    - skybox.frag/.vert - _performs skybox shading_
    - ssao.frag/.vert - _post-process effect which computes screen-space ambient occlusion_
    - fog.frag/.vert - _post-process effect which applies depth fog_
    - blur.frag/.vert - _post-process effect which blurs the input texture_
    - colour_correct.frag/.vert - _post-process effect which applies LUT and basic colour correction_
    - text.frag/.vert - _shader used by the text block object material_
    - gizmo.frag/.vert - _shader used by the gizmo object material_
  - textures
    - basic_lut.png - _LUT texture with no colour alterations_
  - samples
    - asha.obj - _a character 3D model (a fanart made by me), the eponymous bunnygirl_
    - asha.png - _texture for asha_
    - bunny.obj - _a demo 3D model of a bunny (again by me), her name is Julia_
    - bunny.png - _texture for bunny_
    - plane.obj - _quad demo object_
    - cube.obj - _cube demo object_
    - ico_sphere.obj - _icosahedron demo object_
    - uv_sphere.obj - _UV sphere demo object_
    - cylinder.obj - _cylinder demo object_
    - cone.obj - _cone demo object_
    - donut.obj - _torus demo object_
    - torus.obj - _another torus demo object?_
    - nasa_goddard_gaia_dr2_deep_star_map.png - _NASA SVS Deep Star Maps 2020 skybox texture_
    - psx.frag/.vert - _simple stylised shader with a vertex snapping/warping effect (used by asha and the bunny)_
  - font.bmp - _basic bitmap font_
  - icon.ico/.png - _another copy of the app icon_
  - newnodes.png - _contains elements used for drawing the nodes in the node view_
  - nodelinks.png - _contains more elements used for drawing the nodes in the node view_
  - node_shader.frag/.vert - _shader which draws the nodes in the node view_

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
