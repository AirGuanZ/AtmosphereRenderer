#ifndef POSTCOLOR_HLSL
#define POSTCOLOR_HLSL

#define POSTCOLOR_A 2.51
#define POSTCOLOR_B 0.03
#define POSTCOLOR_C 2.43
#define POSTCOLOR_D 0.59
#define POSTCOLOR_E 0.14

float3 tonemap(float3 input)
{
    return (input * (POSTCOLOR_A * input + POSTCOLOR_B))
         / (input * (POSTCOLOR_C * input + POSTCOLOR_D) + POSTCOLOR_E);
}

float3 postProcessColor(float2 seed, float3 input)
{
    input = tonemap(input);

    float rand = frac(sin(dot(
        seed, float2(12.9898, 78.233) * 2.0)) * 43758.5453);
    input = 255 * saturate(pow(input, 1 / 2.2));
    input = rand.xxx < (input - floor(input)) ? ceil(input) : floor(input);

    return input / 255;
}

#endif // #ifndef POSTCOLOR_HLSL
