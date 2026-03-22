#pragma DEFAULT_VERTEX

void vertex()
{
    frag.position = in_position;    // treated as position in pixel coordinates
    frag.colour = in_colour;        // .rgb  -> primary detail colour
                                    // .a    -> unused
    frag.normal = in_normal;        // .x    -> draw mode (0 = text, 1 = 9-slice, 2 = simple uv)
                                    // .y    -> texture slice (for 9-slice and simple uv modes)
                                    // .z    -> packed border booleans (for 9-slice mode)
                                    // .w    -> unused
    frag.tangent = in_tangent;      // .xy   -> quad size (when in 9-slice mode)
                                    // .zw   -> unused
    frag.uv = in_uv;
    // vertex coordinates are passed in in canvas space ({ 0, 0 } is top left, { width, height } is bottom right)
    gl_Position.xy = round(in_position.xy - (scene.viewport_size / 2.0f)) / (scene.viewport_size / 2.0f);
}

#pragma CANVAS_ATTACHMENTS

layout(set = 2, binding = 1) uniform sampler2D text_atlas;
layout(set = 2, binding = 2) uniform sampler3D ui_atlas;

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

void fragment()
{
    int draw_mode = int(frag.normal.x);
    vec3 fill_colour = frag.colour.rgb;

    if (draw_mode == 0)         // text mode
    {
        vec2 uv = frag.uv;
        if (texture(text_atlas, uv).r < 0.5f)
            discard;
        out_colour = vec4(fill_colour, 1);
    }
    else if (draw_mode == 1)    // 9-slice mode
    {
        vec2 quad_size = frag.tangent.xy;
        vec3 atlas_size = vec3(textureSize(ui_atlas, 0));
        uint borders = uint(frag.normal.z);
        int slice = int(frag.normal.y);
        vec2 uv = nineSliceUV(frag.uv, quad_size, atlas_size.xy, bool(borders & 1), bool(borders & 2), bool(borders & 4), bool(borders & 8));
        vec4 colour = texture(ui_atlas, vec3(uv, float(slice) / atlas_size.z));
        if (colour.a < 0.5f)
            discard;
        if (colour.rgb == vec3(1, 0, 1))
            out_colour = fill_colour;
        else
            out_colour = colour.rgb;
    }
    else if (draw_mode == 2)    // simple uv mode
    {
        vec3 atlas_size = vec3(textureSize(ui_atlas, 0));
        vec2 uv = frag.uv;
        int slice = int(frag.normal.y);
        vec4 colour = texture(ui_atlas, vec3(uv, float(slice) / atlas_size.z));
        if (colour.a < 0.5f)
            discard;
        out_colour = fill_colour;
    }
}