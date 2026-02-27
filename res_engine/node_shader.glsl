#pragma DEFAULT_VERTEX

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    float grid_size;
};

void vertex()
{
    frag.position = vec4(in_position.xy, 0, 1);//(object.model_to_world * vec4(in_position.xyz, 1));
    frag.colour = in_colour;
    frag.normal = in_normal;
    frag.tangent = in_tangent;
    frag.uv = in_uv;
    // FIXME: consider this in viewport coordinates, round it, then take it back to clip coords
    vec2 viewport_half = floor(scene.viewport_size.xy / vec2(2, -2));
    gl_Position = vec4(in_position.xy / viewport_half, 0.5f, 1.0f);
}

#pragma DEFAULT_ATTACHMENTS

layout(set = 2, binding = 1) uniform sampler2D node_atlas;
layout(set = 2, binding = 2) uniform sampler2D text_atlas;

// different modes: node frame, fill, text, pins, background grid

vec2 nineSliceUV(vec2 uv, vec2 quad_size, vec2 atlas_size)
{
    // FIXME: actually fix nine-slicing so the UVs behave correctly, and we can index individual segments of the texture
    
    
    vec2 coordinate = uv * quad_size;
    vec2 atlas_border = atlas_size / 4.0f;
    vec2 c_over_s = coordinate / atlas_size;
    // if coordinate is less than a quarter of atlas size away from the edge of the quad
    if (coordinate.x <= atlas_border.x) uv.x = c_over_s.x;
    else if (quad_size.x - coordinate.x <= atlas_border.x) uv.x = (atlas_size.x - (quad_size.x - coordinate.x)) / atlas_size.x;
    else uv.x = 0.5f;

    if (coordinate.y <= atlas_border.y) uv.y = c_over_s.y;
    else if (quad_size.y - coordinate.y <= atlas_border.y) uv.y = (atlas_size.y - (quad_size.y - coordinate.y)) / atlas_size.y;
    else uv.y = 0.5f;
    
    return uv;
}

void fragment()
{
    vec2 uv = frag.uv;
    float render_mode = frag.normal.z;
    vec2 quad_size = frag.normal.xy;
    vec2 atlas_size = textureSize(node_atlas, 0);
    
    // TODO: different modes: filled/bordered box (with submodes for fill and border behaviour), text, pins, background grid
    
    if (render_mode < 0.1f)
    {
        uv = nineSliceUV(uv, quad_size, atlas_size);

        vec4 alb = texture(node_atlas, uv);
        if (alb.r < 0.001f) discard;
        if (alb.r > 0.9f) out_colour = vec4(frag.colour.rgb, 1);
        else out_colour = vec4(frag.colour.rgb * 0.03f, 1);
    }
    else if (render_mode < 0.2f)
    {
        uv = nineSliceUV(uv, quad_size, atlas_size);

        vec4 alb = texture(node_atlas, uv);
        if (alb.g < 0.001f) discard;
        if (alb.g > 0.9f) out_colour = vec4(frag.colour.rgb, 1);
        else out_colour = vec4(frag.colour.rgb * 0.03f, 1);
    }
    else if (render_mode < 0.3f)
    {
        vec2 mods = mod(abs(frag.position.xy), vec2(grid_size));
        // TODO: dots!
        //out_colour = vec4(mods.xy, 0, 1);
        if (mods.x < 1.0f || mods.y < 1.0f)
            out_colour = vec4(frag.colour.rgb, 1);
        else
            discard;
    }
    else
    {
        if (texture(text_atlas, uv).r < 0.5f) discard;
        out_colour = vec4(frag.colour.rgb, 1);
    }
}