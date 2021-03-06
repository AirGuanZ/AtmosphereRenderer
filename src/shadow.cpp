#include "./shadow.h"

void ShadowMap::initialize(const Int2 &res)
{
    shader_.initializeStageFromFile<VS>(
        "./asset/shadow.hlsl", nullptr, "VSMain");
    shader_.initializeStageFromFile<PS>(
        "./asset/shadow.hlsl", nullptr, "PSMain");

    shaderRscs_ = shader_.createResourceManager();

    const D3D11_INPUT_ELEMENT_DESC inputElems[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,
            0, offsetof(Vertex, position),
            D3D11_INPUT_PER_VERTEX_DATA, 0
        }
    };
    inputLayout_ = InputLayoutBuilder(inputElems).build(shader_);

    renderTarget_ = RenderTarget(res);
    renderTarget_.addDepthStencil(
        DXGI_FORMAT_R32_TYPELESS,
        DXGI_FORMAT_D32_FLOAT,
        DXGI_FORMAT_R32_FLOAT);
    
    D3D11_RASTERIZER_DESC rasDesc;
    rasDesc.FillMode              = D3D11_FILL_SOLID;
    rasDesc.CullMode              = D3D11_CULL_NONE;
    rasDesc.FrontCounterClockwise = false;
    rasDesc.DepthBias             = 0;
    rasDesc.DepthBiasClamp        = 0;
    rasDesc.SlopeScaledDepthBias  = 0;
    rasDesc.DepthClipEnable       = true;
    rasDesc.ScissorEnable         = false;
    rasDesc.MultisampleEnable     = false;
    rasDesc.AntialiasedLineEnable = false;
    rasterState_ = device.createRasterizerState(rasDesc);

    vsTransform_.initialize();
    shaderRscs_.getConstantBufferSlot<VS>("VSTransform")
        ->setBuffer(vsTransform_);
}

ComPtr<ID3D11ShaderResourceView> ShadowMap::getShadowMap() const
{
    return renderTarget_.getDepthShaderResourceView();
}

void ShadowMap::setSun(const Mat4 &lightViewProj)
{
    viewProj_ = lightViewProj;
}

void ShadowMap::begin()
{
    renderTarget_.clearDepth(1);
    renderTarget_.bind();
    renderTarget_.useDefaultViewport();

    shader_.bind();
    shaderRscs_.bind();

    deviceContext->RSSetState(rasterState_.Get());
    deviceContext.setInputLayout(inputLayout_);
    deviceContext.setPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ShadowMap::end()
{
    deviceContext.setInputLayout(nullptr);
    deviceContext->RSSetState(nullptr);

    shaderRscs_.unbind();
    shader_.unbind();

    renderTarget_.unbind();
}

void ShadowMap::render(
    const VertexBuffer<Vertex> &vertexBuffer, const Mat4 &world)
{
    vsTransform_.update({ world * viewProj_ });
    vertexBuffer.bind(0);
    deviceContext.draw(vertexBuffer.getVertexCount(), 0);
    vertexBuffer.unbind(0);
}
