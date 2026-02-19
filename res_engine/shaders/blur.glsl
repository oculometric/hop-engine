// version 450 gets inserted first

// descriptor sets 0 (scene) and 1 (object) get inserted next

//#pragma DEFAULT_ATTACHMENTS <-- generates the 1 colour, 3 data attachments
//#pragma CANVAS_ATTACHMENTS  <-- generates the basic 1 colour attachment

// vertex function and fragment function must be present

// vertex layout inputs and fragment layout outputs are inserted here
void vertex()
{
    //#pragma DEFAULT_VERTEX  <-- generates a vertex function as is standard for object shaders
    //#pragma CANVAS_VERTEX   <-- generates a vertex function as is standard for canvas shaders (i.e. as below)
    frag.position = vec4(in_position.xyz, 1);
    frag.uv = in_uv;
    gl_Position = vec4(in_position.xyz, 1);
}

// adding the 'res://' prefix allows accessing files via the package system
#include "res://engine/common.glsl"
#include "res://effects.glsl"

// uniforms have 'layout ... (set = 2, binding = n) ...' inserter automatically
uniform sampler2D main_tex;

// fragment layout inputs are inserted here
void fragment()
{
    evaluateKernel(gaussian_kernel_9, 9, main_tex, out_colour);
}
