#include "SkyBox.h"

HRESULT SkyBox::createGeometry(ID3D11Device* m_pDevice) {
    UINT LatLines = 20, LongLines = 20;
    UINT numSphereVertices = ((LatLines - 2) * LongLines) + 2;
    numSphereTriangles_ = ((LatLines - 3) * (LongLines) * 2) + (LongLines * 2);

    float phi = 0.0f;
    float theta = 0.0f;

    std::vector<SkyboxVertex> vertices(numSphereVertices);

    XMVECTOR currVertPos = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    vertices[0].x = 0.0f;
    vertices[0].y = 0.0f;
    vertices[0].z = 1.0f;

    for (UINT i = 0; i < LatLines - 2; i++) {
        theta = (i + 1) * (XM_PI / (LatLines - 1));
        XMMATRIX Rotationx = XMMatrixRotationX(theta);
        for (UINT j = 0; j < LongLines; j++) {
            phi = j * (XM_2PI / LongLines);
            XMMATRIX Rotationy = XMMatrixRotationZ(phi);
            currVertPos = XMVector3TransformNormal(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), (Rotationx * Rotationy));
            currVertPos = XMVector3Normalize(currVertPos);
            vertices[i * (__int64)LongLines + j + 1].x = XMVectorGetX(currVertPos);
            vertices[i * (__int64)LongLines + j + 1].y = XMVectorGetY(currVertPos);
            vertices[i * (__int64)LongLines + j + 1].z = XMVectorGetZ(currVertPos);
        }
    }

    vertices[(__int64)numSphereVertices - 1].x = 0.0f;
    vertices[(__int64)numSphereVertices - 1].y = 0.0f;
    vertices[(__int64)numSphereVertices - 1].z = -1.0f;

    std::vector<UINT> indices((__int64)numSphereTriangles_ * 3);

    UINT k = 0;
    for (UINT i = 0; i < LongLines - 1; i++) {
        indices[k] = 0;
        indices[(__int64)k + 2] = i + 1;
        indices[(__int64)k + 1] = i + 2;
        k += 3;
    }
    indices[k] = 0;
    indices[(__int64)k + 2] = LongLines;
    indices[(__int64)k + 1] = 1;
    k += 3;

    for (UINT i = 0; i < LatLines - 3; i++) {
        for (UINT j = 0; j < LongLines - 1; j++) {
            indices[k] = i * LongLines + j + 1;
            indices[(__int64)k + 1] = i * LongLines + j + 2;
            indices[(__int64)k + 2] = (i + 1) * LongLines + j + 1;

            indices[(__int64)k + 3] = (i + 1) * LongLines + j + 1;
            indices[(__int64)k + 4] = i * LongLines + j + 2;
            indices[(__int64)k + 5] = (i + 1) * LongLines + j + 2;

            k += 6;
        }

        indices[k] = (i * LongLines) + LongLines;
        indices[(__int64)k + 1] = (i * LongLines) + 1;
        indices[(__int64)k + 2] = ((i + 1) * LongLines) + LongLines;

        indices[(__int64)k + 3] = ((i + 1) * LongLines) + LongLines;
        indices[(__int64)k + 4] = (i * LongLines) + 1;
        indices[(__int64)k + 5] = ((i + 1) * LongLines) + 1;

        k += 6;
    }

    for (UINT i = 0; i < LongLines - 1; i++) {
        indices[k] = numSphereVertices - 1;
        indices[(__int64)k + 2] = (numSphereVertices - 1) - (i + 1);
        indices[(__int64)k + 1] = (numSphereVertices - 1) - (i + 2);
        k += 3;
    }

    indices[k] = numSphereVertices - 1;
    indices[(__int64)k + 2] = (numSphereVertices - 1) - LongLines;
    indices[(__int64)k + 1] = numSphereVertices - 2;

    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(SkyboxVertex) * numSphereVertices;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA data;
    ZeroMemory(&data, sizeof(data));
    data.pSysMem = vertices.data();
    HRESULT result = m_pDevice->CreateBuffer(&desc, &data, &pVertexBuffer_);

    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        ZeroMemory(&desc, sizeof(desc));
        desc.ByteWidth = sizeof(UINT) * numSphereTriangles_ * 3;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = indices.data();

        result = m_pDevice->CreateBuffer(&desc, &data, &pIndexBuffer_);
    }

    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SkyboxWorldMatrixBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        SkyboxWorldMatrixBuffer skyboxWorldMatrixBuffer;

        skyboxWorldMatrixBuffer.worldMatrix = XMMatrixIdentity();
        skyboxWorldMatrixBuffer.size = XMFLOAT4(radius_, 0.0f, 0.0f, 0.0f);

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &skyboxWorldMatrixBuffer;
        data.SysMemPitch = sizeof(skyboxWorldMatrixBuffer);
        data.SysMemSlicePitch = 0;

        result = m_pDevice->CreateBuffer(&desc, &data, &pWorldMatrixBuffer_);
    }
    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SkyboxViewMatrixBuffer);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = m_pDevice->CreateBuffer(&desc, nullptr, &pViewMatrixBuffer_);
    }

    return result;
}

HRESULT SkyBox::createShaders(ID3D11Device* m_pDevice) {
    static const D3D11_INPUT_ELEMENT_DESC SkyboxInputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;
    int flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT result = D3DCompileFromFile(L"CubeMapVS.hlsl", NULL, NULL, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, NULL);
    if (SUCCEEDED(result)) {
        result = m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &pVertexShader_);
    }
    if (SUCCEEDED(result)) {
        result = D3DCompileFromFile(L"CubeMapPS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
        if (SUCCEEDED(result)) {
            result = m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pPixelShader_);
        }
    }
    if (SUCCEEDED(result)) {
        int numElements = sizeof(SkyboxInputDesc) / sizeof(SkyboxInputDesc[0]);
        result = m_pDevice->CreateInputLayout(SkyboxInputDesc, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &pInputLayout_);
    }

    SafeRelease(vertexShaderBuffer);
    SafeRelease(pixelShaderBuffer);

    return result;
}

HRESULT SkyBox::createTextures(ID3D11Device* m_pDevice) {
    ID3D11ShaderResourceView* m_pTextureView;

    HRESULT result = CreateDDSTextureFromFileEx(m_pDevice, L"textures/texture.dds",
            0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE,
            DDS_LOADER_DEFAULT, nullptr, &m_pTextureView);

    if (SUCCEEDED(result)) {
        resources_.push_back(m_pTextureView);
    }

    return result;
}

HRESULT SkyBox::setRasterizerState(ID3D11Device* m_pDevice, D3D11_CULL_MODE cullMode)
{
    HRESULT result;
    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

    rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = cullMode;
    result = m_pDevice->CreateRasterizerState(&rasterizerDesc, &pRasterizerState_);

    return result;
}

HRESULT SkyBox::update(ID3D11DeviceContext* m_pDeviceContext, Camera* pCamera, XMMATRIX mProjection) {
    SkyboxWorldMatrixBuffer skyboxWorldMatrixBuffer;
    D3D11_MAPPED_SUBRESOURCE skyboxSubresource;

    XMMATRIX mView = pCamera->GetViewMatrix();

    skyboxWorldMatrixBuffer.worldMatrix = XMMatrixIdentity();
    skyboxWorldMatrixBuffer.size = XMFLOAT4(radius_, 0.0f, 0.0f, 0.0f);

    m_pDeviceContext->UpdateSubresource(pWorldMatrixBuffer_, 0, nullptr, &skyboxWorldMatrixBuffer, 0, 0);

    HRESULT result = m_pDeviceContext->Map(pViewMatrixBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &skyboxSubresource);
    
    if (SUCCEEDED(result)) {
        SkyboxViewMatrixBuffer& skyboxSceneBuffer = *reinterpret_cast<SkyboxViewMatrixBuffer*>(skyboxSubresource.pData);
        skyboxSceneBuffer.viewProjectionMatrix = XMMatrixMultiply(mView, mProjection);
        XMFLOAT3 cameraPos = pCamera->GetPosition();
        skyboxSceneBuffer.cameraPos = XMFLOAT4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
        m_pDeviceContext->Unmap(pViewMatrixBuffer_, 0);
    }

    return result;
}

void SkyBox::draw(ID3D11DeviceContext* m_pDeviceContext) {
    m_pDeviceContext->RSSetState(pRasterizerState_);
    m_pDeviceContext->PSSetShaderResources(0, 1, resources_.data());

    m_pDeviceContext->IASetIndexBuffer(pIndexBuffer_, DXGI_FORMAT_R32_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { pVertexBuffer_ };
    UINT strides[] = { 12 };
    UINT offsets[] = { 0 };
    m_pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    m_pDeviceContext->IASetInputLayout(pInputLayout_);
    m_pDeviceContext->VSSetShader(pVertexShader_, nullptr, 0);
    m_pDeviceContext->VSSetConstantBuffers(0, 1, &pWorldMatrixBuffer_);
    m_pDeviceContext->VSSetConstantBuffers(1, 1, &pViewMatrixBuffer_);
    m_pDeviceContext->PSSetShader(pPixelShader_, nullptr, 0);

    m_pDeviceContext->DrawIndexed(numSphereTriangles_ * 3, 0, 0);
}
