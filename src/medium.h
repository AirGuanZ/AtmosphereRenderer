#pragma once

#include "./common.h"

struct AtmosphereProperties
{
    Float3 scatterRayleigh  = { 5.802f, 13.558f, 33.1f };
    float  hDensityRayleigh = 8;

    float scatterMie   = 3.996f;
    float asymmetryMie = 0.8f;
    float absorbMie    = 4.4f;
    float hDensityMie  = 1.2f;

    Float3 absorbOzone       = { 0.65f, 1.881f, 0.085f };
    float  ozoneCenterHeight = 25;

    float  ozoneThickness   = 30;
    float  planetRadius     = 6360;
    float  atmosphereRadius = 6460;
    float  pad0             = 0;

    AtmosphereProperties toStdUnit() const;

    Float3 getSigmaS(float h) const;

    Float3 getSigmaT(float h) const;

    Float3 evalPhaseFunction(float h, float u) const;
};
