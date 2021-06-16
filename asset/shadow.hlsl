cbuffer VSTransform
{
    float4x4 WVP;
}

float4 VSMain(float3 position : POSITION) : SV_POSITION
{
    return mul(float4(position, 1), WVP);
}

void PSMain(float4 position : SV_POSITION)
{
    
}
