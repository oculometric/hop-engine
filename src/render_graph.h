#pragma once

// the render graph manages render passes, and binds cameras to them
// the render graph binds textures to post processing textures
// the render graph generates commands/executes them in order
// draw commands issued by objects specify which cameras should draw the object (we should check if the camera's render pass is compatible with the material's)
// there are two types of steps in the graph - camera, post-process
// render pass should have a duplicate command, which maintains its structure while creating additional buffers
// all cameras use duplicates of the standard offscreen render pass (colour, depth, four additional)
// post-process steps are free to use their own render passes
// cameras should define their background colour/skybox config
// default config, and a builder!!

#include "common.h"

namespace HopEngine
{

class RenderGraph
{

};

}

// TODO: build a graphical node editor for this....
// TODO: build a serialisable structure for this
