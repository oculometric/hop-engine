#pragma OMIT_TRANSFORM

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
    vars.uv = vert.uv;

    mat4 to_clip = scene.world_to_view;
    to_clip[0] = normalize(to_clip[0]);
    to_clip[1] = normalize(to_clip[1]);
    to_clip[2] = normalize(to_clip[2]);
    to_clip[3] = vec4(0, 0, 0, 1);
    clip = scene.view_to_clip * to_clip * vec4(vert.position.xyz, 1.0);
}

uniform sampler2D tex;

bool fragment(in Varyings vars, out Fragment frag)
{
    frag.colour = vec4(texture(tex, vars.uv.xy).rgb, 1);
    return true;
}