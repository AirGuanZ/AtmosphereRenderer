#include "./mesh.h"

void MeshRenderer::initialize()
{
    shader_.initializeStageFromFile<VS>(
        "./asset/mesh.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/mesh.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    TSlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("Transmittance");
    ASlot_ = shaderRscs_.getShaderResourceViewSlot<PS>("AerialPerspective");

    const D3D11_INPUT_ELEMENT_DESC inputElems[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, position),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
        {
            "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, normal),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
        {
            "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, color),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };
    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);

    atmos_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("AtmosphereParams")
        ->setBuffer(atmos_);

    psParams_.initialize();
    shaderRscs_.getConstantBufferSlot<PS>("PSParams")
        ->setBuffer(psParams_);

    auto TASampler = device.createSampler(
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP,
        D3D11_TEXTURE_ADDRESS_CLAMP);
    shaderRscs_.getSamplerSlot<PS>("TASampler")
        ->setSampler(TASampler);
}

void MeshRenderer::setAtmosphere(
    const AtmosphereProperties      &atmos,
    ComPtr<ID3D11ShaderResourceView> trans,
    ComPtr<ID3D11ShaderResourceView> aerial,
    float                            maxAerialDistance)
{
    atmos_.update(atmos);
    TSlot_->setShaderResourceView(trans);
    ASlot_->setShaderResourceView(aerial);
    psParamsData_.maxAerialDistance = maxAerialDistance;
}

void MeshRenderer::setCamera(const Float3 &eye, const Mat4 &viewProj)
{
    psParamsData_.eyePos = eye;
    viewProj_ = viewProj;
}

void MeshRenderer::setLight(const Float3 &direction, const Float3 &intensity)
{
    psParamsData_.sunTheta     = std::asin(direction.y);
    psParamsData_.sunDirection = direction;
    psParamsData_.sunIntensity = intensity;
}

void MeshRenderer::setWorldScale(float scale)
{
    psParamsData_.worldScale = scale;
}

void MeshRenderer::begin()
{
    psParams_.update(psParamsData_);

    shader_.bind();
    shaderRscs_.bind();

    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void MeshRenderer::end()
{
    deviceContext.setInputLayout(nullptr);
    shaderRscs_.unbind();
    shader_.unbind();
}

void MeshRenderer::render(
    const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world)
{
    vsTransform_.update({ world * viewProj_, world });
    vertexBuffer.bind(0);
    deviceContext.draw(vertexBuffer.getVertexCount(), 0);
    vertexBuffer.unbind(0);
}
