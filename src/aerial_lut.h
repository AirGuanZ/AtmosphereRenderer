#pragma once

#include "./camera.h"
#include "./medium.h"

class AerialPerspectiveLUT_
{
public:

    void initialize(const Int3 &res);

    void resize(const Int3 &res);

    void setCamera(
        const Float3                    &shadowEyePos,
        float                            eyePosY,
        const Camera::FrustumDirections &frustumDirs);

    void setSun(const Float3 &sunDirection);

    void setParams(
        float worldScale,
        bool  enableMultiScattering,
        bool  enableShadow,
        float maxDistance,
        int   perSliceMarchStepCount);

    ComPtr<ID3D11ShaderResourceView> getOutput() const;

    void render(
        const Mat4                      &shadowViewProj,
        ComPtr<ID3D11ShaderResourceView> shadowMap,
        const AtmosphereProperties      &atmos,
        ComPtr<ID3D11ShaderResourceView> multiScatter,
        ComPtr<ID3D11ShaderResourceView> transmittance);

private:

    struct CSParams
    {
        Float3 sunDirection;      float sunTheta;
        Float3 frustumA;          float maxDistance;
        Float3 frustumB;          int   perSliceStepCount;
        Float3 frustumC;          int   enableMultiScattering;
        Float3 frustumD;          float eyePositionY;
        Float3 shadowEyePosition; int   enableShadow;
        Mat4   shadowViewProj;
        float worldScale;
        float pad0;
        float pad1;
        float pad2;
    };

    Shader<CS>         shader_;
    Shader<CS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<CS> *shadowMapSlot_     = nullptr;
    ShaderResourceViewSlot<CS> *multiScatterSlot_  = nullptr;
    ShaderResourceViewSlot<CS> *transmittanceSlot_ = nullptr;

    Int3                              res_;
    ComPtr<ID3D11ShaderResourceView>  srv_;
    ComPtr<ID3D11UnorderedAccessView> uav_;

    CSParams                 csParamsData_ = {};
    ConstantBuffer<CSParams> csParams_;

    ConstantBuffer<AtmosphereProperties> atmos_;
};
