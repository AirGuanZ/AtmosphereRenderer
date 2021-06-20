#include "./dither.hlsl"
#include "./medium.hlsl"

cbuffer VSTransform
{
    float4x4 WVP;
    float4x4 World;
}

struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float3 color    : COLOR;
};

struct VSOutput
{
    float4 position       : SV_POSITION;
    float4 screenPosition : SCREEN_POSITION;
    float3 worldPosition  : WORLD_POSITION;
    float3 worldNormal    : WORLD_NORMAL;
    float3 color          : COLOR;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    output.position       = mul(float4(input.position, 1), WVP);
    output.screenPosition = output.position;
    output.worldPosition  = mul(float4(input.position, 1), World);
    output.worldNormal    = normalize(mul(float4(input.normal, 0), World).xyz);
    output.color          = input.color;
    return output;
}

cbuffer PSParams
{
    float3   SunDirection;   float SunTheta;
    float3   SunIntensity;   float MaxAerialDistance;
    float3   EyePos;         float WorldScale;
    float4x4 ShadowViewProj;
    float2   JitterFactor;   float2 BlueNoiseUVFactor;
}

Texture2D<float3> Transmittance;
Texture3D<float4> AerialPerspective;
SamplerState      TASampler;

Texture2D<float> ShadowMap;
SamplerState     ShadowSampler;

Texture2D<float2> BlueNoise;
SamplerState      BlueNoiseSampler;

float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 position = input.worldPosition;
    float3 normal = normalize(input.worldNormal);

    float2 scrPos = input.screenPosition.xy / input.screenPosition.w;
    scrPos = 0.5 + float2(0.5, -0.5) * scrPos;
    
    float2 bnUV   = scrPos * BlueNoiseUVFactor;
    float2 bn     = BlueNoise.SampleLevel(BlueNoiseSampler, bnUV, 0);
    float2 offset =
        JitterFactor * bn.x * float2(cos(2 * PI * bn.y), sin(2 * PI * bn.y));
    
    float apZ = WorldScale * distance(position, EyePos) / MaxAerialDistance;
    float4 ap = AerialPerspective.SampleLevel(
        TASampler, float3(scrPos + offset, saturate(apZ)), 0);
    float3 inScatter = ap.xyz;

    float eyeTrans = ap.w;
    float3 sunTrans = getTransmittance(
        Transmittance, TASampler, WorldScale * position.y, SunTheta);
    float3 sunRadiance = input.color * max(0, dot(normal, -SunDirection));

    float4 shadowClip = mul(float4(position + 0.03 * normal, 1), ShadowViewProj);
    float2 shadowNDC = shadowClip.xy / shadowClip.w;
    float2 shadowUV = 0.5 + float2(0.5, -0.5) * shadowNDC;

    float shadowFactor = 1;
    if(all(saturate(shadowUV) == shadowUV))
    {
        float shadedDepth = shadowClip.z;
        float sampledDepth = ShadowMap.SampleLevel(ShadowSampler, shadowUV, 0);
        shadowFactor = shadedDepth <= sampledDepth;
    }

    float3 result = SunIntensity *
        (shadowFactor * sunRadiance * sunTrans * eyeTrans + inScatter);
    
    result = avoidColorBanding(scrPos, pow(result, 1 / 2.2));
    return float4(result, 1);
}
