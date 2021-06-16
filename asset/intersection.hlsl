#ifndef INTERSECTION_HLSL
#define INTERSECTION_HLSL

bool hasIntersectionWithCiecle(float2 o, float2 d, float R)
{
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    return (delta >= 0) && ((C <= 0) | (B <= 0));
}

bool hasIntersectionWithSphere(float3 o, float3 d, float R)
{
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    return (delta >= 0) && ((C <= 0) | (B <= 0));
}

bool findClosestIntersectionWithCircle(
    float2 o, float2 d, float R, out float t)
{
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    if(delta < 0)
        return false;
    t = (-B + (C <= 0 ? sqrt(delta) : -sqrt(delta))) / (2 * A);
    return (C <= 0) | (B <= 0);
}

bool findClosestIntersectionWithSphere(
    float3 o, float3 d, float R, out float t)
{
    float A = dot(d, d);
    float B = 2 * dot(o, d);
    float C = dot(o, o) - R * R;
    float delta = B * B - 4 * A * C;
    if(delta < 0)
        return false;
    t = (-B + (C <= 0 ? sqrt(delta) : -sqrt(delta))) / (2 * A);
    return (C <= 0) | (B <= 0);
}

#endif // #ifndef INTERSECTION_HLSL
