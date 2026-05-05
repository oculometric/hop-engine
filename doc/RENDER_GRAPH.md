# RenderGraph Serial Specification (`.hrgr`)

```C++
/* 
 * 'Resource' statement loads an asset to be used by the render graph.
 * - anonymous arguments
 * - children forbidden
 * - identifier required
 */
Resource(
    shader,                                    // resource type, must be 'shader'
    "res://engine/shaders/colour_correct.glsl" // path to the asset
) : colour_correct_shader;

/*
 * 'RenderPass' statement defines a render pass specification which can be used by render steps.
 * - anonymous arguments
 * - children forbidden
 * - identifier required
 */
RenderPass(
    FALSE, // whether or not a depth buffer should be present, either 'TRUE' or 'FALSE'
    0      // number of extra colour buffers present, between 0 and UINT32_MAX inclusive
) : custom_layout;

RenderPass(TRUE, 3) : default_layout;

/*
 * 'Camera' statement defines a camera render step, which will render scene objects.
 * - named arguments
 * - children forbidden
 * - identifier required
 */
Camera(
    slot = 0,                     // [REQUIRED] camera index which will be rendered into this pass, between 0 and UINT32_MAX inclusive
    scale = 1.0,                  // scale factor applied to canvas resolution, must be positive
    custom_size = [ 128, 128 ],   // custom canvas resolution, both values must be positive integers, overrides 'scale' argument if present
    render_pass = @default_layout // describes render pass layout, identifier of previously declared 'RenderPass' statement
) : scene;

/*
 * 'PostProcess' statement defines a full-screen-quad post-processing step, which will render a specified material.
 * - named arguments
 * - specific children allowed
 * - identifier required
 */
PostProcess(
    shader = @colour_correct_shader, // [REQUIRED] shader from which material will be constructed, identifier of previously declared 'Resource' statement
    scale = 1.0,                     // scale factor applied to canvas resolution, must be positive
    custom_size = [ 128, 128 ],      // custom canvas resolution, both values must be positive integers, overrides 'scale' argument if present
    render_pass = @custom_layout     // describes render pass layout, identifier of previously declared 'RenderPass' statement
) : colour_filter
{
    /*
     * 'Input' statement defines an input texture binding for a post-processing step, connecting it to previous render steps.
     * - named arguments
     * - children forbidden
     * - identifier forbidden
     */
    Input(
        binding = 0,     // [REQUIRED] texture sampler index in the shader to which the input will be bound, between 0 and UINT32_MAX inclusive
        step = @scene,   // [REQUIRED] render step from which to extract output, identifier of previously declared 'Camera' or 'PostProcess' statement
        attachment = 0,  // [REQUIRED] output attachment index to extract from the specified render step, between 0 and UINT32_MAX inclusive
        filter = LINEAR, // filtering mode for sampling the texture, either 'NEAREST' or 'LINEAR'
        address = CLAMP  // addressing mode for sampling the texture, one of 'REPEAT', 'MIRROR', 'CLAMP'
    )
};
```