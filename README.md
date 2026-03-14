# hop-engine

a work-in-progress graphics engine focused on complex, non-destructive multi-step realtime art and videogame rendering worflows. intended to be a powerful but simple-to-use editing and rendering toolkit.

## Features (old)
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

## Credits
the beautiful skybox is a slightly modified (adding the checkerboard floor) version of the NASA SVS Deep Star Maps 2020. credit for this:
> NASA/Goddard Space Flight Center Scientific Visualization Studio. Gaia DR2: ESA/Gaia/DPAC. Constellation figures based on those developed for the IAU by Alan MacRobert of Sky and Telescope magazine (Roger Sinnott and Rick Fienberg).

libraries used:
- Vulkan
- GLFW
- glm
- ImGui
- SPIRV-reflect
- shaderc
- STB Image
- base64

all other assets (models, materials, textures, shaders, font, and demo scenes) are made by me. and the code obviously.

**particular thanks to LearnOpenGL, the Vulkan Tutorial, and David White for the guidance to develop this.**

_additional thanks to Lucy and Louise for keeping me alive and sane. <3_
