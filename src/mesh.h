#pragma once

#include "./medium.h"

class MeshRenderer
{
public:

    void initialize();

    void setAtmosphere(
        const AtmosphereProperties      &atmos,
        ComPtr<ID3D11ShaderResourceView> trans,
        ComPtr<ID3D11ShaderResourceView> aerial,
        float                            maxAerialDistance);
    
    void setCamera(const Float3 &eye, const Mat4 &viewProj);

    void setLight(const Float3 &direction, const Float3 &intensity);

    void setWorldScale(float scale);

    void begin();

    void end();

    void render(const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world);

private:

    struct VSTransform
    {
        Mat4 WVP;
        Mat4 world;
    };

    struct PSParams
    {
        Float3 sunDirection; float sunTheta;
        Float3 sunIntensity; float maxAerialDistance;
        Float3 eyePos;       float worldScale;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ShaderResourceViewSlot<PS> *TSlot_ = nullptr;
    ShaderResourceViewSlot<PS> *ASlot_ = nullptr;

    ComPtr<ID3D11InputLayout> inputLayout_;

    Mat4                        viewProj_;
    ConstantBuffer<VSTransform> vsTransform_;

    ConstantBuffer<AtmosphereProperties> atmos_;

    PSParams                 psParamsData_ = {};
    ConstantBuffer<PSParams> psParams_;
};
