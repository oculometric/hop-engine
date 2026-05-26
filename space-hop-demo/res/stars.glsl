#pragma OMIT_TRANSFORM

void vertex(in Vertex vert, inout vec4 clip, inout Varyings vars)
{
    vars.uv = vert.uv;
    vars.position = vert.position;

    mat4 to_clip = scene.world_to_view;
    to_clip[0] = normalize(to_clip[0]);
    to_clip[1] = normalize(to_clip[1]);
    to_clip[2] = normalize(to_clip[2]);
    to_clip[3] = vec4(0, 0, 0, 1);
    clip = scene.view_to_clip * to_clip * vec4(vert.position.xyz, 1.0);
    vars.normal.xy = clip.xy;
}

float voronoi_hash(vec3 v)
{
    return fract(sin(dot(v, vec3(201.0f, 123.0f, 304.2f))) * 190493.02095f) * 2.0f - 1.0f;
}

// returns the distance metric for a given position in 3D voronoi nosie
float voronoi(vec3 position, float randomness, out vec3 containing_cell)
{
    vec3 cell = floor(position);
    float closest = 4.0f;
    vec3 closest_cell = vec3(0.0f);
    float max_rand = ceil(randomness);

    for (float z = cell.z - max_rand; z <= cell.z + max_rand; z += 1.0f)
    {
        for (float y = cell.y - max_rand; y <= cell.y + max_rand; y += 1.0f)
        {
            for (float x = cell.x - max_rand; x <= cell.x + max_rand; x += 1.0f)
            {
                vec3 test_cell = vec3(x, y, z);
                test_cell += vec3(voronoi_hash(vec3(x,y,z)), voronoi_hash(vec3(y,z,x)), voronoi_hash(vec3(z,x,y))) * randomness * 0.5f;

                float dist = length(position - test_cell);
                if (dist < closest)
                {
                    closest = dist;
                    closest_cell = vec3(x, y, z);
                }
            }
        }
    }

    containing_cell = closest_cell;
    return closest;
}

vec3 star_func(vec3 co, float r)
{
    vec3 v;
    float f = voronoi(co, r, v);
    float b = pow(1.0f - f, 100.0f) * 50.0f;
    float c = voronoi_hash(v);
    float cr = (1.0f - (c * 0.8f)) + 0.3f;
    float cg = 0.6f;
    float c_ = (1.4f * c) + 0.25f;
    float cb = ((c_ * c_ * (3.0f - (2.0f * c_))) * 0.8f) + 0.1f;
    return clamp(vec3(cr, cg, cb), vec3(0.0f), vec3(1.0f)) * b;
}

uniform sampler2D tex;

bool fragment(in Varyings vars, inout Fragment frag)
{
    vec3 dir = normalize(vars.position.xyz);

    float star_scale = 1.0f;
    //frag.colour = vec4(clamp(star_func(dir * 10.0f * star_scale, 1.0f) + star_func(dir * 25.0f * star_scale, 0.6f) + star_func(dir * 55.0f * star_scale, 0.6f), 0.0f, 1.0f), 1);
    vec2 uv = (vars.normal.xy + 1.0f) / 2.0f;
    frag.colour = vec4(texture(tex, vec2(uv.x, 1.0f - uv.y)).rgb, 1.0f);
    return true;
}
