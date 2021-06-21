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
    float3 FrustumD;          float AtmosEyeHeight;
    float3 EyePosition;       int   EnableShadow;
    float4x4 ShadowViewProj;  float WorldScale;
}

Texture2D<float3> MultiScattering;
Texture2D<float3> Transmittance;
SamplerState      MTSampler;

Texture2D<float> ShadowMap;
SamplerState     ShadowSampler;

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

    float3 ori = float3(0, AtmosEyeHeight, 0);
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

    float rand = frac(sin(dot(
        float2(xf, yf), float2(12.9898, 78.233) * 2.0)) * 43758.5453);

    for(int z = 0; z < depth; ++z)
    {
        float dt = (tEnd - tBeg) / PerSliceMarchStepCount;
        float t = tBeg;

        for(int i = 0; i < PerSliceMarchStepCount; ++i)
        {
            float nextT = t + dt;

            float  midT = lerp(t, nextT, rand);
            float3 posR = float3(0, ori.y + PlanetRadius, 0) + dir * midT;
            float  h    = length(posR) - PlanetRadius;

            float sunTheta = PI / 2 - acos(dot(-SunDirection, normalize(posR)));

            float3 sigmaS, sigmaT;
            getSigmaST(h, sigmaS, sigmaT);

            float3 deltaSumSigmaT = dt * sigmaT;
            float3 eyeTrans = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

            if(!hasIntersectionWithSphere(posR, -SunDirection, PlanetRadius))
            {
                float3 shadowPos  = EyePosition + dir * midT / WorldScale;
                float4 shadowClip = mul(float4(shadowPos, 1), ShadowViewProj);
                float2 shadowNDC  = shadowClip.xy / shadowClip.w;
                float2 shadowUV   = 0.5 + float2(0.5, -0.5) * shadowNDC;

                bool inShadow = EnableShadow;
                if(EnableShadow && all(saturate(shadowUV) == shadowUV))
                {
                    float rayZ = shadowClip.z;
                    float smZ = ShadowMap.SampleLevel(ShadowSampler, shadowUV, 0);
                    inShadow = rayZ >= smZ;
                }

                if(!inShadow)
                {
                    float3 rho = evalPhaseFunction(h, u);
                    float3 sunTrans = getTransmittance(
                        Transmittance, MTSampler, h, sunTheta);
                    inScatter += dt * eyeTrans * sigmaS * rho * sunTrans;
                }
            }

            if(EnableMultiScattering)
            {
                float tx = h / (AtmosphereRadius - PlanetRadius);
                float ty = 0.5 + 0.5 * sin(sunTheta);
                float3 ms = MultiScattering.SampleLevel(
                    MTSampler, float2(tx, ty), 0);
                inScatter += dt * eyeTrans * sigmaS * ms;
            }

            sumSigmaT += deltaSumSigmaT;
            t = nextT;
        }

        float transmittance = relativeLuminance(exp(-sumSigmaT));
        AerialPerspectiveLUT[int3(threadIdx.xy, z)] =
            float4(inScatter, transmittance);

        tBeg = tEnd;
        tEnd = min(tEnd + sliceDepth, maxT);
    }
}
