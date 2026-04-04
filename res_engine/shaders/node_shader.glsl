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
    int grid_scale;
    vec3 outline_colour_highlight;
    vec3 background_colour;
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
    frag.position = vec4(in_position.xy, 0, 1);
    frag.colour = in_colour;
    frag.normal = in_normal;
    frag.tangent = in_tangent;
    frag.uv = in_uv;
    // FIXME: consider this in viewport coordinates, round it, then take it back to clip coords
    vec2 viewport_half = floor(scene.viewport_size.xy / vec2(2, -2));
    vec2 camera_offset = floor(object.model_to_world[3].xy * vec2(0.5f, -0.5f)) / 0.5f;
    gl_Position = vec4((frag.position.xy + camera_offset) / viewport_half, 0.5f, 1.0f);
}

#pragma DEFAULT_ATTACHMENTS

layout(set = 2, binding = 1) uniform sampler2D node_atlas;
layout(set = 2, binding = 2) uniform sampler2D text_atlas;
layout(set = 2, binding = 3) uniform sampler2D extra_atlas;
layout(set = 2, binding = 4) uniform sampler2D ui_atlas;

vec2 nineSliceUV(vec2 uv, vec2 quad_size, vec2 atlas_size, bool top_border, bool bottom_border, bool left_border, bool right_border)
{
    vec2 pixels_one_third = atlas_size / 3.0f;
    vec2 pixels_two_third = pixels_one_third * 2.0f;
    vec2 pixel_coord = uv * quad_size;

    // figure out which region of the atlas we should be drawing
    int region_x = 0;
    int region_y = 0;
    if      (pixel_coord.x < pixels_one_third.x)               region_x = 0;
    else if (quad_size.x - pixel_coord.x < pixels_one_third.x) region_x = 2;
    else                                                       region_x = 1;

    if      (pixel_coord.y < pixels_one_third.y)               region_y = 0;
    else if (quad_size.y - pixel_coord.y < pixels_one_third.y) region_y = 2;
    else                                                       region_y = 1;

    // apply border toggles
    if      (region_x == 0 && !left_border)   region_x = 1;
    else if (region_x == 2 && !right_border)  region_x = 1;
    if      (region_y == 0 && !top_border)    region_y = 1;
    else if (region_y == 2 && !bottom_border) region_y = 1;
    
    // figure out our UV within the quadrant
    vec2 new_uv;
    switch (region_x)
    {
        case 0: new_uv.x = min(pixel_coord.x, pixels_one_third.x); break;
        case 1: new_uv.x = pixels_one_third.x + mod(pixel_coord.x, pixels_one_third.x); break;
        case 2: new_uv.x = max((pixel_coord.x - quad_size.x) + atlas_size.x, pixels_two_third.x); break;
    }
    switch (region_y)
    {
        case 0: new_uv.y = min(pixel_coord.y, pixels_one_third.y); break;
        case 1: new_uv.y = pixels_one_third.y + mod(pixel_coord.y, pixels_one_third.y); break;
        case 2: new_uv.y = max((pixel_coord.y - quad_size.y) + atlas_size.y, pixels_two_third.y); break;
    }
    new_uv = new_uv / atlas_size;

    return new_uv;
}

#define RENDER_MODE_BOX 0.0f
#define RENDER_MODE_TEXT 1.0f
#define RENDER_MODE_PINS 2.0f
#define RENDER_MODE_BACKGROUND 3.0f
#define RENDER_MODE_UI 4.0f

void fragment()
{
    vec2 uv = frag.uv;
    float render_mode = frag.normal.z;
    vec2 quad_size = frag.normal.xy;

    if (render_mode == RENDER_MODE_BOX)
    {
        // box mode
        int unpacked = int(frag.tangent.z);
        uv = nineSliceUV(uv, quad_size, textureSize(node_atlas, 0), bool(unpacked & 1), bool(unpacked & 2), bool(unpacked & 4), bool(unpacked & 8));

        vec3 fill = fill_colour;
        if      (frag.tangent.y == 1.0f) fill = frag.colour.rgb;
        else if (frag.tangent.y == 2.0f) fill = frag.colour.rgb * fill_modulate;
        else if (frag.tangent.y == 3.0f)
        {
            fill = frag.colour.rgb;
            if (mod(floor(frag.position.x / 2.0f), 2.0f) == mod(floor(frag.position.y / 2.0f), 2.0f))
                discard;
        }

        vec3 outline = fill;
        if (frag.tangent.x > 1.0f)
        {
            outline = outline_colour_highlight;
        }
        else if (frag.tangent.x > 0.0f)
        {
            if      (outline_style == 1) outline = outline_colour;
            else if (outline_style == 2) outline = frag.colour.rgb;
            else if (outline_style == 3) outline = frag.colour.rgb * outline_modulate;
        }

        vec4 alb = texture(node_atlas, uv);
        if (alb.a < 0.001f) discard;
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
        if (v < 0.5f && frag.tangent.x < 0.5f) out_colour = vec4(fill_colour, 1);
        else out_colour = vec4(frag.colour.rgb, 1);
    }
    else if (render_mode == RENDER_MODE_BACKGROUND)
    {
        // background grid mode
        vec2 mods = mod(abs(frag.position.xy), vec2(grid_size) * grid_scale);
        float factor = float(mods.x < 1.0f) + float(mods.y < 1.0f);
        if (factor > 0.0f)
            out_colour = vec4(frag.colour.rgb * (factor > 1.0f ? grid_dots_modulate : 1.0f), 1);
        else
            out_colour = vec4(background_colour, 1);
    }
    else if (render_mode == RENDER_MODE_UI)
    {
        // ui elements
        float v = texture(ui_atlas, uv).r;
        if (v < 0.001f) discard;
        if (v < 0.5f) out_colour = vec4(frag.tangent.rgb, 1);
        else out_colour = vec4(frag.colour.rgb, 1);
    }
}