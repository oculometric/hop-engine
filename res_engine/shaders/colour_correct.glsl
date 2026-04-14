#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

#include "res://common.glsl"
#include "res://effects.glsl"

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

uniform sampler2D tex;
uniform sampler3D lut;

uniform Params
{
    float gamma;
    float exposure;
    float offset;
    float use_lut;
};

bool fragment(in Varyings vars, out Fragment frag)
{
    vec3 colour = texture(tex, vars.uv.xy).rgb;
    if (use_lut > 0.5f) colour = sampleLut(colour, lut);
    frag.colour = vec4(gammaAdjust(colour, gamma, exposure, offset), 1);
    return true;
}