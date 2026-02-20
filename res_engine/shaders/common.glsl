vec3 toLinear(vec3 srgb)
{
    return vec3(mix(
        pow((srgb * 0.9478672986f) + 0.0521327014f, vec3(2.4f)),
        srgb * 0.0773993808f,
        lessThan(srgb, vec3(0.04045f))
        )
    );
}

vec3 toSRGB(vec3 linear)
{
    return vec3(mix(
        (pow(linear, vec3(0.41666f)) * 1.055f) - 0.055f,
        linear * 12.92f,
        lessThan(linear, vec3(0.0031308f))
        )
    );
}

float saturate(float f) { return min(max(f, 0.0f), 1.0f); }
