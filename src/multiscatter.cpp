#include <random>

#include <cyPoint.h>
#include <cySampleElim.h>

#include "./multiscatter.h"

namespace
{

    std::vector<Float2> getPoissonDiskSamples(int count)
    {
        std::default_random_engine rng{ std::random_device()() };
        std::uniform_real_distribution<float> dis(0, 1);

        std::vector<cy::Point2f> rawPoints;
        for(int i = 0; i < count * 10; ++i)
        {
            const float u = dis(rng);
            const float v = dis(rng);
            rawPoints.push_back({ u, v });
        }

        std::vector<cy::Point2f> outputPoints(count);

        cy::WeightedSampleElimination<cy::Point2f, float, 2> wse;
        wse.SetTiling(true);
        wse.Eliminate(
            rawPoints.data(),    rawPoints.size(),
            outputPoints.data(), outputPoints.size());

        std::vector<Float2> result;
        for(auto &p : outputPoints)
            result.push_back({ p.x, p.y });

        return result;
    }

    constexpr int DIR_SAMPLE_COUNT     = 64;
    constexpr int RAY_MARCH_STEP_COUNT = 256;

}

void MultiScatteringLUT::generate(
    const Int2                      &res,
    ComPtr<ID3D11ShaderResourceView> transmittance,
    const Float3                    &terrainAlbedo,
    const AtmosphereProperties      &atmos)
{
    if(!shader_.isAllStageAvailable())
    {
        shader_.initializeStageFromFile<CS>(
            "./asset/multiscatter.hlsl", nullptr, "CSMain");
    }
    auto shaderRscs = shader_.createResourceManager();

    auto rawSamples = getPoissonDiskSamples(DIR_SAMPLE_COUNT);

    D3D11_BUFFER_DESC rawSamplesBufDesc;
    rawSamplesBufDesc.ByteWidth           = sizeof(Float2) * DIR_SAMPLE_COUNT;
    rawSamplesBufDesc.Usage               = D3D11_USAGE_IMMUTABLE;
    rawSamplesBufDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    rawSamplesBufDesc.CPUAccessFlags      = 0;
    rawSamplesBufDesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    rawSamplesBufDesc.StructureByteStride = sizeof(Float2);

    D3D11_SUBRESOURCE_DATA rawSamplesBufSubrscData;
    rawSamplesBufSubrscData.pSysMem          = rawSamples.data();
    rawSamplesBufSubrscData.SysMemPitch      = 0;
    rawSamplesBufSubrscData.SysMemSlicePitch = 0;

    auto rawSamplesBuf = device.createBuffer(
        rawSamplesBufDesc, &rawSamplesBufSubrscData);

    D3D11_SHADER_RESOURCE_VIEW_DESC rawSamplesSRVDesc;
    rawSamplesSRVDesc.Format              = DXGI_FORMAT_UNKNOWN;
    rawSamplesSRVDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
    rawSamplesSRVDesc.Buffer.FirstElement = 0;
    rawSamplesSRVDesc.Buffer.NumElements  = DIR_SAMPLE_COUNT;

    auto rawSamplesSRV = device.createSRV(rawSamplesBuf, rawSamplesSRVDesc);

    shaderRscs.getShaderResourceViewSlot<CS>("RawDirSamples")
        ->setShaderResourceView(rawSamplesSRV);

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width          = static_cast<UINT>(res.x);
    texDesc.Height         = static_cast<UINT>(res.y);
    texDesc.MipLevels      = 1;
    texDesc.ArraySize      = 1;
    texDesc.Format         = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc     = { 1, 0 };
    texDesc.Usage          = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags      = D3D11_BIND_UNORDERED_ACCESS;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags      = 0;
    auto shaderOutputTex = device.createTex2D(texDesc);

    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    auto shaderResourceTex = device.createTex2D(texDesc);

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
    uavDesc.Format             = DXGI_FORMAT_R32G32B32A32_FLOAT;
    uavDesc.ViewDimension      = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    auto uav = device.createUAV(shaderOutputTex, uavDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                    = DXGI_FORMAT_R32G32B32A32_FLOAT;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    auto srv = device.createSRV(shaderResourceTex, srvDesc);

    ConstantBuffer<AtmosphereProperties> atmosConsts;
    atmosConsts.initialize();
    atmosConsts.update(atmos);
    shaderRscs.getConstantBufferSlot<CS>("AtmosphereParams")
        ->setBuffer(atmosConsts);

    struct CSParams
    {
        Float3 terrainAlbedo;
        int dirSampleCount;

        Float3 sunIntensity;
        int rayMarchStepCount;
    };

    ConstantBuffer<CSParams> csParams;
    csParams.initialize();
    csParams.update(
        { terrainAlbedo, DIR_SAMPLE_COUNT, Float3(1), RAY_MARCH_STEP_COUNT });
    shaderRscs.getConstantBufferSlot<CS>("CSParams")
        ->setBuffer(csParams);

    shaderRscs.getShaderResourceViewSlot<CS>("Transmittance")
        ->setShaderResourceView(transmittance);

    auto transmittanceSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs.getSamplerSlot<CS>("TransmittanceSampler")
        ->setSampler(transmittanceSampler);

    shaderRscs.getUnorderedAccessViewSlot<CS>("MultiScattering")
        ->setUnorderedAccessView(uav);

    constexpr int THREAD_GROUP_SIZE_X = 16;
    constexpr int THREAD_GROUP_SIZE_Y = 16;
    const int threadGroupCountX =
        (res.x + THREAD_GROUP_SIZE_X - 1) / THREAD_GROUP_SIZE_X;
    const int threadGroupCountY =
        (res.y + THREAD_GROUP_SIZE_Y - 1) / THREAD_GROUP_SIZE_Y;

    shader_.bind();
    shaderRscs.bind();
    deviceContext.dispatch(threadGroupCountX, threadGroupCountY);
    shaderRscs.unbind();
    shader_.unbind();
    deviceContext->CopyResource(shaderResourceTex.Get(), shaderOutputTex.Get());

    srv_ = std::move(srv);
}

ComPtr<ID3D11ShaderResourceView> MultiScatteringLUT::getSRV() const
{
    return srv_;
}
