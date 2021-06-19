#include "./aerial_lut.h"

void AerialPerspectiveLUT_::initialize(const Int3 &res)
{
    shader_.initializeStageFromFile<CS>(
        "./asset/aerial_lut.hlsl", nullptr, "CSMain");
    shaderRscs_ = shader_.createResourceManager();

    shadowMapSlot_ =
        shaderRscs_.getShaderResourceViewSlot<CS>("ShadowMap");
    multiScatterSlot_  =
        shaderRscs_.getShaderResourceViewSlot<CS>("MultiScattering");
    transmittanceSlot_ =
        shaderRscs_.getShaderResourceViewSlot<CS>("Transmittance");

    resize(res);
    
    csParams_.initialize();
    shaderRscs_.getConstantBufferSlot<CS>("CSParams")->setBuffer(csParams_);

    atmos_.initialize();
    shaderRscs_.getConstantBufferSlot<CS>("AtmosphereParams")->setBuffer(atmos_);

    auto transmittanceSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<CS>("TransmittanceSampler")
        ->setSampler(transmittanceSampler);

    auto shadowSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<CS>("ShadowSampler")
        ->setSampler(shadowSampler);
}

void AerialPerspectiveLUT_::resize(const Int3 &res)
{
    D3D11_TEXTURE3D_DESC texDesc;
    texDesc.Width          = static_cast<UINT>(res.x);
    texDesc.Height         = static_cast<UINT>(res.y);
    texDesc.Depth          = static_cast<UINT>(res.z);
    texDesc.MipLevels      = 1;
    texDesc.Format         = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;
    auto tex = device.createTex3D(texDesc);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format                = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension         = D3D11_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.FirstWSlice = 0;
    uavDesc.Texture3D.WSize       = static_cast<UINT>(res.z);
    uavDesc.Texture3D.MipSlice    = 0;
    auto uav = device.createUAV(tex, uavDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Texture3D.MipLevels       = 1;
    srvDesc.Texture3D.MostDetailedMip = 0;
    auto srv = device.createSRV(tex, srvDesc);

    res_ = res;
    srv_ = std::move(srv);
    uav_ = std::move(uav);

    shaderRscs_.getUnorderedAccessViewSlot<CS>("AerialPerspectiveLUT")
        ->setUnorderedAccessView(std::move(uav_));
}

void AerialPerspectiveLUT_::setCamera(
    const Float3                    &shadowEyePos,
    float                            eyePosY,
    const Camera::FrustumDirections &frustumDirs)
{
    csParamsData_.shadowEyePosition = shadowEyePos;
    csParamsData_.eyePositionY = eyePosY;
    csParamsData_.frustumA = frustumDirs.frustumA;
    csParamsData_.frustumB = frustumDirs.frustumB;
    csParamsData_.frustumC = frustumDirs.frustumC;
    csParamsData_.frustumD = frustumDirs.frustumD;
}

void AerialPerspectiveLUT_::setSun(const Float3 &sunDirection)
{
    csParamsData_.sunDirection = sunDirection.normalize();
    csParamsData_.sunTheta = std::asin(-csParamsData_.sunDirection.y);
}

void AerialPerspectiveLUT_::setParams(
    float worldScale,
    bool  enableMultiScattering,
    bool  enableShadow,
    float maxDistance,
    int   perSliceMarchStepCount)
{
    csParamsData_.worldScale            = worldScale;
    csParamsData_.enableMultiScattering = enableMultiScattering;
    csParamsData_.enableShadow          = enableShadow;
    csParamsData_.maxDistance           = maxDistance;
    csParamsData_.perSliceStepCount     = perSliceMarchStepCount;
}

ComPtr<ID3D11ShaderResourceView> AerialPerspectiveLUT_::getOutput() const
{
    return srv_;
}

void AerialPerspectiveLUT_::render(
    const Mat4                      &shadowViewProj,
    ComPtr<ID3D11ShaderResourceView> shadowMap,
    const AtmosphereProperties      &atmos,
    ComPtr<ID3D11ShaderResourceView> multiScatter,
    ComPtr<ID3D11ShaderResourceView> transmittance)
{
    atmos_.update(atmos);

    csParamsData_.shadowViewProj = shadowViewProj;
    csParams_.update(csParamsData_);

    shadowMapSlot_->setShaderResourceView(shadowMap);
    multiScatterSlot_->setShaderResourceView(multiScatter);
    transmittanceSlot_->setShaderResourceView(transmittance);

    shader_.bind();
    shaderRscs_.bind();

    constexpr int THREAD_GROUP_SIZE_X = 16;
    constexpr int THREAD_GROUP_SIZE_Y = 16;
    const int threadGroupCountX =
        (res_.x + THREAD_GROUP_SIZE_X - 1) / THREAD_GROUP_SIZE_X;
    const int threadGroupCountY =
        (res_.y + THREAD_GROUP_SIZE_Y - 1) / THREAD_GROUP_SIZE_Y;

    deviceContext.dispatch(threadGroupCountX, threadGroupCountY);

    shaderRscs_.unbind();
    shader_.unbind();
}
