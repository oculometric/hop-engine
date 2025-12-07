#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    bool debug_segments;
};

layout(set = 2, binding = 1) uniform sampler2D sliced_texture;
layout(set = 2, binding = 2) uniform sampler2D text_atlas;

void main()
{
    vec2 uv = frag.uv;
    
    if (frag.colour.z > 0)
    {
        if (texture(text_atlas, uv).r < 0.5f)
            discard;

        colour = vec4(0, 0, 0, 1);
    }
    else
    {
        vec2 size_units = frag.colour.xy;
        vec2 scaled_uv = frag.uv * size_units;
        vec2 segment = vec2(floor(scaled_uv));
        if (debug_segments)
        {
            colour = vec4(segment / size_units, 0, 1);
        }
        else
        {
            vec2 fraction = scaled_uv - segment;
            uv = fraction + 1.0f;
            uv -= vec2(equal(segment, vec2(0, 0)));
            uv += vec2(equal(segment, size_units - vec2(1, 1)));

            uv /= 3.0f;

            vec4 tex_sample = texture(sliced_texture, uv);
            if (tex_sample.a < 0.5f)
                discard;
            colour = vec4(tex_sample.rgb, 1);
        }
    }
}