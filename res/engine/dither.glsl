const float dither_map_0[1] = 
{
    0
};

const float dither_map_2[4] =
{
    0, 2,
    3, 1,
};

const float dither_map_4[16] =
{
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
};

const float dither_map_8[64] =
{
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21,
};

float dither_2x2(float f, ivec2 coord)
{
    return float(f * 4 >= dither_map_2[(coord.x % 2) + ((coord.y % 2) * 2)]);
}

float dither_4x4(float f, ivec2 coord)
{
    return float((f * 16) - 0.5f >= dither_map_4[(coord.x % 4) + ((coord.y % 4) * 4)]);
}

float dither_8x8(float f, ivec2 coord)
{
    return float(f * 64 >= dither_map_8[(coord.x % 8) + ((coord.y % 8) * 8)]);
}