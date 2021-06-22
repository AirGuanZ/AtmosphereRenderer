#pragma once

#include "./medium.h"

class SkyLUT
{
public:

    void initialize(const Int2 &res);

    void resize(const Int2 &res);
    
    ComPtr<ID3D11ShaderResourceView> getLUT() const;

    void setCamera(const Float3 &atmosEyePos);

    void setRayMarching(int stepCount);

    void setAtmosphere(const AtmosphereProperties &atmos);

    void setSun(const Float3 &direction, const Float3 &intensity);

    void setTransmittance(ComPtr<ID3D11ShaderResourceView> T);

    void setMultiScattering(bool enabled, ComPtr<ID3D11ShaderResourceView> M);

    void generate();

private:

    struct PSParams
    {
        Float3 atmosEyePosition;
        int    lowResMarchStepCount;

        Float3 sunDirection;
        int    enableMultiScattering;

        Float3 sunIntensity;
        float  pad0;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;
    
    ShaderResourceViewSlot<PS> *transmittanceSlot_ = nullptr;
    ShaderResourceViewSlot<PS> *multiScatterSlot_  = nullptr;

    RenderTarget LUT_;

    PSParams psParamsData_ = {};

    ConstantBuffer<AtmosphereProperties> psAtmos_;
    ConstantBuffer<PSParams>             psParams_;
};
