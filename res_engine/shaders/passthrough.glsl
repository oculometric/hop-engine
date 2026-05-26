#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
    clip = scene.view_to_clip * vec4(vert.position.xyz, 1);
}

uniform sampler2D screen_texture;
uniform sampler2D stencil_texture;
uniform MaterialUniforms
{
    int display_depth;
};

bool fragment(in Varyings vars, inout Fragment frag)
{
    if (display_depth == 1)
    {
        float d = texture(screen_texture, vars.uv.xy).r;
        float depth = scene.near_far.x * scene.near_far.y / (scene.near_far.y + d * (scene.near_far.x - scene.near_far.y));
        frag.colour = vec4(vec3(depth), 1);
    }
    else if (display_depth == 2)
    {
        float d = texture(stencil_texture, vars.uv.xy).r * 16.0f;
        frag.colour = vec4(vec3(d), 1);
    }
    else frag.colour = texture(screen_texture, vars.uv.xy);
    return true;
}