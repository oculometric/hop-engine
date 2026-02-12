# instructions

hello David! this is a condensed version of the README to make marking easier.

## navigation
basic controls are what you'd expect:
- **W/S** - move forward/backward
- **A/D** - move left/right
- **E/Q** - move up/down
- **Right Mouse + Drag** - look around
- **Left Mouse** - select object under cursor (can be unintuitive)

there's also a bunch of ImGui windows but i won't go into a ton of detail about them here.

## key rendering features

**splines** - use the 'spline control' window to enable the camera flythrough or just track the planet moving on a spline

**render onto object** - if you look to the right when you load the application, you should see a little CRT monitor! if you move toward it, you will be able to see a copy of the screen texture visible on the CRT monitor.

**normal mapping** - nearly everything in the scene has normals! if you leave the main room through the doorway on the left, then turn left again, you can see a simple demo of normal mapping using a metal pipe and a demo texture. you should also be able to see them on the brick walls, concrete floor, desk, metal pipes, and the palm trees!

**post processing** - you should be able to see from the beginning that i've implemented SSAO, and colour grading using a LUT! but there's lots of other post-processing functionality, a whole sequence in fact. you can modify the colour grading using the 'colour correction' window; go ahead and try dragging the sliders. if you want more control, expand the 'scene' window and look at the 'render graph' section:
- here you can see all the different rendering steps being executed
- each one can be toggled on/off using the tickbox beside it
- you can see the details of each by hovering over it
- you can use the 'show step' field to see the outputs of each step in the sequence (-1 automatically uses the last step)
- you can use the 'show attachment' slider to change which output buffer of that step you're seeing
- some passes of interest are:
  - 'camera' which renders the scene itself into various G-buffers
  - 'shaded' which performs deferred rendering
  - 'ssao' which computes SSAO at quarter resolution
  - 'colour_grading' which applies, well, colour grading

**blur** - a Gaussian blur is implemented. this is used both to blur the SSAO to make it smoother (which can be seen in the 'scene'->'render graph' window by alternating between steps 3 and 4), but is also available as a whole-screen post-processing effect
- to enble it across the whole screen, open the 'scene'->'render graph' window and tick the last checkbox. this will enable the corresponding (blur) pass. make sure you're on step -1 in order to see it!

**deferred rendering** - i decided to implement deferred rendering using a two-pass approach. my rationale for this is described in this forum post: https://daf.staffs.ac.uk/topic/83681-costen-cassette-c025180n/#comment-1148115
- at the top of your screen, you can see a series of mini-views, which are the various key elements to the render process (left-to-right: albedo, normal, roughness/metallic/emission, specular, depth, after-shading, SSAO)
- to see in more detail, use the 'scene'->'render graph' window described above to switch to step 0, and drag the 'show attachment' slider to get a proper view of all the G-buffers emitted by the first pass of the deferred rendering process
- you can then switch to step 1, attachment 0 to see the result of the deferred shading on its own

## bonus features

**scene heirarchy** - nested transforms, as demonstrated by the orrery in center-stage!

**PBR shading** - everything in the scene makes use of a standard PBR shader

**text rendering** - you should be able to see a bunch of floating text in the scene. all of that is rendered (and is editable!) in realtime, using my own bitmap font

**object picking & editing** - left-clicking on an object allows you to select it. using the 'object' ImGui window, you can modify its position/rotation/scale, switch out the mesh or material, edit text, etc