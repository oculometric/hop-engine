#pragma OMIT_TRANSFORM

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
    vec4 position = object.model_to_world[3];
    clip = scene.view_to_clip * scene.world_to_view * position;
    clip /= clip.w;
    clip.xy += (vert.position.xy * 0.2f);
    vars.uv = vert.uv * vec3(1, -1, 1);
}

uniform sampler2D image;

bool fragment(in Varyings vars, inout Fragment frag)
{
    vec4 colour = texture(image, vars.uv.xy);
    if (colour.a < 0.5f)
        return false;
    frag.colour = colour;
    return true;
}