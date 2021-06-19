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
    float3 SunDirection; float SunTheta;
    float3 SunIntensity; float MaxAerialDistance;
    float3 EyePos;       float WorldScale;
}

Texture2D<float3> Transmittance;
Texture3D<float4> AerialPerspective;

SamplerState TASampler;

float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 position = input.worldPosition;
    float3 normal = normalize(input.worldNormal);

    float2 scrPos = input.screenPosition.xy / input.screenPosition.w;
    float apZ = WorldScale * distance(position, EyePos) / MaxAerialDistance;
    float4 ap = AerialPerspective.SampleLevel(
        TASampler,
        float3(0.5 + float2(0.5, -0.5) * scrPos, saturate(apZ)), 0);
    float3 inScatter = ap.xyz;

    float eyeTrans = ap.w;
    float3 sunTrans = getTransmittance(
        Transmittance, TASampler, WorldScale * position.y, SunTheta);
    float3 sunRadiance = input.color * max(0, dot(normal, -SunDirection));

    float3 result =
        SunIntensity * (sunRadiance * sunTrans * eyeTrans + inScatter);
    return float4(pow(result, 1 / 2.2), 1);
}
