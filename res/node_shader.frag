#version 450

#define FRAGMENT
#include "common.glsl"

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    bool debug_segments;
    vec4 background_colour;
};

layout(set = 2, binding = 1) uniform sampler2D node_atlas;
layout(set = 2, binding = 2) uniform sampler2D text_atlas;
layout(set = 2, binding = 3) uniform sampler2D link_atlas;

const float slice_size = 24.0f;
const float border_width = 1.0f;
const float bordered_size = slice_size + (2.0f * border_width);
const float border_ratio = slice_size / bordered_size;
const float border_fraction = border_width / bordered_size;

void main()
{
    vec2 uv = frag.uv;
    
    if (frag.normal.z > 0.7f)
    {
        vec2 fraction = fract(uv);
        fraction *= border_ratio;
        fraction += border_fraction;
        uv = (fraction + floor(uv)) / 4.0f;
        if (texture(link_atlas, vec2(uv.x, 1.0f - uv.y)).a < 0.5f)
            discard;
        colour = vec4(frag.colour.rgb, 1);
    }
    else if (frag.normal.z > 0.0f)
    {
        if (texture(text_atlas, uv).r < 0.5f)
            discard;
        colour = vec4(frag.colour.rgb, 1);
    }
    else
    {
        vec2 size_units = frag.normal.xy;
        vec2 scaled_uv = frag.uv * size_units;
        ivec2 segment = ivec2(floor(scaled_uv));
        if (debug_segments)
        {
            colour = vec4(fract(scaled_uv), 0, 1);
        }
        else
        {
            vec2 fraction = fract(scaled_uv);
            fraction *= border_ratio;
            fraction += border_fraction;
            fraction = clamp(fraction, border_fraction, 1.0f - border_fraction);
            uv = fraction + 1.0f;
            uv -= vec2(lessThan(segment, ivec2(1, 1)));
            uv += vec2(greaterThan(segment, size_units - ivec2(2, 2)));
            
            uv /= 3.0f;

            vec4 tex_sample = texture(node_atlas, uv);
            float factor = length(tex_sample.rgb * frag.colour.rgb) * tex_sample.a;
            if (factor <= 0.001f)
                discard;
            else if (factor <= 0.7f)
                colour = vec4(background_colour.rgb, 1);
            else
                colour = vec4(frag.tangent.rgb, 1);
        }
    }
}