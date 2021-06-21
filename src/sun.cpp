#include "./sun.h"

namespace
{
    VertexBuffer<Float2> generateSunDiskVertices(int segCount)
    {
        std::vector<Float2> data;
        for(int i = 0; i < segCount; ++i)
        {
            const float phiBeg = agz::math::lerp(
                0.0f, 2 * PI, static_cast<float>(i) / segCount);
            const float phiEnd = agz::math::lerp(
                0.0f, 2 * PI, static_cast<float>(i + 1) / segCount);

            const Float2 A = {};
            const Float2 B = { std::cos(phiBeg), std::sin(phiBeg) };
            const Float2 C = { std::cos(phiEnd), std::sin(phiEnd) };

            data.push_back(A);
            data.push_back(B);
            data.push_back(C);
        }

        VertexBuffer<Float2> result;
        result.initialize(data.size(), data.data());
        return result;
    }
} // namespace anonymous

void SunRenderer::initialize()
{
    setSunDiskSegments(32);

    shader_.initializeStageFromFile<VS>(
        "./asset/sun.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/sun.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    TSlot_ = shaderRscs_.getShaderResourceViewSlot<VS>("Transmittance");
    
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable    = true;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    dsDesc.DepthFunc      = D3D11_COMPARISON_LESS_EQUAL;
    dsDesc.StencilEnable  = false;
    dsState_ = device.createDepthStencilState(dsDesc);

    const D3D11_INPUT_ELEMENT_DESC inputElems[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,
            0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };
    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);

    atmos_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("AtmosphereParams")
        ->setBuffer(atmos_);

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);

    psParams_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSParams")
        ->setBuffer(psParams_);

    auto transSampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<VS>("TransmittanceSampler")
        ->setSampler(transSampler);
}

void SunRenderer::setSunDiskSegments(int segCount)
{
    sunDiskVertices_ = generateSunDiskVertices(segCount);
}

void SunRenderer::setWorldScale(float worldScale)
{
    worldScale_ = worldScale;
}

void SunRenderer::setSun(
    float radius, const Float3 &direction, const Float3 &intensity)
{
    radius_    = radius;
    direction_ = direction;
    intensity_ = intensity;
}

void SunRenderer::setCamera(const Float3 &eye, const Mat4 &viewProj)
{
    eye_      = eye;
    viewProj_ = viewProj;
}

void SunRenderer::setAtmosphere(const AtmosphereProperties &atmos)
{
    atmos_.update(atmos);
}

void SunRenderer::setTransmittance(ComPtr<ID3D11ShaderResourceView> T)
{
    TSlot_->setShaderResourceView(std::move(T));
}

void SunRenderer::render()
{
    updateConstantBuffers();

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setInputLayout(inputLayout_);
    deviceContext->OMSetDepthStencilState(dsState_.Get(), 0);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    sunDiskVertices_.bind(0);
    deviceContext.draw(sunDiskVertices_.getVertexCount(), 0);
    sunDiskVertices_.unbind(0);

    deviceContext->OMSetDepthStencilState(nullptr, 0);
    deviceContext.setInputLayout(nullptr);

    shaderRscs_.unbind();
    shader_.unbind();
}

void SunRenderer::updateConstantBuffers()
{
    psParams_.update({ intensity_, 0 });

    const auto localFrame = agz::math::tcoord3<float>::from_z(direction_);

    const Mat4 world =
        Trans4::scale(Float3(radius_)) *
        Mat4::from_rows(
            Float4(localFrame.x, 0),
            Float4(localFrame.y, 0),
            Float4(localFrame.z, 0),
            Float4(0, 0, 0, 1)) *
        Trans4::translate(eye_) *
        Trans4::translate(-direction_);

    const float sunTheta = std::asin(-direction_.y);

    vsTransform_.update(
        { world * viewProj_, sunTheta, worldScale_ * eye_.y, 0, 0 });
}
