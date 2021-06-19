#define THREAD_GROUP_SIZE_X 16
#define THREAD_GROUP_SIZE_Y 16

#include "./intersection.hlsl"
#include "./medium.hlsl"

cbuffer CSParams
{
    float3 SunDirection;      float SunTheta;
    float3 FrustumA;          float MaxDistance;
    float3 FrustumB;          int   PerSliceMarchStepCount;
    float3 FrustumC;          int   EnableMultiScattering;
    float3 FrustumD;          float EyePositionY;
    float3 ShadowEyePosition; int   EnableShadow;
    float4x4 ShadowViewProj;  float WorldScale;
}

Texture2D<float3> MultiScattering;
Texture2D<float3> Transmittance;
SamplerState TransmittanceSampler;

Texture2D<float> ShadowMap;
SamplerState ShadowSampler;

RWTexture3D<float4> AerialPerspectiveLUT;

float relativeLuminance(float3 c)
{
    return 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b;
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height, depth;
    AerialPerspectiveLUT.GetDimensions(width, height, depth);
    if(threadIdx.x >= width || threadIdx.y >= height)
        return;

    float xf = (threadIdx.x + 0.5) / width;
    float yf = (threadIdx.y + 0.5) / height;

    float3 ori = float3(0, EyePositionY, 0);
    float3 dir = normalize(lerp(
        lerp(FrustumA, FrustumB, xf), lerp(FrustumC, FrustumD, xf), yf));

    float u = dot(SunDirection, -dir);

    float maxT = 0;
    if(!findClosestIntersectionWithSphere(
        ori + float3(0, PlanetRadius, 0), dir, PlanetRadius, maxT))
    {
        findClosestIntersectionWithSphere(
            ori + float3(0, PlanetRadius, 0), dir, AtmosphereRadius, maxT);
    }

    float sliceDepth = MaxDistance / depth;
    float halfSliceDepth = 0.5 * sliceDepth;
    float tBeg = 0, tEnd = min(halfSliceDepth, maxT);

    float3 sumSigmaT = float3(0, 0, 0);
    float3 inScatter = float3(0, 0, 0);

    for(int z = 0; z < depth; ++z)
    {
        float dt = (tEnd - tBeg) / PerSliceMarchStepCount;
        float t = tBeg;

        for(int i = 0; i < PerSliceMarchStepCount; ++i)
        {
            float nextT = t + dt;
            if(EnableShadow)
            {
                shadowedMarchStepInAtmosphere(
                    ShadowMap, ShadowSampler, ShadowViewProj,
                    MultiScattering, Transmittance, TransmittanceSampler,
                    EnableMultiScattering, SunDirection, SunTheta, u,
                    ShadowEyePosition, ori, dir,
                    WorldScale, t, nextT, sumSigmaT, inScatter);
            }
            else
            {
                marchStepInAtmosphere(
                    MultiScattering, Transmittance, TransmittanceSampler,
                    EnableMultiScattering, SunDirection, SunTheta, u,
                    ori, dir, t, nextT, sumSigmaT, inScatter);
            }
            t = nextT;
        }

        float transmittance = relativeLuminance(exp(-sumSigmaT));
        AerialPerspectiveLUT[int3(threadIdx.xy, z)] =
            float4(inScatter, transmittance);

        tBeg = tEnd;
        tEnd = min(tEnd + sliceDepth, maxT);
    }
}
