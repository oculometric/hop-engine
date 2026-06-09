#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

#include "res://engine/shaders/dither.glsl"

uniform sampler2D normal_texture;
uniform sampler2D depth_texture;
uniform sampler2D colour_texture;

uniform SSAOParams
{
    float filter_radius; // default 12
    int samples; // default 4
    bool use_smoothstep; // default false
    float power; // default 1.2
    float strength; // default 1.0
};

struct SSAOConstants
{
    float filter_radius;
    int samples;
    vec3 omega_o;
    vec2 x_hat;
    vec3 x;
    vec3 n_x;
    float inv_sq_depth;
    vec3 perp_a;
    vec3 perp_b;
};

vec3 screenPointToWorldSpace(vec2 screen_point, out float depth)
{
    depth = texture(depth_texture, (screen_point + 1.0f) * 0.5f).r;
    vec4 tmp = scene.clip_to_view * vec4(screen_point, depth, 1.0f);
    tmp /= tmp.w;
    depth = tmp.z;
    return (scene.view_to_world * tmp).xyz;
}

float computeHorizon(in SSAOConstants constants, vec2 t_phi)
{
    float max_s_dot_o = 0.0f;
    for (float k = 1.0f; k <= constants.filter_radius; k += 1.0f)
    {
        // new test pixel in the image plane
        vec2 s_hat = constants.x_hat + (t_phi * k * constants.inv_sq_depth);
        // point in world space for s
        float d;
        vec3 s = screenPointToWorldSpace(s_hat, d);
        // vector from x to s
        vec3 omega_s = normalize(s - constants.x);
        float s_dot_o = max(dot(omega_s, constants.omega_o), 0.0f);
        max_s_dot_o = max(s_dot_o, max_s_dot_o);
    }
    return acos(max_s_dot_o);
}

float computeInnerIntegral(in SSAOConstants constants, float phi)
{
    // vector in the image plane based on phi
    vec3 t_phi = (constants.perp_a * cos(phi)) + (constants.perp_b * sin(phi));
    vec2 t_hat_phi = vec2(dot(t_phi, scene.view_to_world[0].xyz), dot(t_phi, scene.view_to_world[1].xyz)) / vec2(scene.viewport_size);

    // compute plane defined by t_phi (i.e. unprojected t_hat_phi) and omega_o
    vec3 proj_plane = normalize(cross(t_phi, constants.omega_o));
    // project n_x onto plane
    vec3 proj_n_x = constants.n_x - (dot(constants.n_x, proj_plane) * proj_plane);
    proj_n_x = (dot(proj_n_x, proj_n_x) != 1.0f) ? constants.n_x : proj_n_x;
    // angle between normal and view vector
    float cos_gamma = dot(constants.omega_o, normalize(proj_n_x));
    float gamma = acos(cos_gamma);
    
    // horizons on both sides of the vector
    float theta_1 = computeHorizon(constants, t_hat_phi);
    float theta_2 = computeHorizon(constants, -t_hat_phi);

    // solve inner integral a_hat
    float sin_gamma = sin(gamma);
    float a_hat = (
        (cos_gamma + (2.0f * theta_1 * sin_gamma)) + cos_gamma + (2.0f * theta_2 * sin_gamma)
       - (cos((2.0f * theta_1) - gamma) + cos((2.0f * theta_2) - gamma))
    ) / 4.0f;

    return length(proj_n_x) * a_hat;
}

/*
 * based on the GTAO paper 'Practical Realtime Strategies for Accurate Indirect Occlusion' by Jimenez, Wu, Pesce, and Jarabo
 */
bool fragment(in Varyings vars, inout Fragment frag)
{
    // world space normal vector of the surface
    vec3 n_x = texture(normal_texture, vars.uv.xy).xyz;
    frag.colour = texture(colour_texture, vars.uv.xy);
    if (length(n_x) < 0.5f)
        return true;
    // projected position of the pixel in clip space
    vec2 x_hat = vars.position.xy;
    // pixel point in world space
    float depth;
    vec3 x = screenPointToWorldSpace(x_hat, depth);
    float inv_sq_depth = depth * depth;
    // world space view vector of the pixel
    vec3 omega_o = normalize(scene.eye_position - x);

    SSAOConstants constants;
    if (filter_radius <= 0.0f)
        constants.filter_radius = 12.0f;
    else
        constants.filter_radius = filter_radius;
    if (samples <= 0)
        constants.samples = 4;
    else
        constants.samples = samples;
    constants.omega_o = omega_o;
    constants.x_hat = x_hat;
    constants.x = x;
    constants.n_x = n_x;
    constants.inv_sq_depth = inv_sq_depth;
    constants.perp_a = cross(constants.omega_o, scene.view_to_world[1].xyz);
    constants.perp_b = cross(scene.view_to_world[0].xyz, constants.omega_o);

    // TODO: FIX THE INTEGRATION PROPERLY
    int _samples = samples;
    if (_samples <= 0)
        _samples = 4;
    float step_size = 3.1415f / float(_samples + 1);
    ivec2 coord = ivec2(floor(vars.uv.xy * scene.viewport_size));
    float phi = (dither_map_4[(coord.x % 4) + ((coord.y % 4) * 4)] * 3.1415f / 16.0f);
    float first = computeInnerIntegral(constants, phi);

    float total = 0.0f;
    for (int i = 0; i < _samples - 1; ++i)
    {
        phi += step_size;
        float second = computeInnerIntegral(constants, phi);
        float area = (second + first) * (step_size / 2.0f);
        first = second;
        total += area;
    }
    total /= 3.1415f;

    float final = use_smoothstep ? smoothstep(0, 1, total) : total;
        
    final = pow(final, (power <= 0.0f) ? 1.2f : power);

    frag.colour.rgb = mix(frag.colour.rgb, frag.colour.rgb * final, (strength <= 0.0f) ? 1.0f : strength);
    return true;
}