#pragma once

#include "framework.h"
#include "camera.h"

#include <vector>

/*class Shape {
public:
    Shape():
        pVertexBuffer_(nullptr),
        pIndexBuffer_(nullptr),
        pVertexShader_(nullptr),
        pInputLayout_(nullptr),
        pPixelShader_(nullptr)
    {};

    Shape(const Shape&) = delete;
    Shape(const Shape&&) = delete;

    virtual ~Shape() {
        SAFE_RELEASE(pVertexBuffer_);
        SAFE_RELEASE(pIndexBuffer_);
        SAFE_RELEASE(pVertexShader_);
        SAFE_RELEASE(pInputLayout_);
        SAFE_RELEASE(pPixelShader_);
    }

    virtual HRESULT createGeometry(ID3D11Device* m_pDevice) = 0;
    virtual HRESULT createShaders(ID3D11Device* pDevice) = 0;
protected:
    
    ID3D11VertexShader* pVertexShader_;
    ID3D11PixelShader* pPixelShader_;
    ID3D11InputLayout* pInputLayout_;
};*/


class Cube
{
    struct Vertex {
        XMFLOAT3 pos;
        XMFLOAT2 uv;
        XMFLOAT3 normal;
        XMFLOAT3 tangent;
    };
public:
    Cube() = default;

    Cube(const Cube&) = delete;
    Cube(const Cube&&) = delete;

    ~Cube() = default;

    HRESULT createGeometry(ID3D11Device* m_pDevice);
    HRESULT createShaders(ID3D11Device* pDevice);
    void setGeometry(ID3D11DeviceContext* pDeviceContext);
private:
    ID3D11Buffer* pVertexBuffer_;
    ID3D11Buffer* pIndexBuffer_;

    ID3D11VertexShader* pVertexShader_;
    ID3D11PixelShader* pPixelShader_;
    ID3D11InputLayout* pInputLayout_;
};

class Rect
{
    struct TransparentVertex {
        float x, y, z;
        COLORREF color;
    };
public:
    Rect():
        color_(RGB(255, 0, 0))
    {};

    Rect(const Rect&) = delete;
    Rect(const Rect&&) = delete;

    ~Rect() = default;

    HRESULT createGeometry(ID3D11Device* m_pDevice);
    HRESULT createShaders(ID3D11Device* pDevice);
    void SetColor(COLORREF color, ID3D11DeviceContext* m_pDeviceContext);
private:
    COLORREF color_;
    ID3D11Buffer* pVertexBuffer_ = nullptr;
    ID3D11Buffer* pIndexBuffer_ = nullptr;

    ID3D11VertexShader* pVertexShader_ = nullptr;
    ID3D11PixelShader* pPixelShader_ = nullptr;
    ID3D11InputLayout* pInputLayout_ = nullptr;
};