#ifndef MEDIUM_HLSL
#define MEDIUM_HLSL

#include "./intersection.hlsl"

cbuffer AtmosphereParams
{
    float3 ScatterRayleigh;
    float  HDensityRayleigh;

    float ScatterMie;
    float AsymmetryMie;
    float AbsorbMie;
    float HDensityMie;

    float3 AbsorbOzone;
    float  OzoneCenterHeight;

    float OzoneThickness;
    float PlanetRadius;
    float AtmosphereRadius;
}

float3 getSigmaS(float h)
{
    float3 rayleigh = ScatterRayleigh * exp(-h / HDensityRayleigh);
    float mie = ScatterMie * exp(-h / HDensityMie);
    return rayleigh + mie;
}

float3 getSigmaT(float h)
{
    float3 rayleigh = ScatterRayleigh * exp(-h / HDensityRayleigh);
    float mie = (ScatterMie + AbsorbMie) * exp(-h / HDensityMie);
    float3 ozone = AbsorbOzone * max(
        0.0f, 1 - 0.5 * abs(h - OzoneCenterHeight) / OzoneThickness);
    return rayleigh + mie + ozone;
}

void getSigmaST(float h, out float3 sigmaS, out float3 sigmaT)
{
    float3 rayleigh = ScatterRayleigh * exp(-h / HDensityRayleigh);

    float mieDensity = exp(-h / HDensityMie);
    float mieS = ScatterMie * mieDensity;
    float mieT = (ScatterMie + AbsorbMie) * mieDensity;

    float3 ozone = AbsorbOzone * max(
        0.0f, 1 - 0.5 * abs(h - OzoneCenterHeight) / OzoneThickness);

    sigmaS = rayleigh + mieS;
    sigmaT = rayleigh + mieT + ozone;
}

float3 evalPhaseFunction(float h, float u)
{
    float3 sRayleigh = ScatterRayleigh * exp(-h / HDensityRayleigh);
    float sMie = ScatterMie * exp(-h / HDensityMie);
    float3 s = sRayleigh + sMie;

    float g = AsymmetryMie, g2 = g * g, u2 = u * u;
    float pRayleigh = 3 / (16 * PI) * (1 + u2);

    float m = 1 + g2 - 2 * g * u;
    float pMie = 3 / (8 * PI) * (1 - g2) * (1 + u2) / ((2 + g2) * m * sqrt(m));

    float3 result;
    result.x = s.x > 0 ? (pRayleigh * sRayleigh.x + pMie * sMie) / s.x : 0;
    result.y = s.y > 0 ? (pRayleigh * sRayleigh.y + pMie * sMie) / s.y : 0;
    result.z = s.z > 0 ? (pRayleigh * sRayleigh.z + pMie * sMie) / s.z : 0;
    return result;
}

float3 getTransmittance(
    Texture2D<float3> T, SamplerState S, float h, float theta)
{
    float u = h / (AtmosphereRadius - PlanetRadius);
    float v = 0.5 + 0.5 * sin(theta);
    return T.SampleLevel(S, float2(u, v), 0);
}

void marchStepInAtmosphere(
    Texture2D<float3> M,
    Texture2D<float3> T,
    SamplerState      MTSampler,
    bool              enableMultiScattering,
    float3            sunDirection,
    float             sunTheta,
    float             u,
    float3            worldOri,
    float3            worldDir,
    float             thisT,
    float             nextT,
    inout float3      sumSigmaT,
    inout float3      inScattering)
{
    float3 worldOriR = float3(0, worldOri.y + PlanetRadius, 0);

    float midT = 0.5 * (thisT + nextT);
    float h = length(worldOriR + worldDir * midT) - PlanetRadius;

    float3 sigmaS, sigmaT;
    getSigmaST(h, sigmaS, sigmaT);

    float3 deltaSumSigmaT = (nextT - thisT) * sigmaT;
    float3 transmittance = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

    if(!hasIntersectionWithSphere(
        worldOriR + worldDir * midT,
        -sunDirection,
        PlanetRadius))
    {
        float3 rho = evalPhaseFunction(h, u);
        float3 sunTransmittance = getTransmittance(
            T, MTSampler, h, sunTheta);
        inScattering +=
            (nextT - thisT) * transmittance * sigmaS * rho * sunTransmittance;
    }

    if(enableMultiScattering)
    {
        float tx = h / (AtmosphereRadius - PlanetRadius);
        float ty = 0.5 + 0.5 * sin(sunTheta);
        float3 ms = M.SampleLevel(
            MTSampler, float2(tx, ty), 0);
        inScattering += (nextT - thisT) * transmittance * sigmaS * ms;
    }

    sumSigmaT += deltaSumSigmaT;
}

void shadowedMarchStepInAtmosphere(
    Texture2D<float>  shadowMap,
    SamplerState      shadowSampler,
    float4x4          shadowViewProj,
    Texture2D<float3> M,
    Texture2D<float3> T,
    SamplerState      MTSampler,
    bool              enableMultiScattering,
    float3            sunDirection,
    float             sunTheta,
    float             u,
    float3            shadowOri,
    float3            worldOri,
    float3            worldDir,
    float             worldScale,
    float             thisT,
    float             nextT,
    inout float3      sumSigmaT,
    inout float3      inScattering)
{
    float3 worldOriR = float3(0, worldOri.y + PlanetRadius, 0);

    float midT = 0.5 * (thisT + nextT);
    float h = length(worldOriR + worldDir * midT) - PlanetRadius;

    float3 sigmaS, sigmaT;
    getSigmaST(h, sigmaS, sigmaT);

    float3 deltaSumSigmaT = (nextT - thisT) * sigmaT;
    float3 transmittance = exp(-sumSigmaT - 0.5 * deltaSumSigmaT);

    if(!hasIntersectionWithSphere(
        worldOriR + worldDir * midT,
        -sunDirection,
        PlanetRadius))
    {
        float3 shadowPos = shadowOri + worldDir * midT / worldScale;
        float4 shadowClip = mul(float4(shadowPos, 1), shadowViewProj);
        float2 shadowNDC = shadowClip.xy / shadowClip.w;
        float2 shadowUV = 0.5 + float2(0.5, -0.5) * shadowNDC;

        bool inShadow = false;
        if(all(saturate(shadowUV) == shadowUV))
        {
            float midShadowZ = shadowClip.z;
            float sampledZ = shadowMap.SampleLevel(shadowSampler, shadowUV, 0);
            inShadow = midShadowZ >= sampledZ;
        }

        if(!inShadow)
        {
            float3 rho = evalPhaseFunction(h, u);
            float3 sunTransmittance = getTransmittance(
                T, MTSampler, h, sunTheta);
            inScattering +=
                (nextT - thisT) * transmittance *
                sigmaS * rho * sunTransmittance;
        }
    }

    if(enableMultiScattering)
    {
        float tx = h / (AtmosphereRadius - PlanetRadius);
        float ty = 0.5 + 0.5 * sin(sunTheta);
        float3 ms = M.SampleLevel(
            MTSampler, float2(tx, ty), 0);
        inScattering += (nextT - thisT) * transmittance * sigmaS * ms;
    }

    sumSigmaT += deltaSumSigmaT;
}

#endif // #ifndef MEDIUM_HLSL
