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

SamplerState SkyViewSampler;

float4 PSMain(VSOutput input) : SV_TARGET
{
    float3 dir = normalize(lerp(
        lerp(FrustumA, FrustumB, input.texCoord.x),
        lerp(FrustumC, FrustumD, input.texCoord.x), input.texCoord.y));

    float phi = !dir.z && !dir.x ? 0 : atan2(dir.z, dir.x);
    float u = phi / (2 * PI);
    
    float theta = asin(dir.y);
    float v = max(0.5 * (1 + sqrt(theta / (PI / 2))), 0.5);

    float3 skyColor = SkyView.SampleLevel(SkyViewSampler, float2(u, v), 0);

    float rand = frac(sin(dot(
        input.texCoord, float2(12.9898, 78.233) * 2.0)) * 43758.5453);
    skyColor = 255 * saturate(pow(skyColor, 1 / 2.2));
    skyColor = rand.xxx < (skyColor - floor(skyColor)) ? ceil(skyColor) : floor(skyColor);
    skyColor *= 1.0 / 255;

    return float4(skyColor, 1);
}
