#include <agz-utils/thread.h>

#include "./transmittance.h"

void TransmittanceLUT::generate(
    const Int2 &res, const AtmosphereProperties &atmos)
{
    if(!shader_.isAllStageAvailable())
    {
        shader_.initializeStageFromFile<CS>(
            "./asset/transmittance.hlsl", nullptr, "CSMain");
    }
    auto shaderRscs = shader_.createResourceManager();

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

    shaderRscs.getUnorderedAccessViewSlot<CS>("Transmittance")
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

    srv_  = std::move(srv);
}

ComPtr<ID3D11ShaderResourceView> TransmittanceLUT::getSRV() const
{
    return srv_;
}
