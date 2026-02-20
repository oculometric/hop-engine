#pragma DEFAULT_VERTEX

layout(set = 2, binding = 0) uniform MaterialUniforms
{
    bool debug_segments;
    int background_mode;
    float background_factor;
    vec4 background_colour;
};

void vertex()
{
    frag.position = (object.model_to_world * vec4(in_position.xyz, 1));
    frag.colour = in_colour;
    frag.normal = in_normal;
    frag.tangent = in_tangent;
    frag.uv = in_uv;
    vec2 translation = vec2(scene.world_to_view[3][0], scene.world_to_view[3][1]);
    vec2 downsized_viewport = floor(scene.viewport_size.xy * 0.25f);
    translation = floor(translation * downsized_viewport) / downsized_viewport;
    mat4 translation_only = mat4(1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    translation.x, translation.y, 0.5f, 1);
    gl_Position = translation_only * ((object.model_to_world * vec4(in_position.xyz, 1) / vec4(scene.viewport_size.xy / 2.0f, 1, 1)) * vec4(1, -1, -1, 1));
    gl_Position.xy = floor(gl_Position.xy * floor(scene.viewport_size.xy * 0.5f)) / floor(scene.viewport_size.xy * 0.5f);
}

#pragma DEFAULT_ATTACHMENTS

layout(set = 2, binding = 1) uniform sampler2D node_atlas;
layout(set = 2, binding = 2) uniform sampler2D text_atlas;
layout(set = 2, binding = 3) uniform sampler2D link_atlas;

const float slice_size = 24.0f;
const float border_width = 1.0f;
const float bordered_size = slice_size + (2.0f * border_width);
const float border_ratio = slice_size / bordered_size;
const float border_fraction = border_width / bordered_size;

void fragment()
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
        out_colour = vec4(frag.colour.rgb, 1);
    }
    else if (frag.normal.z > 0.0f)
    {
        if (texture(text_atlas, uv).r < 0.5f)
            discard;
        out_colour = vec4(frag.colour.rgb, 1);
    }
    else
    {
        vec2 size_units = frag.normal.xy;
        vec2 scaled_uv = frag.uv * size_units;
        ivec2 segment = ivec2(floor(scaled_uv));
        if (debug_segments)
        {
            out_colour = vec4(fract(scaled_uv), 0, 1);
        }
        else
        {
            vec2 fraction = fract(scaled_uv);
            fraction *= border_ratio;
            fraction += border_fraction;
            uv = fraction + 1.0f;
            uv -= vec2(lessThan(segment, ivec2(1, 1)));
            uv += vec2(greaterThan(segment, size_units - ivec2(2, 2)));

            uv /= 3.0f;

            vec4 tex_sample = texture(node_atlas, uv);
            float factor = length(tex_sample.rgb * frag.colour.rgb) * tex_sample.a;
            if (factor <= 0.001f)
                discard;
            else if (factor <= 0.7f)
                out_colour = vec4(background_mode == 0 ? background_colour.rgb : frag.tangent.rgb * background_factor, 1);
            else
                out_colour = vec4(frag.tangent.rgb, 1);
        }
    }
}