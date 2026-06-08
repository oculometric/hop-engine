#pragma CANVAS_TRANSFORM
#pragma CANVAS_ATTACHMENTS

// #include "res://engine/shaders/common.glsl"
// #include "res://engine/shaders/effects.glsl"

uniform sampler2D normal_texture;
uniform sampler2D depth_texture;

// uniform SSAOParams
// {
    
// }

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
}

const float n = 5.0f;

/*
 * based on the GTAO paper 'Practical Realtime Strategies for Accurate Indirect Occlusion' by Jimenez, Wu, Pesce, and Jarabo
 */
bool fragment(in Varyings vars, inout Fragment frag)
{
    // world space normal vector of the surface
    vec3 n = texture(normal_texture, vars.uv.xy).xyz;
    // non-linear z-depth from the camera
    float depth = texture(depth_texture, vars.uv.xy).x;
    // world space view vector of the pixel
    vec3 omega_0 = normalize((scene.view_to_world * scene.clip_to_view * vec4(vars.position.xyz, 1.0f)).xyz);
    // projected position of the pixel in clip space
    vec2 x_hat = vars.position.xy;
    // 
    float gamma = acos(dot(omega_0, n));

    // phi is the direction to search in the image plane
    float phi = 0.0f; // TODO: randomize
    // TODO: what should the divisor be here??
    vec2 t_phi = vec2(cos(phi), sin(phi)) / vec2(textureSize(depth_texture, 0));

    // maximum horizon angle in the t(phi) direction
    float theta_1 = 0;
    // maximum horizon angle in the -t(phi) direction
    float theta_2 = 0;


    for (float s = 1.0f; s <= n; s += 1.0f)
    {
        float s_hat = x_hat + (t_phi * s);
        
    }
    // float c = max(cos(theta - gamma), 0.0f) * abs(sin(theta));


    //frag.colour = vec4(vec3(dot(view, normal)), 1.0f);
    //frag.colour = vec4(texture(normal_texture, vars.uv.xy).rgb, 1.0f);
    //frag.colour = vec4(vec3(computeSSAO(1.0f, 2.0f, 0.025f, vars.uv.xy, vars.position.xy, normal_texture, depth_texture, samples)), 1);
    return true;
}