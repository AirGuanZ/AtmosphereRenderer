#define THREAD_GROUP_SIZE_X 16
#define THREAD_GROUP_SIZE_Y 16

#include "./intersection.hlsl"
#include "./medium.hlsl"

cbuffer CSParams
{
    float3 TerrainAlbedo;
    int DirSampleCount;

    float3 SunIntensity;
    int RayMarchStepCount;
}

StructuredBuffer<float2> RawDirSamples;

RWTexture2D<float4> MultiScattering;
Texture2D<float3>   Transmittance;

SamplerState TransmittanceSampler;

float3 uniformOnUnitSphere(float u1, float u2)
{
    float z   = 1 - 2 * u1;
    float r   = sqrt(max(0, 1 - z * z));
    float phi = 2 * PI * u2;
    return float3(r * cos(phi), r * sin(phi), z);
}

void integrate(
    float3 worldOri, float3 worldDir, float sunTheta, float3 toSunDir,
    out float3 innerL2, out float3 innerF)
{
    float u = dot(worldDir, toSunDir);

    float endT = 0;
    bool groundInct = findClosestIntersectionWithSphere(
        worldOri, worldDir, PlanetRadius, endT);
    if(!groundInct)
    {
        findClosestIntersectionWithSphere(
            worldOri, worldDir, AtmosphereRadius, endT);
    }

    float dt = endT / RayMarchStepCount;
    float halfDt = 0.5 * dt;
    float t = 0;

    float3 sumSigmaT = float3(0, 0, 0);

    float3 sumL2 = float3(0, 0, 0), sumF = float3(0, 0, 0);
    for(int i = 0; i < RayMarchStepCount; ++i)
    {
        float midT = t + halfDt;
        t += dt;

        float3 worldPos = worldOri + midT * worldDir;
        float h = length(worldPos) - PlanetRadius;

        float3 sigmaS, sigmaT;
        getSigmaST(h, sigmaS, sigmaT);

        float3 deltaSumSigmaT = dt * sigmaT;
        float3 transmittance = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

        if(!hasIntersectionWithSphere(worldPos, toSunDir, PlanetRadius))
        {
            float3 rho = evalPhaseFunction(h, u);
            float3 sunTransmittance = getTransmittance(
                Transmittance, TransmittanceSampler, h, sunTheta);

            sumL2 += dt * transmittance * sunTransmittance * sigmaS *
                     rho * SunIntensity;
        }

        sumF      += dt * transmittance * sigmaS;
        sumSigmaT += deltaSumSigmaT;
    }

    if(groundInct)
    {
        float3 transmittance = exp(-sumSigmaT);
        float3 sunTransmittance = getTransmittance(
            Transmittance, TransmittanceSampler, 0, sunTheta);
        sumL2 += transmittance * sunTransmittance * max(0, toSunDir.y) *
                 SunIntensity * (TerrainAlbedo / PI);
    }

    innerL2 = sumL2;
    innerF  = sumF;
}

float3 computeM(float h, float sunTheta)
{
    float3 worldOri = { 0, h + PlanetRadius, 0 };
    float3 toSunDir = { cos(sunTheta), sin(sunTheta), 0 };

    float3 sumL2 = float3(0, 0, 0), sumF = float3(0, 0, 0);
    for(int i = 0; i < DirSampleCount; ++i)
    {
        float2 rawSample = RawDirSamples[i];
        float3 worldDir = uniformOnUnitSphere(rawSample.x, rawSample.y);

        float3 innerL2, innerF;
        integrate(worldOri, worldDir, sunTheta, toSunDir, innerL2, innerF);

        // phase function is canceled by pdf

        sumL2 += innerL2;
        sumF  += innerF;
    }

    float3 l2 = sumL2 / DirSampleCount;
    float3 f  = sumF  / DirSampleCount;
    return l2 / (1 - f);
}

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, 1)]
void CSMain(int3 threadIdx : SV_DispatchThreadID)
{
    int width, height;
    MultiScattering.GetDimensions(width, height);
    if(threadIdx.x >= width || threadIdx.y >= height)
        return;

    float sinSunTheta = lerp(-1, 1, (threadIdx.y + 0.5) / height);
    float sunTheta = asin(sinSunTheta);

    float h = lerp(
        0.0, AtmosphereRadius - PlanetRadius, (threadIdx.x + 0.5) / width);

    MultiScattering[threadIdx.xy] = float4(computeM(h, sunTheta), 1);
}
