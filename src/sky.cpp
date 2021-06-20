#include "./sky.h"

void SkyRenderer::initialize()
{
    shader_.initializeStageFromFile<VS>("./asset/sky.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>("./asset/sky.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    skyViewSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("SkyView");

    psTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSTransform")
        ->setBuffer(psTransform_);

    auto skyViewSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("SkyViewSampler")
        ->setSampler(skyViewSampler);
}

void SkyRenderer::setCamera(const Camera::FrustumDirections &frustumDirs)
{
    psTransformData_.frustumA = frustumDirs.frustumA;
    psTransformData_.frustumB = frustumDirs.frustumB;
    psTransformData_.frustumC = frustumDirs.frustumC;
    psTransformData_.frustumD = frustumDirs.frustumD;
}

void SkyRenderer::render(ComPtr<ID3D11ShaderResourceView> skyView)
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
