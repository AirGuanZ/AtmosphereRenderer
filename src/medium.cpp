#include "./medium.h"

AtmosphereProperties AtmosphereProperties::toStdUnit() const
{
    AtmosphereProperties ret = *this;
    ret.scatterRayleigh   = 1e-6f * ret.scatterRayleigh;
    ret.hDensityRayleigh  = 1e3f  * ret.hDensityRayleigh;
    ret.scatterMie        = 1e-6f * ret.scatterMie;
    ret.absorbMie         = 1e-6f * ret.absorbMie;
    ret.hDensityMie       = 1e3f  * ret.hDensityMie;
    ret.absorbOzone       = 1e-6f * ret.absorbOzone;
    ret.ozoneCenterHeight = 1e3f  * ret.ozoneCenterHeight;
    ret.ozoneThickness    = 1e3f  * ret.ozoneThickness;
    ret.planetRadius      = 1e3f  * ret.planetRadius;
    ret.atmosphereRadius  = 1e3f  * ret.atmosphereRadius;
    return ret;
}

Float3 AtmosphereProperties::getSigmaS(float h) const
{
    const Float3 rayleigh = scatterRayleigh * std::exp(-h / hDensityRayleigh);
    const Float3 mie      = Float3(scatterMie * std::exp(-h / hDensityMie));
    return rayleigh + mie;
}

Float3 AtmosphereProperties::getSigmaT(float h) const
{
    const Float3 rayleigh = scatterRayleigh * std::exp(-h / hDensityRayleigh);
    const float mie = (scatterMie + absorbMie) * std::exp(-h / hDensityMie);
    const float ozoneDensity = (std::max)(
        0.0f, 1 - 0.5f * std::abs(h - ozoneCenterHeight) / ozoneThickness);
    const Float3 ozone = absorbOzone * ozoneDensity;

    return rayleigh + mie + ozone;
}

Float3 AtmosphereProperties::evalPhaseFunction(float h, float u) const
{
    const Float3 sRayleigh = scatterRayleigh * std::exp(-h / hDensityRayleigh);
    const Float3 sMie      = Float3(scatterMie * std::exp(-h / hDensityMie));
    const Float3 s         = sRayleigh + sMie;

    const float g = asymmetryMie, g2 = g * g, u2 = u * u;
    const float pRayleigh = 3 / (16 * PI) * (1 + u2);

    const float m = 1 + g2 - 2 * g * u;
    const float pMie = 3 / (8 * PI) * (1 - g2) * (1 + u2)
                                    / ((2 + g2) * m * std::sqrt(m));

    Float3 result;
    for(int i = 0; i < 3; ++i)
    {
        if(s[i] > 0)
            result[i] = (pRayleigh * sRayleigh[i] + pMie * sMie[i]) / s[i];
    }

    return result;
}
