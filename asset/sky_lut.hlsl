#include "./intersection.hlsl"
#include "./medium.hlsl"

Texture2D<float3> Transmittance;
Texture2D<float3> MultiScattering;

SamplerState MTSampler;

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
    float3 AtmosEyePosition;
    int    MarchStepCount;

    float3 SunDirection;
    int    EnableMultiScattering;

    float3 SunIntensity;
    float  SunTheta;
}

void marchStep(
    float phaseU, float3 ori, float3 dir, float thisT, float nextT,
    inout float3 sumSigmaT, inout float3 inScattering)
{
    float  midT = 0.5 * (thisT + nextT);
    float3 posR = float3(0, ori.y + PlanetRadius, 0) + dir * midT;
    float  h    = length(posR) - PlanetRadius;

    float3 sigmaS, sigmaT;
    getSigmaST(h, sigmaS, sigmaT);

    float3 deltaSumSigmaT = (nextT - thisT) * sigmaT;
    float3 eyeTrans = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

    if(!hasIntersectionWithSphere(
        posR, -SunDirection, PlanetRadius))
    {
        float3 rho = evalPhaseFunction(h, phaseU);
        float3 sunTrans = getTransmittance(
            Transmittance, MTSampler, h, SunTheta);

        inScattering += (nextT - thisT) * eyeTrans * sigmaS * rho * sunTrans;
    }

    if(EnableMultiScattering)
    {
        float tx = h / (AtmosphereRadius - PlanetRadius);
        float ty = 0.5 + 0.5 * sin(SunTheta);
        float3 ms = MultiScattering.SampleLevel(
            MTSampler, float2(tx, ty), 0);

        inScattering += (nextT - thisT) * eyeTrans * sigmaS * ms;
    }

    sumSigmaT += deltaSumSigmaT;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float phi = 2 * PI * input.texCoord.x;
    float vm = 2 * input.texCoord.y - 1;
    float theta = sign(vm) * (PI / 2) * vm * vm;
    float sinTheta = sin(theta), cosTheta = cos(theta);

    float3 ori = AtmosEyePosition;
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

    // phase function input

    float phaseU = dot(SunDirection, -dir);

    // ray march

    float t = 0;
    float3 inScatter = float3(0, 0, 0);
    float3 sumSigmaT = float3(0, 0, 0);

    float dt = (endT - t) / MarchStepCount;
    for(int i = 0; i < MarchStepCount; ++i)
    {
        float nextT = t + dt;
        marchStep(phaseU, ori, dir, t, nextT, sumSigmaT, inScatter);
        t = nextT;
    }

    return float4(inScatter * SunIntensity, 1);
}
