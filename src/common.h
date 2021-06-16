#pragma once

#include <agz-utils/graphics_api.h>

constexpr float PI = agz::math::PI_f;

using namespace agz::d3d11;

struct Vertex
{
    Float3 position;
    Float3 normal;
};
