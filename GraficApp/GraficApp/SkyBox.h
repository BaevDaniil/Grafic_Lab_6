#pragma once

#include "framework.h"
#include "camera.h"
#include <vector>

class SkyBox
{
    struct SkyboxWorldMatrixBuffer {
        XMMATRIX worldMatrix;
        XMFLOAT4 size;
    };

    struct SkyboxViewMatrixBuffer {
        XMMATRIX viewProjectionMatrix;
        XMFLOAT4 cameraPos;
    };

    struct SkyboxVertex {
        float x, y, z;
    };
public:
    SkyBox() :
        pVertexBuffer_(nullptr),
        pIndexBuffer_(nullptr),
        pInputLayout_(nullptr),
        pVertexShader_(nullptr),
        pRasterizerState_(nullptr),
        pPixelShader_(nullptr),
        pWorldMatrixBuffer_ (nullptr),
        pViewMatrixBuffer_ (nullptr),
        radius_ (1.0f),
        numSphereTriangles_(0)
    {};

    SkyBox(const SkyBox&) = delete;
    SkyBox(const SkyBox&&) = delete;

    ~SkyBox() {
        SafeRelease(pVertexBuffer_);
        SafeRelease(pIndexBuffer_);
        SafeRelease(pInputLayout_);
        SafeRelease(pVertexShader_);
        SafeRelease(pRasterizerState_);
        SafeRelease(pPixelShader_);
        SafeRelease(pWorldMatrixBuffer_);
        SafeRelease(pViewMatrixBuffer_);

        for (auto resource : resources_)
            SafeRelease(resource);
    }

    HRESULT createGeometry(ID3D11Device* m_pDevice);
    HRESULT createShaders(ID3D11Device* m_pDevice);
    HRESULT createTextures(ID3D11Device* m_pDevice);
    
    HRESULT update(ID3D11DeviceContext* m_pDeviceContext, Camera* pCamera, XMMATRIX mProjection);
    void draw(ID3D11DeviceContext* m_pDeviceContext);

    void setRadius(float radius) { radius_ = radius; };
    HRESULT setRasterizerState(ID3D11Device* m_pDevice, D3D11_CULL_MODE cullMode);

private:
    ID3D11Buffer* pVertexBuffer_;
    ID3D11Buffer* pIndexBuffer_;
    ID3D11InputLayout* pInputLayout_;
    ID3D11VertexShader* pVertexShader_;
    ID3D11RasterizerState* pRasterizerState_;
    ID3D11PixelShader* pPixelShader_ ;

    std::vector<ID3D11ShaderResourceView*> resources_;

    ID3D11Buffer* pWorldMatrixBuffer_;
    ID3D11Buffer* pViewMatrixBuffer_;

    UINT numSphereTriangles_;
    float radius_;
};

