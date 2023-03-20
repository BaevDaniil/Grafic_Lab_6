#pragma once

#include <windows.h>
#include <vector>
#include <string>

#include "framework.h"
#include "D3DInclude.h"
#include "camera.h"
#include "input.h"
#include "Shape.h"
#include "SkyBox.h"

#define MAX_LIGHT 10

struct Light {
    XMFLOAT4 pos;
    XMFLOAT4 color;
};

struct ViewMatrixBuffer {
    XMMATRIX viewProjectionMatrix;
    XMFLOAT4 cameraPos;
    XMINT4 lightParams;
    Light lights[MAX_LIGHT];
    XMFLOAT4 ambientColor;
};

struct SkyboxVertex {
    float x, y, z;
};

struct SkyboxWorldMatrixBuffer {
    XMMATRIX worldMatrix;
    XMFLOAT4 size;
};

struct SkyboxViewMatrixBuffer {
    XMMATRIX viewProjectionMatrix;
    XMFLOAT4 cameraPos;
};

struct TransparentVertex {
    float x, y, z;
    COLORREF color;
};

class Renderer
{
public:
    static Renderer& GetInstance();
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;

    ~Renderer();
    void CleanAll();

    bool Init(HINSTANCE hInstance, const HWND hWnd);
    bool Render();
    bool Resize(const unsigned width, const unsigned height);

    static constexpr UINT defaultWidth = 1280;
    static constexpr UINT defaultHeight = 720;

private:
    Renderer();

    std::vector<Shape*> shapes_;
    SkyBox skybox_;

    HRESULT InitScene();
    void InputHandler();
    bool UpdateScene();

    ID3D11Device* pDevice_ = NULL;
    ID3D11DeviceContext* pDeviceContext_ = NULL;
    IDXGISwapChain* pSwapChain_ =  NULL;
    ID3D11RenderTargetView* pRenderTargetView_ = NULL;

    ID3D11Buffer* pViewMatrixBuffer_ = NULL;
    ID3D11RasterizerState* pRasterizerState_ = NULL;
    ID3D11SamplerState* pSampler_ = NULL;

    ID3D11Texture2D* pDepthBuffer_;
    ID3D11DepthStencilView* pDepthBufferDSV_;
    ID3D11DepthStencilState* pDepthState_[2] = { NULL, NULL };
    ID3D11BlendState* pBlendState_;

    Camera* pCamera_;
    Input* pInput_;

    bool useNormalMap_ = true;
    bool showNormals_ = false;
    std::vector<Light> lights_;

    UINT width_;
    UINT height_;
    UINT numSphereTriangles_;
    float radius_;

    bool isFirst_ = true;
};
