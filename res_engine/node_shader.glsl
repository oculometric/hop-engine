#pragma DEFAULT_VERTEX

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    float grid_size;
    vec3 outline_colour;
    int outline_style;
    float outline_modulate;
    vec3 fill_colour;
    float fill_modulate;
    float grid_dots_modulate;
};

// int outline_mode;
// 0 - no outline
// 1 - outline use preset colour
// 2 - outline use node colour
// 3 - outline modulate node colour

// int fill_mode;
// 0 - fill use preset colour
// 1 - fill use node colour
// 2 - fill modulate node colour

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
layout(set = 2, binding = 3) uniform sampler2D extra_atlas;

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

#define RENDER_MODE_BOX 0.0f
#define RENDER_MODE_TEXT 0.1f
#define RENDER_MODE_PINS 0.2f
#define RENDER_MODE_BACKGROUND 0.3f

void fragment()
{
    vec2 uv = frag.uv;
    float render_mode = frag.normal.z;
    vec2 quad_size = frag.normal.xy;
    vec2 atlas_size = textureSize(node_atlas, 0);

    if (render_mode == RENDER_MODE_BOX)
    {
        // box mode
        uv = nineSliceUV(uv, quad_size, atlas_size);

        vec3 fill = fill_colour;
        if (frag.tangent.y == 1.0f)      fill = frag.colour.rgb;
        else if (frag.tangent.y == 2.0f) fill = frag.colour.rgb * fill_modulate;

        vec3 outline = fill;
        if (frag.tangent.x > 0.0f)
        {
            if (outline_style == 1)      outline = outline_colour;
            else if (outline_style == 2) outline = frag.colour.rgb;
            else if (outline_style == 3) outline = frag.colour.rgb * outline_modulate;
        }

        vec4 alb = texture(node_atlas, uv);
        if (alb.r < 0.001f) discard;
        if (alb.r > 0.9f)   out_colour = vec4(fill, 1);
        else                out_colour = vec4(outline, 1);
    }
    else if (render_mode == RENDER_MODE_TEXT)
    {
        // text mode
        if (texture(text_atlas, uv).r < 0.5f) discard;
        out_colour = vec4(frag.colour.rgb, 1);
    }
    else if (render_mode == RENDER_MODE_PINS)
    {
        // pins mode
        float v = texture(extra_atlas, uv).b;
        if (v < 0.001f) discard;
        if (v < 0.5f && frag.tangent.x < 0.5f) discard;
        out_colour = vec4(frag.colour.rgb, 1);
    }
    else if (render_mode == RENDER_MODE_BACKGROUND)
    {
        // background grid mode
        vec2 mods = mod(abs(frag.position.xy), vec2(grid_size));
        float factor = float(mods.x < 1.0f) + float(mods.y < 1.0f);
        if (factor > 0.0f)
            out_colour = vec4(frag.colour.rgb * (factor > 1.0f ? grid_dots_modulate : 1.0f), 1);
        else
            discard;
    }
}