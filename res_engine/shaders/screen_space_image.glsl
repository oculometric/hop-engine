#pragma DEFAULT_VERTEX

void vertex()
{
    #pragma CANVAS_TRANSFORM
    frag.position = object.model_to_world * vec4(in_position.xyz, 1);
    gl_Position.xy = round(frag.position.xy - (scene.viewport_size / 2.0f)) / scene.viewport_size;
    gl_Position.y = -gl_Position.y;
    frag.colour = in_colour;
}

#pragma CANVAS_ATTACHMENTS

layout(set = 2, binding = 0) uniform sampler2D image;

void fragment()
{
    vec2 uv = frag.uv;
    if (texture(image, uv).a < 0.5f)
        discard;

    out_colour = vec4(texture(image, uv).rgb, 1);
}