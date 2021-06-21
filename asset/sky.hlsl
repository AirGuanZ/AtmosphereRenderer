#include "./postcolor.hlsl"

#define PI 3.14159265

struct VSOutput
{
    float4 position       : SV_POSITION;
    float2 texCoord       : TEXCOORD;
};

VSOutput VSMain(uint vertexID : SV_VertexID)
{
    VSOutput output;
    output.texCoord = float2((vertexID << 1) & 2, vertexID & 2);
    output.position = float4(output.texCoord * float2(2, -2) + float2(-1, 1), 0.99999, 1);
    return output;
}

cbuffer PSTransform
{
    float3 FrustumA;
    float3 FrustumB;
    float3 FrustumC;
    float3 FrustumD;
}

Texture2D<float3> SkyView;
SamplerState      SkyViewSampler;

float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 dir = normalize(lerp(
        lerp(FrustumA, FrustumB, input.texCoord.x),
        lerp(FrustumC, FrustumD, input.texCoord.x), input.texCoord.y));

    float phi = atan2(dir.z, dir.x);
    float u = phi / (2 * PI);
    
    float theta = asin(dir.y);
    float v = 0.5 + 0.5 * sign(theta) * sqrt(abs(theta) / (PI / 2));

    float3 skyColor = SkyView.SampleLevel(SkyViewSampler, float2(u, v), 0);

    skyColor = postProcessColor(input.texCoord, skyColor);
    return float4(skyColor, 1);
}
