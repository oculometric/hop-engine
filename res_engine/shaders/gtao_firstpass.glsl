#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

// #include "res://engine/shaders/common.glsl"
// #include "res://engine/shaders/effects.glsl"

uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform sampler2D colour_texture;

// uniform SSAOParams
// {
    
// }

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

const float n = 4.0f;
const int samples = 4;
const float scale = 0.8f;
const float power = 1.0f;

vec3 screenPointToWorldSpace(vec2 screen_point, out float depth)
{
    depth = texture(depth_texture, (screen_point + 1.0f) * 0.5f).r;
    vec4 tmp = scene.clip_to_view * vec4(screen_point, depth, 1.0f);
    tmp /= tmp.w;
    depth = tmp.z;
    return (scene.view_to_world * tmp).xyz;
}

float computeHorizon(vec3 omega_o, vec2 x_hat, vec3 x, float inv_sq_depth, vec2 t_phi)
{
    float max_s_dot_o = 0.0f;
    for (float k = 1.0f; k <= n; k += 1.0f)
    {
        // new test pixel in the image plane
        vec2 s_hat = x_hat + (t_phi * k * inv_sq_depth);
        // point in world space for s
        float d;
        vec3 s = screenPointToWorldSpace(s_hat, d);
        // vector from x to s
        vec3 omega_s = normalize(s - x);
        float s_dot_o = max(dot(omega_s, omega_o), 0.0f);
        max_s_dot_o = max(s_dot_o, max_s_dot_o);
    }
    return acos(max_s_dot_o);
}

float computeInnerIntegral(vec3 omega_o, vec2 x_hat, vec3 x, vec3 n_x, float inv_sq_depth, float phi)
{
    // vector in the image plane based on phi
    vec3 perp_a = cross(omega_o, scene.view_to_world[1].xyz);
    vec3 perp_b = cross(scene.view_to_world[0].xyz, omega_o);
    vec3 t_phi = (perp_a * cos(phi)) + (perp_b * sin(phi));

    vec2 t_hat_phi = vec2(dot(t_phi, scene.view_to_world[0].xyz), dot(t_phi, scene.view_to_world[1].xyz)) / vec2(scene.viewport_size);

    // compute plane defined by t_phi (i.e. unprojected t_hat_phi) and omega_o
    vec3 proj_plane = normalize(cross(t_phi, omega_o));
    // project n_x onto plane
    vec3 proj_n_x = n_x - (dot(n_x, proj_plane) * proj_plane);
    if (length(proj_n_x) != 1.0f)
        proj_n_x = n_x;
    // angle between normal and view vector
    float gamma = acos(dot(omega_o, normalize(proj_n_x)));
    
    // horizons on both sides of the vector
    float theta_1 = computeHorizon(omega_o, x_hat, x, inv_sq_depth, t_hat_phi);
    float theta_2 = computeHorizon(omega_o, x_hat, x, inv_sq_depth, -t_hat_phi);

    // solve inner integral a_hat
    float cos_gamma = cos(gamma);
    float sin_gamma = sin(gamma);
    // TODO: OPTIMISE
    float a_hat = ((cos_gamma + (2.0f * theta_1 * sin_gamma) - cos((2.0f * theta_1) - gamma))
                 + (cos_gamma + (2.0f * theta_2 * sin_gamma) - cos((2.0f * theta_2) - gamma))
    ) / 4.0f;

    return length(proj_n_x) * a_hat;
}

const float dither_map_4[16] =
{
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
};

/*
 * based on the GTAO paper 'Practical Realtime Strategies for Accurate Indirect Occlusion' by Jimenez, Wu, Pesce, and Jarabo
 */
bool fragment(in Varyings vars, inout Fragment frag)
{
    // world space normal vector of the surface
    vec3 n_x = texture(normal_texture, vars.uv.xy).xyz;
    // projected position of the pixel in clip space
    vec2 x_hat = vars.position.xy;
    // pixel point in world space
    float depth;
    vec3 x = screenPointToWorldSpace(x_hat, depth);
    float inv_sq_depth = depth * depth;
    // world space view vector of the pixel
    vec3 omega_o = normalize(scene.eye_position - x);

    float step_size = 3.1415f / float(samples + 1.0f);
    ivec2 coord = ivec2(floor(vars.uv.xy * scene.viewport_size));
    float phi = step_size + (dither_map_4[(coord.x % 4) + ((coord.y % 4) * 4)] * 3.1415f / 16.0f);
    float first = computeInnerIntegral(omega_o, x_hat, x, n_x, inv_sq_depth, 0.0f);

    float total = 0.0f;
    for (int i = 0; i < samples - 1; ++i)
    {
        float second = computeInnerIntegral(omega_o, x_hat, x, n_x, inv_sq_depth, phi);
        float area = (second + first) * (step_size / 2.0f);
        first = second;
        total += area;
        phi += step_size;
    }

    float final = (total / 3.1415f);  //pow((total / 3.1415f) * scale, power);

    frag.colour = texture(colour_texture, vars.uv.xy);
    if (length(n_x) > 0.5f)
        frag.colour.rgb = frag.colour.rgb * final;
    return true;
}