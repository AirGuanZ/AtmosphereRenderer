#pragma once

#include "./common.h"

struct AtmosphereProperties
{
    Float3 scatterRayleigh  = { 5.802e-6f, 13.558e-6f, 33.1e-6f };
    float  hDensityRayleigh = 8'000;

    float scatterMie   = 3.996e-6f;
    float asymmetryMie = 0.8f;
    float absorbMie    = 4.40e-6f;
    float hDensityMie  = 1'200;

    Float3 absorbOzone       = { 0.650e-6f, 1.881e-6f, 0.085e-6f };
    float  ozoneCenterHeight = 25'000;

    float  ozoneThickness   = 30'000;
    float  planetRadius     = 6360'000;
    float  atmosphereRadius = 6460'000;
    float  pad0             = 0;

    Float3 getSigmaS(float h) const;

    Float3 getSigmaT(float h) const;

    Float3 evalPhaseFunction(float h, float u) const;
};
