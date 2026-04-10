void vertex()
{
    #pragma DEFAULT_TRANSFORM
}

#pragma DEFAULT_ATTACHMENTS

void fragment()
{
    out_colour = vec4(1, 0, 1, 1);
    out_params.w = 0.0f;
}

