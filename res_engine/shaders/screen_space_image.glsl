#pragma OMIT_TRANSFORM
#pragma CANVAS_ATTACHMENTS

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
    vars.position = object.model_to_world * vec4(vert.position.xyz, 1);
    clip.xy = round(vars.position.xy - (scene.viewport_size / 2.0f)) / scene.viewport_size;
    clip.y = -clip.y;
    clip.z = 1;
    vars.colour = vert.colour;
    vars.uv = vert.uv;
}

uniform sampler2D image;

bool fragment(in Varyings vars, inout Fragment frag)
{
    vec2 uv = vars.uv.xy;
    if (texture(image, uv).a < 0.5f)
        return false;

    frag.colour = vec4(texture(image, uv).rgb, 1);
    return true;
}