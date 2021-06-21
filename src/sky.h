#pragma once

#include "./camera.h"

class SkyRenderer
{
public:

    void initialize();

    void setCamera(const Camera::FrustumDirections &frustumDirs);

    void render(ComPtr<ID3D11ShaderResourceView> skyView);

private:

    struct PSTransform
    {
        Float3 frustumA; float pad0;
        Float3 frustumB; float pad1;
        Float3 frustumC; float pad2;
        Float3 frustumD; float pad3;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *skyViewSlot_ = nullptr;

    ComPtr<ID3D11DepthStencilState> dsState_;

    PSTransform                 psTransformData_ = {};
    ConstantBuffer<PSTransform> psTransform_;
};
