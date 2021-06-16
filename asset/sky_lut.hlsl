#include "./intersection.hlsl"
#include "./medium.hlsl"

Texture2D<float3> Transmittance;
Texture2D<float3> MultiScattering;

SamplerState TransmittanceSampler;

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texCoord * float2(2, -2) + float2(-1, 1), 0.5, 1);
    return output;
}

cbuffer PSParams
{
    float3 EyePosition;
    int    LowResMarchStepCount;

    float3 SunDirection;
    float  HighResMarchStep;

    float3 SunIntensity;
    int HighResMarchStepCount;

    int EnableMultiScattering;
}

void marchStep(
    float        sunTheta,
    float        u,
    float3       worldOri,
    float3       worldDir,
    float2       planetOri,
    float2       planetDir,
    float        thisT,
    float        nextT,
    inout float3 sumSigmaT,
    inout float3 sum)
{
    float midT = 0.5 * (thisT + nextT);

    float h = length(planetOri + planetDir * midT) - PlanetRadius;
    float3 sigmaT = getSigmaT(h);
    float3 sigmaS = getSigmaS(h);

    float3 deltaSumSigmaT = (nextT - thisT) * sigmaT;
    float3 transmittance = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);
    
    if(!hasIntersectionWithSphere(
        float3(0, worldOri.y + PlanetRadius, 0) + worldDir * midT,
        -SunDirection,
        PlanetRadius))
    {
        float3 rho = evalPhaseFunction(h, u);
        float3 sunTransmittance = getTransmittance(
            Transmittance, TransmittanceSampler, h, sunTheta);
        sum += (nextT - thisT) * transmittance * sigmaS * rho * sunTransmittance;
    }

    if(EnableMultiScattering)
    {
        float tx = h / (AtmosphereRadius - PlanetRadius);
        float ty = 0.5 + 0.5 * sin(sunTheta);
        float3 ms = MultiScattering.SampleLevel(
            TransmittanceSampler, float2(tx, ty), 0);
        sum += (nextT - thisT) * transmittance * sigmaS * ms;
    }

    sumSigmaT += deltaSumSigmaT;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float phi = 2 * PI * input.texCoord.x;

    float vm = 2 * input.texCoord.y - 1;
    float theta = sign(vm) * (PI / 2) * vm * vm;

    float sinTheta = sin(theta), cosTheta = cos(theta);

    float3 ori = EyePosition;
    float3 dir = float3(cos(phi) * cosTheta, sinTheta, sin(phi) * cosTheta);

    float2 planetOri = float2(0, ori.y + PlanetRadius);
    float2 planetDir = float2(cosTheta, sinTheta);

    // find end point

    float endT = 0;
    if(!findClosestIntersectionWithCircle(
        planetOri, planetDir, PlanetRadius, endT))
    {
        findClosestIntersectionWithCircle(
            planetOri, planetDir, AtmosphereRadius, endT);
    }

    // phase function input of single scattering

    float u = dot(SunDirection, -dir);

    float sunTheta = asin(-SunDirection.y);

    // high res ray marching

    float t = 0;
    float3 sum = float3(0, 0, 0);

    float3 accumTransmittance = float3(0, 0, 0);

    for(int i = 0; i < HighResMarchStepCount && t < endT; ++i)
    {
        float nextT = min(t + HighResMarchStep, endT);
        marchStep(
            sunTheta, u, ori, dir, planetOri, planetDir,
            t, nextT, accumTransmittance, sum);
        t = nextT;
    }

    // lwo res ray marching

    if(t < endT)
    {
        float dt = (endT - t) / LowResMarchStepCount;
        for(int i = 0; i < LowResMarchStepCount; ++i)
        {
            float nextT = t + dt;
            marchStep(
                sunTheta, u, ori, dir, planetOri, planetDir,
                t, nextT, accumTransmittance, sum);
            t = nextT;
        }
    }

    return float4(sum * SunIntensity, 1);
}
