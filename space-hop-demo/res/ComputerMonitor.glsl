uniform sampler2D albedo;
uniform sampler2D screen;

// borrowed from https://snowing.dev/articles/shader_noise.html

// This exponent in a float gives numbers in the range of [1.0, 2.0)
#define EXPONENT 0x3F800000
// Mask of the mantissa of a float
#define MASK 0x007FFFFF

ivec3 hash(ivec3 h)
{
    // This initial XOR is to make sure there is a
    // variety of bits to start with
    int a = h.x ^ 0x0fe382ac;
    int b = h.y ^ 0x7862c765;
    int c = h.z ^ 0xe63cf826;

    a ^= a << 3;
    a += a >> 5;
    a ^= a << 7;
    a += a >> 11;
    a ^= a << 13;
    a += a >> 17;
    a ^= a << 5;

    b ^= a;
    b += b >> 2;
    b ^= b << 4;
    b += b >> 6;
    b ^= b << 12;
    b += b >> 20;
    b ^= b << 13;
    b += b >> 5;
    b ^= b << 9;

    c ^= b;
    c += c >> 1;
    c ^= c << 3;
    c += c >> 8;
    c ^= c << 14;
    c += c >> 23;
    c ^= c << 11;
    c += c >> 6;
    c ^= c << 2;

    return ivec3(a,b,c);
}

float noise(vec3 seed)
{
    ivec3 h = hash(floatBitsToInt(seed));

    float result = intBitsToFloat((MASK & (h.x ^ h.y ^ h.z)) | EXPONENT);

    return result-1.0;
}

bool fragment(in Varyings vars, inout Fragment frag)
{
    if (vars.colour.b < 0.5f)
    {
        frag.colour = texture(albedo, vars.uv.xy);
    }
    else
    {
        vec4 col = texture(screen, vars.colour.rg);
        if (col.a > 0.5f)
            frag.colour.rgb = col.rgb;
        else
        {
            vec2 s = floor(vars.colour.rg * vec2(40, 15)) / vec2(40, 15);
            frag.colour.rgb = vec3(noise(vec3(s, scene.time)));
        }
    }
    frag.colour.a = 1.0f;
    return true;
}