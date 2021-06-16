#pragma once

#include "./common.h"

class ShadowMap
{
public:

    void initialize(const Int2 &res);

    ComPtr<ID3D11ShaderResourceView> getShadowMap() const;

    void setLight(const Mat4 &lightViewProj);

    void begin();

    void end();

    void render(const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world);

private:

    struct VSTransform
    {
        Mat4 WVP;
    };

    Shader<VS, PS>         shader_;
    Shader<VS, PS>::RscMgr shaderRscs_;

    ComPtr<ID3D11InputLayout> inputLayout_;
    RenderTarget              renderTarget_;

    Mat4                        viewProj_;
    ConstantBuffer<VSTransform> vsTransform_;
};
