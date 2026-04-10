void vertex()
{
    #pragma CANVAS_TRANSFORM
    gl_Position.xy = round(in_position.xy - (scene.viewport_size / 2.0f)) / (scene.viewport_size / 2.0f);
    frag.colour = in_colour;
}

#pragma CANVAS_ATTACHMENTS

layout(set = 2, binding = 0) uniform sampler2D text_atlas;

void fragment()
{
    vec2 uv = frag.uv.xy;
    if (texture(text_atlas, uv).r < 0.5f)
        discard;

    out_colour = vec4(frag.colour.rgb, 1);
}