uniform Uniforms
{
    vec4 light_direction;
    vec4 light_colour;
    vec4 ambient_colour;
    vec4 surface_colour;
};

bool fragment(in Varyings vars, inout Fragment frag)
{
    frag.colour = vec4(surface_colour.rgb * (ambient_colour.rgb + (light_colour.rgb * clamp(0, 1, dot(-light_direction.xyz, vars.normal.xyz)))), 1.0f);
    return true;
}