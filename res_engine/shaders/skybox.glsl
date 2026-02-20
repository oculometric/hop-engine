#pragma DEFAULT_VERTEX

void vertex()
{
    frag.uv = in_uv;

    mat4 to_clip = scene.world_to_view;
    to_clip[0] = normalize(to_clip[0]);
    to_clip[1] = normalize(to_clip[1]);
    to_clip[2] = normalize(to_clip[2]);
    to_clip[3] = vec4(0, 0, 0, 1);
    gl_Position = scene.view_to_clip * to_clip * vec4(in_position.xyz, 1.0);
}

#pragma DEFAULT_ATTACHMENTS

layout(set = 2, binding = 0) uniform sampler2D tex;

void fragment()
{
    out_colour = vec4(texture(tex, frag.uv).rgb, 1);
    out_params.w = 0.0f;
}