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

    float u        = dot(SunDirection, -dir);
    float sunTheta = asin(-SunDirection.y);

    // high res ray marching

    float t = 0;
    float3 inScatter = float3(0, 0, 0);

    float3 accumTransmittance = float3(0, 0, 0);

    for(int i = 0; i < HighResMarchStepCount && t < endT; ++i)
    {
        float nextT = min(t + HighResMarchStep, endT);
        marchStepInAtmosphere(
            MultiScattering, Transmittance, TransmittanceSampler,
            EnableMultiScattering, SunDirection, sunTheta, u, ori, dir,
            t, nextT, accumTransmittance, inScatter);
        t = nextT;
    }

    // lwo res ray marching

    if(t < endT)
    {
        float dt = (endT - t) / LowResMarchStepCount;
        for(int i = 0; i < LowResMarchStepCount; ++i)
        {
            float nextT = t + dt;
            marchStepInAtmosphere(
                MultiScattering, Transmittance, TransmittanceSampler,
                EnableMultiScattering, SunDirection, sunTheta, u, ori, dir,
                t, nextT, accumTransmittance, inScatter);
            t = nextT;
        }
    }

    return float4(inScatter * SunIntensity, 1);
}
