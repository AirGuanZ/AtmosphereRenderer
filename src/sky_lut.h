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

    void enableMultiScattering(bool enable);

    void generate(
        const AtmosphereProperties      &atmos,
        const Float3                    &sunDirection,
        const Float3                    &sunIntensity,
        ComPtr<ID3D11ShaderResourceView> transmittance,
        ComPtr<ID3D11ShaderResourceView> multiscatter);

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
