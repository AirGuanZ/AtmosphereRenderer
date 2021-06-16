#include "./final.h"

void FinalRenderer::initialize()
{
    shader_.initializeStageFromFile<VS>("./asset/final.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>("./asset/final.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    skyViewSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("SkyView");

    psTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSTransform")
        ->setBuffer(psTransform_);

    auto skyViewSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_WRAP);
    shaderRscs_.getSamplerSlot<PS>("SkyViewSampler")
        ->setSampler(skyViewSampler);
}

void FinalRenderer::setCamera(const Mat4 &invViewProj)
{
    Float4 A0 = Float4(-1, 1, 0.2f, 1) * invViewProj;
    Float4 A1 = Float4(-1, 1, 0.5f, 1) * invViewProj;

    Float4 B0 = Float4(1, 1, 0.2f, 1) * invViewProj;
    Float4 B1 = Float4(1, 1, 0.5f, 1) * invViewProj;

    Float4 C0 = Float4(-1, -1, 0.2f, 1) * invViewProj;
    Float4 C1 = Float4(-1, -1, 0.5f, 1) * invViewProj;

    Float4 D0 = Float4(1, -1, 0.2f, 1) * invViewProj;
    Float4 D1 = Float4(1, -1, 0.5f, 1) * invViewProj;

    psTransformData_.frustumA = (A1.xyz() / A1.w - A0.xyz() / A0.w).normalize();
    psTransformData_.frustumB = (B1.xyz() / B1.w - B0.xyz() / B0.w).normalize();
    psTransformData_.frustumC = (C1.xyz() / C1.w - C0.xyz() / C0.w).normalize();
    psTransformData_.frustumD = (D1.xyz() / D1.w - D0.xyz() / D0.w).normalize();
}

void FinalRenderer::render(ComPtr<ID3D11ShaderResourceView> skyView)
{
    skyViewSlot_->setShaderResourceView(std::move(skyView));
    psTransform_.update(psTransformData_);

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext.draw(6, 0);

    shaderRscs_.unbind();
    shader_.unbind();
}
