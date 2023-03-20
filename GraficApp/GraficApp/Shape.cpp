#include "Shape.h"

HRESULT Cube::createGeometry(ID3D11Device* pDevice)
{
    static const Vertex Vertices[] = {
        {{-1.0, -1.0,  1.0}, {0,1}, {0,-1,0}, {1,0,0}},
        {{ 1.0, -1.0,  1.0}, {1,1}, {0,-1,0}, {1,0,0}},
        {{ 1.0, -1.0, -1.0}, {1,0}, {0,-1,0}, {1,0,0}},
        {{-1.0, -1.0, -1.0}, {0,0}, {0,-1,0}, {1,0,0}},

        {{-1.0,  1.0, -1.0}, {0,1}, {0,1,0}, {1,0,0}},
        {{ 1.0,  1.0, -1.0}, {1,1}, {0,1,0}, {1,0,0}},
        {{ 1.0,  1.0,  1.0}, {1,0}, {0,1,0}, {1,0,0}},
        {{-1.0,  1.0,  1.0}, {0,0}, {0,1,0}, {1,0,0}},

        {{ 1.0, -1.0, -1.0}, {0,1}, {1,0,0}, {0,0,1}},
        {{ 1.0, -1.0,  1.0}, {1,1}, {1,0,0}, {0,0,1}},
        {{ 1.0,  1.0,  1.0}, {1,0}, {1,0,0}, {0,0,1}},
        {{ 1.0,  1.0, -1.0}, {0,0}, {1,0,0}, {0,0,1}},

        {{-1.0, -1.0,  1.0}, {0,1}, {-1,0,0}, {0,0,-1}},
        {{-1.0, -1.0, -1.0}, {1,1}, {-1,0,0}, {0,0,-1}},
        {{-1.0,  1.0, -1.0}, {1,0}, {-1,0,0}, {0,0,-1}},
        {{-1.0,  1.0,  1.0}, {0,0}, {-1,0,0}, {0,0,-1}},

        {{ 1.0, -1.0,  1.0}, {0,1}, {0,0,1}, {-1,0,0}},
        {{-1.0, -1.0,  1.0}, {1,1}, {0,0,1}, {-1,0,0}},
        {{-1.0,  1.0,  1.0}, {1,0}, {0,0,1}, {-1,0,0}},
        {{ 1.0,  1.0,  1.0}, {0,0}, {0,0,1}, {-1,0,0}},

        {{-1.0, -1.0, -1.0}, {0,1}, {0,0,-1}, {1,0,0}},
        {{ 1.0, -1.0, -1.0}, {1,1}, {0,0,-1}, {1,0,0}},
        {{ 1.0,  1.0, -1.0}, {1,0}, {0,0,-1}, {1,0,0}},
        {{-1.0,  1.0, -1.0}, {0,0}, {0,0,-1}, {1,0,0}}
    };
    static const USHORT Indices[] = {
        0, 2, 1, 0, 3, 2,
        4, 6, 5, 4, 7, 6,
        8, 10, 9, 8, 11, 10,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 22, 21, 20, 23, 22
    };

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(Vertices);
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &Vertices;
    data.SysMemPitch = sizeof(Vertices);
    data.SysMemSlicePitch = 0;

    HRESULT result = pDevice->CreateBuffer(&desc, &data, &pVertexBuffer_);

    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(Indices);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &Indices;
        data.SysMemPitch = sizeof(Indices);
        data.SysMemSlicePitch = 0;

        result = pDevice->CreateBuffer(&desc, &data, &pIndexBuffer_);
    }
    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(WorldMatrixBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        WorldMatrixBuffer worldMatrixBuffer;
        worldMatrixBuffer.worldMatrix = DirectX::XMMatrixIdentity();
        worldMatrixBuffer.shine = XMFLOAT4(300.0f, 0.0f, 0.0f, 0.0f);

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &worldMatrixBuffer;
        data.SysMemPitch = sizeof(worldMatrixBuffer);
        data.SysMemSlicePitch = 0;

        result = pDevice->CreateBuffer(&desc, &data, &pWorldMatrixBuffer_);
    }

    return result;
}

HRESULT Cube::createShaders(ID3D11Device* pDevice) {
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;

    HRESULT result = S_OK;

    int flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    D3DInclude includeObj;

    if (SUCCEEDED(result)) {
        result = D3DCompileFromFile(L"VertexShader.hlsl", NULL, &includeObj, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, NULL);
        if (SUCCEEDED(result)) {
            result = pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &pVertexShader_);
        }
    }
    if (SUCCEEDED(result)) {
        result = D3DCompileFromFile(L"PixelShader.hlsl", NULL, &includeObj, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
        if (SUCCEEDED(result)) {
            result = pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pPixelShader_);
        }
    }
    if (SUCCEEDED(result)) {
        int numElements = sizeof(InputDesc) / sizeof(InputDesc[0]);
        result = pDevice->CreateInputLayout(InputDesc, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &pInputLayout_);
    }

    SafeRelease(vertexShaderBuffer);
    SafeRelease(pixelShaderBuffer);

    return result;
}

HRESULT Cube::createTextures(ID3D11Device* pDevice) {
    ID3D11ShaderResourceView* m_pTextureView[2];

    HRESULT result = CreateDDSTextureFromFile(pDevice, L"textures/156.dds", nullptr, &m_pTextureView[0]);

    if (SUCCEEDED(result)) {
        result = CreateDDSTextureFromFile(pDevice, L"textures/156_norm.dds", nullptr, &m_pTextureView[1]);
    }

    if (SUCCEEDED(result))
    {
        resources_.push_back(m_pTextureView[0]);
        resources_.push_back(m_pTextureView[1]);
    }

    return result;
}

HRESULT Cube::update(ID3D11DeviceContext* m_pDeviceContext) {
    worldMatrix_ = DirectX::XMMatrixIdentity();
    worldMatrix_ = rotateMatrix_ * scaleMatrix_ * translateMatrix_;
    WorldMatrixBuffer worldMatrixBuffer;
    worldMatrixBuffer.worldMatrix = worldMatrix_;

    m_pDeviceContext->UpdateSubresource(pWorldMatrixBuffer_, 0, nullptr, &worldMatrixBuffer, 0, 0);

    return S_OK;
}

void Cube::draw(ID3D11Buffer* pViewMatrixBuffer, ID3D11DeviceContext* pDeviceContext)
{
    pDeviceContext->RSSetState(rasterizerState_);
    pDeviceContext->PSSetShaderResources(0, 1, resources_.data());

    pDeviceContext->IASetIndexBuffer(pIndexBuffer_, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { pVertexBuffer_ };
    UINT strides[] = { sizeof(Vertex) };
    UINT offsets[] = { 0 };
    pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pDeviceContext->IASetInputLayout(pInputLayout_);
    pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pDeviceContext->VSSetConstantBuffers(0, 1, &pWorldMatrixBuffer_);
    pDeviceContext->VSSetConstantBuffers(1, 1, &pViewMatrixBuffer);
    pDeviceContext->VSSetShader(pVertexShader_, nullptr, 0);
    pDeviceContext->PSSetShader(pPixelShader_, nullptr, 0);
    pDeviceContext->PSSetConstantBuffers(1, 1, &pViewMatrixBuffer);
    pDeviceContext->DrawIndexed(36, 0, 0);
    pDeviceContext->VSSetConstantBuffers(0, 1, &pWorldMatrixBuffer_);
    pDeviceContext->DrawIndexed(36, 0, 0);
}

HRESULT Rect::createGeometry(ID3D11Device* m_pDevice)
{
    static const TransparentVertex VerticesT[] = {
        {0, -1, -1, color_},
        {0,  1, -1, color_},
        {0,  1,  1, color_},
        {0, -1,  1, color_}
    };
    static const USHORT IndicesT[] = {
        0, 2, 1, 0, 3, 2
    };
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(VerticesT);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    data.pSysMem = &VerticesT;
    data.SysMemPitch = sizeof(VerticesT);
    data.SysMemSlicePitch = 0;

    HRESULT result = m_pDevice->CreateBuffer(&desc, &data, &pVertexBuffer_);
    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(IndicesT);
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &IndicesT;
        data.SysMemPitch = sizeof(IndicesT);
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &pIndexBuffer_);
    }

    if (SUCCEEDED(result)) {
        desc = {};
        desc.ByteWidth = sizeof(WorldMatrixBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        WorldMatrixBuffer worldMatrixBuffer;
        worldMatrixBuffer.worldMatrix = DirectX::XMMatrixIdentity();
        worldMatrixBuffer.shine = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);

        data = {};
        data.pSysMem = &worldMatrixBuffer;
        data.SysMemPitch = sizeof(worldMatrixBuffer);
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &pWorldMatrixBuffer_);
    }
    return result;
}

HRESULT Rect::createShaders(ID3D11Device* m_pDevice)
{
    static const D3D11_INPUT_ELEMENT_DESC InputDescT[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0,  DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;
    int flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    D3DInclude includeObj;
    D3D_SHADER_MACRO Shader_Macros[] = { {"USE_LIGHTS"}, {NULL, NULL} };

   HRESULT result = D3DCompileFromFile(L"TVS.hlsl", NULL, &includeObj, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, NULL);
   if (SUCCEEDED(result)) {
       result = m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &pVertexShader_);
   }
    if (SUCCEEDED(result)) {
        result = D3DCompileFromFile(L"TPS.hlsl", Shader_Macros, &includeObj, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
        if (SUCCEEDED(result)) {
            result = m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pPixelShader_);
        }
    }
    if (SUCCEEDED(result)) {
        int numElements = sizeof(InputDescT) / sizeof(InputDescT[0]);
        result = m_pDevice->CreateInputLayout(InputDescT, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &pInputLayout_);
    }

    SafeRelease(vertexShaderBuffer);
    SafeRelease(pixelShaderBuffer);

    return result;
}

HRESULT Rect::update(ID3D11DeviceContext* m_pDeviceContext) {
    worldMatrix_ = DirectX::XMMatrixIdentity();
    worldMatrix_ = rotateMatrix_ * scaleMatrix_ * translateMatrix_;
    WorldMatrixBuffer worldMatrixBuffer;
    worldMatrixBuffer.worldMatrix = worldMatrix_;

    m_pDeviceContext->UpdateSubresource(pWorldMatrixBuffer_, 0, nullptr, &worldMatrixBuffer, 0, 0);
    
    return S_OK;
}

void Rect::draw(ID3D11Buffer* pViewMatrixBuffer, ID3D11DeviceContext* pDeviceContext)
{
    pDeviceContext->RSSetState(rasterizerState_);

    pDeviceContext->IASetIndexBuffer(pIndexBuffer_, DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { pVertexBuffer_ };
    UINT strides[] = { 16 };
    UINT offsets[] = { 0 };
    pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pDeviceContext->IASetInputLayout(pInputLayout_);

    pDeviceContext->VSSetShader(pVertexShader_, nullptr, 0);
    pDeviceContext->PSSetShader(pPixelShader_, nullptr, 0);
    pDeviceContext->VSSetConstantBuffers(1, 1, &pViewMatrixBuffer);

    //pDeviceContext->OMSetBlendState(pBlendState_, nullptr, 0xFFFFFFFF);

    pDeviceContext->VSSetConstantBuffers(0, 1, &pWorldMatrixBuffer_);
    pDeviceContext->DrawIndexed(6, 0, 0);
}

void Rect::SetColor(COLORREF color, ID3D11DeviceContext* m_pDeviceContext) {
    color_ = color;
    D3D11_MAPPED_SUBRESOURCE subresource;
    HRESULT result = m_pDeviceContext->Map(pVertexBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    if (SUCCEEDED(result)) {
        static const TransparentVertex VerticesT[] = {
            {0, -1, -1, color_},
            {0,  1, -1, color_},
            {0,  1,  1, color_},
            {0, -1,  1, color_}
        };
        memcpy(subresource.pData, VerticesT, sizeof(VerticesT));
        m_pDeviceContext->Unmap(pVertexBuffer_, 0);
    }
}

HRESULT Shape::setRasterizerState(ID3D11Device* m_pDevice, D3D11_CULL_MODE cullMode)
{
    HRESULT result;
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

    rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = cullMode;
    result = m_pDevice->CreateRasterizerState(&rasterizerDesc, &rasterizerState_);

    return result;
}

void Shape::translate(DirectX::XMMATRIX translateMatrix)
{
    this->translateMatrix_ = translateMatrix;
}

void Shape::scale(DirectX::XMMATRIX scaleMatrix)
{
    this->scaleMatrix_ = scaleMatrix;
}

void Shape::rotate(DirectX::XMMATRIX rotateMatrix)
{
    this->rotateMatrix_ = rotateMatrix;
}
