# Material Serial Specification (`.hmat`)

```C++
/* 
 * 'Resource' statement loads an asset to be used by the material.
 * - anonymous arguments
 * - children forbidden
 * - identifier required
 */
Resource(
    shader,                         // resource type, either 'shader' or 'texture'
    "res://engine/shaders/psx.glsl" // path to the asset
) : shader;

Resource(texture, "res://engine/textures/bunny.png") : albedo;

/*
 * 'Shader' statement sets the shader module used for rendering.
 * - named arguments
 * - children forbidden
 * - identifier forbidden
 */
Shader(
    resource = @shader // [REQUIRED] identifier of a previous `Resource` statement
);

/*
 * `Depth` statement controls depth buffer interation parameters.
 * - named arguments
 * - children forbidden
 * - identifier forbidden
 */
Depth(
    operation = LESS_EQUAL, // depth comparison condition, one of 'ALWAYS', 'EQUAL', 'GREATER', 'GREATER_EQUAL', 'LESS', 'LESS_EQUAL', 'NEVER', 'NOT_EQUAL'
    test = TRUE,            // whether the depth buffer is tested, either 'TRUE' or 'FALSE'
    write = TRUE            // whether the depth buffer is modified, either 'TRUE', or 'FALSE'
);

/*
 * 'Culling' statement controls face culling behaviour.
 * - named arguments
 * - children forbidden
 * - identifier forbidden
 */
Culling(
    mode = NONE // [REQUIRED] which side of faces should be culled, one of 'NONE', 'FRONT', 'BACK'
);

/*
 * 'Polygon' statement controls triangle drawing behaviour.
 * - named arguments
 * - children forbidden
 * - identifier forbidden
 */
Polygon(
    mode = FILL // [REQUIRED] technique used to render geometry, one of 'FILL', 'LINE', 'POINT'
);

/*
 * 'Stencil' statement controls stencil buffer interaction.
 * - named arguments
 * - children forbidden
 * - identifier forbidden
 */
Stencil(
    compare = ALWAYS,           // [REQUIRED] stencil comparison condition, one of 'ALWAYS', 'EQUAL', 'GREATER', 'GREATER_EQUAL', 'LESS', 'LESS_EQUAL', 'NEVER', 'NOT_EQUAL'
    compare_value = 0,          // [REQUIRED] value to which the stencil buffer is compared, between 0 and UINT32_MAX inclusive
    compare_mask = 4294967295,  // [REQUIRED] bitmask value with which to AND stencil buffer, between 0 and UINT32_MAX inclusive
    write_mask = 4294967295,    // bitmask value controlling which bits are written in the stencil buffer
);

/*
 * 'RenderPass' statement specifies a custom render pass layout.
 * - named arguments
 * - children forbidden
 * - identifier forbidden
 */
RenderPass(
    extra_outputs = 0, // [REQUIRED] number of extra colour output buffers, between 0 and UINT32_MAX inclusive
    has_depth = TRUE   // [REQUIRED] whether a depth buffer will be present, either 'TRUE' or 'FALSE'
);

/*
 * 'Texture' statement binds a texture resource to a samplerXX uniform from the shader.
 * - named arguments
 * - children forbidden
 * - identifier forbidden
 */
Texture(
    binding = "albedo", // [REQUIRED] texture uniform name in shader
    resource = @albedo, // [REQUIRED] idenfitier of a previous 'Resource' statement
    filter = NEAREST,   // filtering mode for the sampler, either 'NEAREST' or 'LINEAR'
    address = REPEAT    // addressing mode for the sampler, one of 'REPEAT', 'MIRROR', 'CLAMP'
);

/* 
 * 'Uniform' statement assigns values to shader uniform variables.
 * - no arguments
 * - specific children allowed
 * - identifier forbidden
 */
Uniform()
{
    /*
     * 'vec4' statement specifies 4-component float vector variables.
     * - anonymous arguments
     * - no children allowed
     * - identifier forbidden
     */
    vec4(
        "material_colour", // name of the uniform variable in the shader
        [ 1, 1, 1, 1 ]     // vector value to be assigned
    );

    /*
     * 'float' statement specifies float variables.
     * - anonymous arguments
     * - no children allowed
     * - identifier forbidden
     */
    float(
        "contrast",     // name of the uniform variable in the shader
        0.95            // float value to be assigned
    );
};
```