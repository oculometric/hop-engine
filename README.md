# hop-engine
hello! welcome to my advanced graphics project. i went a bit further with this than the module requires, mostly in terms of turning this into a proper graphics engine. how to use, features, and limitations are listed below.

## Navigation
basic controls are what you'd expect:
- W/S - move forward/backward
- A/D - move left/right
- E/Q - move up/down
- Right Mouse + Drag - look around
- Left Mouse - select object under cursor (can be unintuitive)

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
- scene heirarchy
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

## Project Structure
this turned into a really big project so the structure is a little complicated

## Limitations
- does not support deferred lighting (i.e. separating deferred rendering), although this would be simple to implement via the render graph system
- 

## Credits
the beautiful skybox is a slightly modified (adding the checkerboard floor) version of the NASA 2020 SVS Deep Star Maps. credit for this:
NASA/Goddard Space Flight Center Scientific Visualization Studio. Gaia DR2: ESA/Gaia/DPAC. Constellation figures based on those developed for the IAU by Alan MacRobert of Sky and Telescope magazine (Roger Sinnott and Rick Fienberg).

libraries used:
- Vulkan
- GLFW
- glm
- ImGui
- SPIRV-reflect
- STB Image
- base64

all other assets (models, materials, textures, shaders, font, and demo scenes) are made by me. and the code obviously.

particular thanks to LearnOpenGL, the Vulkan Tutorial, and David White for the guidance to develop this.
additional thanks to Lucy and Louise for keeping me sane.


// TODO: explain features and where they're implemented
// TODO: project structure
// TODO: reflect on limitations more