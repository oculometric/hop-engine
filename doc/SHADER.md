# Shader Format Specification (`.glsl`)

```glsl
/*
 * pragmas should be inserted at the top of the file. supported pragmas are listed below.
 *
 * `#pragma OMIT_TRANSFORM` - no vertex transform is performed. the `vec4` and `Varyings` parameters of the vertex function must be initialised inside the function.
 *
 * `#pragma CANVAS_TRANSFORM` - a simplified vertex transform is performed. should be used for post-processing shaders.
 *
 * `#pragma CANVAS_ATTACHMENTS` - a simplified output configuration is used and the additional fragment output buffers are disabled.
 * 
 * `#pragma OMIT_OBJECT_UNIFORMS` - skips inserting the object (set 1) uniform buffer.
 */

// includes are processed either relative to the path of the current shader file, or the `res://` prefix can be used to access files inside the resource registry.
#include "res://noise_function.glsl"

/*
 * descriptor set 0 contains the scene/camera uniform variables. the structure is inserted automatically, and has the following contents.
 * 
 * struct Light
 * {
 *     vec4 position;
 *     vec4 direction;
 *     vec4 colour;
 *     float spot_angle;
 *     int type;
 *     bool enabled;
 *     float padding;
 * };
 *
 * layout(set = 0, binding = 0) uniform SceneUniforms
 * {
 *     mat4 world_to_view;
 *     mat4 view_to_clip;
 *     mat4 clip_to_view;
 *     mat4 view_to_world;
 *     ivec2 viewport_size;
 *     vec3 eye_position;
 *     float time;
 *     vec2 near_far;
 *     Light lights[8];
 *     vec4 ambient_light;
 * } scene;
 * 
 * the contents of the scene uniform buffer can be accessed in the form `scene.world_to_view`, etc.
 */

/*
 * descriptor set 1 contains the object/instance uniform variables. the structure is inserted automatically, and has the following contents.
 *
 * layout(set = 1, binding = 0) uniform ObjectUniforms
 * {
 *     mat4 model_to_world;
 *     int id;
 * } object;
 *
 * the contents of the object uniform buffer can be accessed in the form `object.model_to_world`, etc.
 */

/*
 * you can freely specify additional uniform variable blocks. only blocks are supported (lone variables are not permitted).
 *
 * all additional uniform blocks are inserted into descriptor set 2, and the binding index is calculated automatically based on the order in which uniform blocks/samplers are declared in the shader.
 *
 * for example, this uniform block will be present at set 2, binding 0.
 *
 * variables in the block can be accessed simply by name, for example `colour`.
 */
uniform MaterialUniforms
{
    vec4 colour;
};

/*
 * you can freely specify texture-sampler uniforms.
 *
 * all texture-samplers must be combined image samplers, and are inserted into descriptor set 2, and the binding index is calculated automatically based on the order in which uniform blocks/samplers are declared in the shader.
 *
 * for example, this uniform texture will be present at set 2, binding 1.
 */
uniform sampler2D image_texture;

/*
 * the vertex shader is executed once per vertex in the mesh.
 * 
 * you may provide vertex shader functionality to customise the transformation applied to vertices before they pass into the varying-fragment pipeline. alternatively the vertex shader function may be omitted.
 *
 * unless the `OMIT_TRANSFORM` pragma has been specified, the vertices will be transformed according to a model-world-view-clip transformation series, and the values of `inout vec4` and `inout Varyings` will be updated before the function is called.
 *
 * - the vertex function must return `void`.
 * - the `in Vertex` parameter contains vertex attributes from the mesh.
 * - the `inout vec4` parameter contains the clip position which will be passed to the rasteriser (aka gl_Position), and is modifiable. unless `OMIT_TRANSFORM` has been specified, this will already have been calculated before the vertex function is executed.
 * - the `inout Varyings` parameter contains information which will be interpolated across triangles and passed to the fragment shader, and is modifiable. unless `OMIT_TRANSFORM` has been specified, this will already have been calculated before the vertex function is executed.
 *
 * the `Vertex` type has the following structure.
 *
 * struct Vertex
 * {
 *     vec4 position;
 *     vec4 colour;
 *     vec4 normal;
 *     vec4 tangent;
 *     vec3 uv;
 * };
 *
 * the `Varyings` type has the following structure.
 *
 * struct Varyings
 * {
 *     vec4 position;
 *     vec4 colour;
 *     vec4 normal;
 *     vec4 tangent;
 *     vec3 uv;
 * };
 *
 * - additional varying types may be specified as parameters in the format `out MyStruct ms`, as a fourth (or more) parameter to the vertex function. the type of the parameter must be a previously declared struct, and the contents must be initialised by the vertex function. if additional varying structs are specified, they must also be specified to the fragment function signature.
 */
void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
    // you may perform your own modification, such as custom transformations or warping, here
}

/*
 * the fragment shader is executed once per pixel in the framebuffer.
 *
 * you may provide functionality to calculate the values for fragment outputs, which translate to render pass attachments. alternatively the fragment shader may be omitted.
 *
 * the `Fragment` structure contains four buffers which can be written to; however if the `CANVAS_ATTACHMENTS` pragma is specified, all but the `colour` field are ignored.
 *
 * - the fragment function must return a `bool`. if `false` is returned, then the pixel will be discarded, otherwise the pixel will be submitted to the rasteriser. a `return false;` statement should be written instead of `discard;` if the discard functionality is desired (e.g. for masked transparency).
 * - the `in Varyings` parameter contains interpolated values from the vertex shader.
 * - the `inout Fragment` parameter contains fields which can be written to by the fragment function in order to send values to the framebuffer attachments.
 *
 * the `Fragment` type has the following structure.
 *
 * struct Fragment
 * {
 *     vec4 colour;
 *     vec4 normal;
 *     vec4 params;
 *     vec4 custom;
 * };
 *
 * - additional varying types may be specified as parameters in the format `in MyStruct ms`, before the `inout Fragment` parameter. the same restrictions apply as with additional varyings in the vertex shader, and the number, type, and ordering of input varyings to the fragment shader must match the varyings specified in the vertex function signature.
 */
bool fragment(in Varyings vars, inout Fragment frag)
{
    // you may perform whatever shading logic you wish here. values can be read from the `in Varyings` parameter
    vec2 uv = vars.uv.xy;
    // values can be sampled from texture-sampler uniforms, as well as block uniform variables
    vec4 col = texture(image_texture, uv) * colour;
    if (col.a < 0.5f)
    {
        // the function may return `false` (or `true`, as needed) early. returning `false` causes the pixel to be discarded
        return false;
    }
    // values can be written to the `inout Fragment` parameter
    frag.colour = vec4(col.rgb * scene.ambient_light.rgb, 1.0f);
    frag.normal.xyz = vars.normal.xyz;
    // the function must always return a value. if you want the pixel to be recorded to the framebuffer, return `true`
    return true;
}
```