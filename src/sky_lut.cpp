#include "./sky_lut.h"

void SkyLUT::initialize(const Int2 &res)
{
    shader_.initializeStageFromFile<VS>(
        "./asset/sky_lut.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/sky_lut.hlsl", nullptr, "PSMain");
    
    shaderRscs_ = shader_.createResourceManager();

    transmittanceSlot_ =
        shaderRscs_.getShaderResourceViewSlot<PS>("Transmittance");
    multiScatterSlot_ =
        shaderRscs_.getShaderResourceViewSlot<PS>("MultiScattering");

    resize(res);

    psAtmos_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("AtmosphereParams")
        ->setBuffer(psAtmos_);

    psParams_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSParams")
        ->setBuffer(psParams_);

    auto transmittanceSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("TransmittanceSampler")
        ->setSampler(transmittanceSampler);
}

void SkyLUT::resize(const Int2 &res)
{
    LUT_ = RenderTarget(res);
    LUT_.addColorBuffer(
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT,
        DXGI_FORMAT_R32G32B32A32_FLOAT);
}

ComPtr<ID3D11ShaderResourceView> SkyLUT::getLUT() const
{
    return LUT_.getColorShaderResourceView(0);
}

void SkyLUT::setCamera(const Float3 &eye)
{
    psParamsData_.eyePosition = eye;
}

void SkyLUT::setParams(
    int lowResStepCount, int highResStepCount, float highResStep)
{
    psParamsData_.lowResMarchStepCount  = lowResStepCount;
    psParamsData_.highResMarchStepCount = highResStepCount;
    psParamsData_.highResMarchStep      = highResStep;
}

void SkyLUT::enableMultiScattering(bool enable)
{
    psParamsData_.enableMultiScattering = enable;
}

void SkyLUT::generate(
    const AtmosphereProperties      &atmos,
    const Float3                    &sunDirection,
    const Float3                    &sunIntensity,
    ComPtr<ID3D11ShaderResourceView> transmittance,
    ComPtr<ID3D11ShaderResourceView> multiscatter)
{
    psAtmos_.update(atmos);

    psParamsData_.sunDirection = sunDirection;
    psParamsData_.sunIntensity = sunIntensity;
    psParams_.update(psParamsData_);

    transmittanceSlot_->setShaderResourceView(std::move(transmittance));
    multiScatterSlot_->setShaderResourceView(std::move(multiscatter));

    LUT_.bind();
    LUT_.useDefaultViewport();

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext.draw(6, 0);

    shaderRscs_.unbind();
    shader_.unbind();

    LUT_.unbind();
}
