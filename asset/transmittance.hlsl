#define THREAD_GROUP_SIZE_X 16
#define THREAD_GROUP_SIZE_Y 16

#define STEP_COUNT 1000

#include "./intersection.hlsl"
#include "./medium.hlsl"

RWTexture2D<float4> Transmittance;

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height;
    Transmittance.GetDimensions(width, height);
    if(threadIdx.x >= width || threadIdx.y >= height)
        return;

    float theta = asin(lerp(-1.0, 1.0, (threadIdx.y + 0.5) / height));
    float h = lerp(
        0.0, AtmosphereRadius - PlanetRadius, (threadIdx.x + 0.5) / width);

    float2 o = float2(0, PlanetRadius + h);
    float2 d = float2(cos(theta), sin(theta));
    
    float t = 0;
    if(!findClosestIntersectionWithCircle(o, d, PlanetRadius, t))
        findClosestIntersectionWithCircle(o, d, AtmosphereRadius, t);

    float2 end = o + t * d;

    float3 sum;
    for(int i = 0; i < STEP_COUNT; ++i)
    {
        float2 pi = lerp(o, end, (i + 0.5) / STEP_COUNT);
        float hi = length(pi) - PlanetRadius;
        float3 sigma = getSigmaT(hi);
        sum += sigma;
    }

    float3 result = exp(-sum * (t / STEP_COUNT));
    Transmittance[threadIdx.xy] = float4(result, 1);
}
