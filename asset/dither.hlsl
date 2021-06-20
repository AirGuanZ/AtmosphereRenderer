#ifndef DITHER_HLSL
#define DITHER_HLSL

float3 avoidColorBanding(float2 seed, float3 input)
{
    float rand = frac(sin(dot(
        seed, float2(12.9898, 78.233) * 2.0)) * 43758.5453);
    input = 255 * saturate(input);
    input = rand.xxx < (input - floor(input)) ? ceil(input) : floor(input);
    return 1 / 255.0 * input;
}

#endif // #ifndef DITHER_HLSL
