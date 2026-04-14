#pragma OMIT_TRANSFORM

vec4 snap(vec4 value)
{
    vec2 snapping_value = vec2(scene.viewport_size) * 0.25f;
    vec2 rounding = value.xy / value.w;
    vec2 snapped = round(rounding * snapping_value) / snapping_value;
    snapped *= value.w;
    return vec4(snapped, value.z, value.w);
}

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
    vars.position = object.model_to_world * vec4(vert.position.xyz, 1);
    vars.colour = vert.colour;
    vars.normal = vec4(normalize((object.model_to_world * vec4(vert.normal.xyz, 0)).xyz), 0);
    vars.tangent = vec4(normalize((object.model_to_world * vec4(vert.tangent.xyz, 0)).xyz), 0);
    vars.uv = vert.uv;

    clip = snap(scene.view_to_clip * scene.world_to_view * vars.position);
}

uniform sampler2D albedo;

const vec3 highlight = vec3(1.300f, 1.300f, 1.300f);
const vec3 shadow = vec3(0.194f, 0.129f, 0.076f);

bool fragment(in Varyings vars, out Fragment frag)
{
    vec4 col = texture(albedo, vars.uv.xy);
    if (col.a < 0.5f)
        return false;

    float n_dot_l = dot(-scene.lights[0].direction.xyz, vars.normal.xyz) * 3.0f;
    if (n_dot_l < 0.061f)
        frag.colour = vec4(col.rgb * shadow, 1);
    else if (n_dot_l < 0.82f)
        frag.colour = vec4(mix(col.rgb, col.rgb * shadow, 1.0f - 0.341f), 1);
    else if (n_dot_l > 2.20f)
        frag.colour = vec4(mix(col.rgb, col.rgb * highlight, 0.76f), 1);
    else
        frag.colour = vec4(col.rgb, 1);
    frag.normal = vec4(vars.normal.xyz, 0);
    frag.custom = vec4(vars.position.xyz, 0);
    frag.params = vec4(vars.colour.xyz, 0);
    return true;
}