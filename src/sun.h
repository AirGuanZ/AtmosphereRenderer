#pragma once

#include "./medium.h"

class SunRenderer
{
public:

    void initialize();

    void setSunDiskSegments(int segCount);
    
    void setSun(
        float         radius,
        const Float3 &direction,
        const Float3 &intensity);

    void setWorldScale(float worldScale);

    void setCamera(const Float3 &eye, const Mat4 &viewProj);

    void setAtmosphere(const AtmosphereProperties &atmos);

    void setTransmittance(ComPtr<ID3D11ShaderResourceView> T);

    void render();

private:

    void updateConstantBuffers();

    struct VSTransform
    {
        Mat4  WVP;
        float sunTheta;
        float eyeHeight;
        float pad0;
        float pad1;
    };

    struct PSParams
    {
        Float3 radiance_;
        float  pad0;
    };

    VertexBuffer<Float2> sunDiskVertices_;

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<VS> *TSlot_ = nullptr;

    ComPtr<ID3D11InputLayout>       inputLayout_;
    ComPtr<ID3D11DepthStencilState> dsState_;

    float  radius_ = 0;
    Float3 direction_;
    Float3 intensity_;

    float  worldScale_;
    Float3 eye_;
    Mat4   viewProj_;

    ConstantBuffer<AtmosphereProperties> atmos_;
    ConstantBuffer<VSTransform>          vsTransform_;
    ConstantBuffer<PSParams>             psParams_;
};
