vec4 snap(vec4 value)
{
    vec2 snapping_value = vec2(scene.viewport_size) * 0.25f;
    vec2 rounding = value.xy / value.w;
    vec2 snapped = round(rounding * snapping_value) / snapping_value;
    snapped *= value.w;
    return vec4(snapped, value.z, value.w);
}

#pragma DEFAULT_VERTEX

void vertex()
{
    frag.position = object.model_to_world * vec4(in_position.xyz, 1);
    frag.colour = in_colour;
    frag.normal = vec4(normalize((object.model_to_world * vec4(in_normal.xyz, 0)).xyz), 0);
    frag.tangent = vec4(normalize((object.model_to_world * vec4(in_tangent.xyz, 0)).xyz), 0);
    frag.uv = in_uv;

    gl_Position = snap(scene.view_to_clip * scene.world_to_view * frag.position);
}

layout(set = 2, binding = 1) uniform sampler2D albedo;

const vec3 highlight = vec3(1.300f, 1.300f, 1.300f);
const vec3 shadow = vec3(0.194f, 0.129f, 0.076f);

#pragma DEFAULT_ATTACHMENTS

void fragment()
{
    vec4 col = texture(albedo, frag.uv.xy);
    if (col.a < 0.5f)
        discard;

    float n_dot_l = dot(-scene.lights[0].direction.xyz, frag.normal.xyz) * 3.0f;
    if (n_dot_l < 0.061f)
        out_colour = vec4(col.rgb * shadow, 1);
    else if (n_dot_l < 0.82f)
        out_colour = vec4(mix(col.rgb, col.rgb * shadow, 1.0f - 0.341f), 1);
    else if (n_dot_l > 2.20f)
        out_colour = vec4(mix(col.rgb, col.rgb * highlight, 0.76f), 1);
    else
        out_colour = vec4(col.rgb, 1);
    out_normal = vec4(frag.normal.xyz, 0);
    out_custom = vec4(frag.position.xyz, 0);
    out_params = vec4(frag.colour.xyz, 0);
}