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

    auto MTSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("MTSampler")
        ->setSampler(MTSampler);
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

void SkyLUT::setCamera(const Float3 &atmosEyePos)
{
    psParamsData_.atmosEyePosition = atmosEyePos;
}

void SkyLUT::setRayMarching(int stepCount)
{
    psParamsData_.lowResMarchStepCount = stepCount;
}

void SkyLUT::setAtmosphere(const AtmosphereProperties &atmos)
{
    psAtmos_.update(atmos);
}

void SkyLUT::setSun(const Float3 &direction, const Float3 &intensity)
{
    psParamsData_.sunDirection = direction;
    psParamsData_.sunIntensity = intensity;
}

void SkyLUT::setTransmittance(ComPtr<ID3D11ShaderResourceView> T)
{
    transmittanceSlot_->setShaderResourceView(std::move(T));
}

void SkyLUT::setMultiScattering(bool enabled, ComPtr<ID3D11ShaderResourceView> M)
{
    psParamsData_.enableMultiScattering = enabled;
    multiScatterSlot_->setShaderResourceView(std::move(M));
}

void SkyLUT::generate()
{
    psParams_.update(psParamsData_);

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
