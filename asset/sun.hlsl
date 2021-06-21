#include "./postcolor.hlsl"
#include "./medium.hlsl"

cbuffer VSTransform
{
    float4x4 WVP;

    float sunTheta;
    float eyeHeight;
}

Texture2D<float3> Transmittance;
SamplerState      TransmittanceSampler;

struct VSOutput
{
    float4 position      : SV_POSITION;
    float4 clipPos       : CLIP_POSITION;
    float3 transmittance : TRANSMITTANCE;
};

VSOutput VSMain(float2 position : POSITION)
{
    VSOutput output;
    output.position = mul(float4(position, 0, 1), WVP);
    output.position.z = output.position.w;
    output.clipPos  = output.position;
    output.transmittance = getTransmittance(
        Transmittance, TransmittanceSampler, eyeHeight, sunTheta);
    return output;
}

cbuffer PSParams
{
    float3 SunRadiance;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    return float4(postProcessColor(
        input.clipPos.xy / input.clipPos.w,
        input.transmittance * SunRadiance), 1);
}
