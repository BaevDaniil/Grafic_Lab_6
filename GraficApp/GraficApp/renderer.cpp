#include "renderer.h"
#include "Renderer.h"

#define SAFE_RELEASE(A) if ((A) != NULL) { (A)->Release(); (A) = NULL; }

Renderer& Renderer::GetInstance() {
    static Renderer instance;
    return instance;
}

Renderer::Renderer() :
    pDevice_(NULL),
    pDeviceContext_(NULL),
    pSwapChain_(NULL),
    pRenderTargetView_(NULL),
    pRasterizerState_(NULL),
    pSampler_(NULL),
    pCamera_(NULL),
    pInput_(NULL),
    pFrustum_(NULL),
    pDepthBuffer_(NULL),
    pDepthBufferDSV_(NULL),
    pBlendState_(NULL),
    skybox_(NULL),
    width_(defaultWidth),
    height_(defaultHeight)
    //numSphereTriangles_(0),
    //radius_(1.0) 
    {}

bool Renderer::Init(HINSTANCE hInstance, HWND hWnd) {
    // Create a DirectX graphics interface factory.​
    IDXGIFactory* pFactory = nullptr;
    HRESULT result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    // Select hardware adapter
    IDXGIAdapter* pSelectedAdapter = NULL;
    if (SUCCEEDED(result)) {
        IDXGIAdapter* pAdapter = NULL;
        UINT adapterIdx = 0;
        while (SUCCEEDED(pFactory->EnumAdapters(adapterIdx, &pAdapter))) {
            DXGI_ADAPTER_DESC desc;
            pAdapter->GetDesc(&desc);
            if (wcscmp(desc.Description, L"Microsoft Basic Render Driver")) {
                pSelectedAdapter = pAdapter;
                break;
            }
            pAdapter->Release();
            adapterIdx++;
        }
    }
    if (pSelectedAdapter == NULL) {
        SAFE_RELEASE(pFactory);
        return false;
    }

    // Create DirectX11 pDevice_
    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
    UINT flags = 0;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG
    result = D3D11CreateDevice(
        pSelectedAdapter,
        D3D_DRIVER_TYPE_UNKNOWN,
        NULL,
        flags,
        levels,
        1,
        D3D11_SDK_VERSION,
        &pDevice_,
        &level,
        &pDeviceContext_
    );
    if (D3D_FEATURE_LEVEL_11_0 != level || !SUCCEEDED(result)) {
        SAFE_RELEASE(pFactory);
        SAFE_RELEASE(pSelectedAdapter);
        Cleanup();
        return false;
    }

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
    swapChainDesc.BufferCount = 2;
    swapChainDesc.BufferDesc.Width = defaultWidth;
    swapChainDesc.BufferDesc.Height = defaultHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = true;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = 0;
    result = pFactory->CreateSwapChain(pDevice_, &swapChainDesc, &pSwapChain_);

    ID3D11Texture2D* pBackBuffer = NULL;
    if (SUCCEEDED(result)) {
        result = pSwapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    }
    if (SUCCEEDED(result)) {
        result = pDevice_->CreateRenderTargetView(pBackBuffer, NULL, &pRenderTargetView_);
    }
    if (SUCCEEDED(result)) {
        result = InitScene();
    }
    SAFE_RELEASE(pFactory);
    SAFE_RELEASE(pSelectedAdapter);
    SAFE_RELEASE(pBackBuffer);
    if (SUCCEEDED(result)) {
        pCamera_ = new Camera;
        if (!pCamera_) {
            result = S_FALSE;
        }
    }
    if (SUCCEEDED(result)) {
        pInput_ = new Input;
        if (!pInput_) {
            result = S_FALSE;
        }
    }
    if (SUCCEEDED(result)) {
        pFrustum_ = new Frustum(SCREEN_NEAR);
        if (!pFrustum_) {
            result = S_FALSE;
        }
    }
    if (SUCCEEDED(result)) {
        result = pInput_->Init(hInstance, hWnd);
    }
    if (SUCCEEDED(result)) {
        result = InitRenderTexture(defaultWidth, defaultHeight);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(pDevice_, pDeviceContext_);

    if (FAILED(result)) {
        Cleanup();
    }

    return SUCCEEDED(result);
}

HRESULT Renderer::InitRenderTexture(int textureWidth, int textureHeight) {
    D3D11_TEXTURE2D_DESC textureDesc;
    ZeroMemory(&textureDesc, sizeof(textureDesc));

    textureDesc.Width = textureWidth;
    textureDesc.Height = textureHeight;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;

    HRESULT result = pDevice_->CreateTexture2D(&textureDesc, NULL, &pRenderTargetTexture_);
    if (FAILED(result)) {
        return result;
    }

    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
    renderTargetViewDesc.Format = textureDesc.Format;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;

    result = pDevice_->CreateRenderTargetView(pRenderTargetTexture_, &renderTargetViewDesc, &pPostEffectRenderTargetView_);
    if (FAILED(result)) {
        return result;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    shaderResourceViewDesc.Format = textureDesc.Format;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
    shaderResourceViewDesc.Texture2D.MipLevels = 1;

    result = pDevice_->CreateShaderResourceView(pRenderTargetTexture_, &shaderResourceViewDesc, &pShaderResourceView_);
    if (FAILED(result)) {
        return result;
    }

    return S_OK;
}

HRESULT Renderer::InitScene() {
    HRESULT result;

    for (int i = 0; i < MAX_CUBE; i++) {
        Cube tmp;
        float textureIndex = (float)(rand() % 2);
        tmp.pos = XMFLOAT4((float)(rand() % 12 - 6), (float)(rand() % 12 - 6), (float)(rand() % 12 - 6), 1.0f);
        tmp.shineSpeedIdNM = XMFLOAT4(5.0f, (float)(rand() % 5), textureIndex, textureIndex > 0.0f ? 0.0f : 1.0f);
        cubes_.push_back(tmp);
    }

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
    static const D3D11_INPUT_ELEMENT_DESC InputDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    static const USHORT IndicesT[] = {
        0, 2, 1, 0, 3, 2
    };
    static const D3D11_INPUT_ELEMENT_DESC InputDescT[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    skybox_ = new SkyBox;
    result = skybox_->createGeometry(pDevice_);
    if (SUCCEEDED(result)){
        skybox_->createShaders(pDevice_);
    }
    if (SUCCEEDED(result)) {
        skybox_->createTextures(pDevice_);
    }
    if (SUCCEEDED(result)) {
        skybox_->setRasterizerState(pDevice_, D3D11_CULL_NONE);
    }


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

    result = pDevice_->CreateBuffer(&desc, &data, &pVertexBuffer_[0]);

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

        result = pDevice_->CreateBuffer(&desc, &data, &pIndexBuffer_[0]);
    }

    ID3D10Blob* vertexShaderBuffer = nullptr;
    ID3D10Blob* pixelShaderBuffer = nullptr;
    int flags = 0;
#ifdef _DEBUG
    flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    D3DInclude includeObj;

    if (SUCCEEDED(result)) {
        result = D3DCompileFromFile(L"VS.hlsl", NULL, &includeObj, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, NULL);
        if (SUCCEEDED(result)) {
            result = pDevice_->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &pVertexShader_[0]);
        }
    }
    if (SUCCEEDED(result)) {
        result = D3DCompileFromFile(L"PS.hlsl", NULL, &includeObj, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
        if (SUCCEEDED(result)) {
            result = pDevice_->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pPixelShader_[0]);
        }
    }
    if (SUCCEEDED(result)) {
        int numElements = sizeof(InputDesc) / sizeof(InputDesc[0]);
        result = pDevice_->CreateInputLayout(InputDesc, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &pInputLayout_[0]);
    }

    SAFE_RELEASE(vertexShaderBuffer);
    SAFE_RELEASE(pixelShaderBuffer);

    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(GeomBuffer) * MAX_CUBE;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        GeomBuffer geomBufferInst[MAX_CUBE];
        for (int i = 0; i < cubes_.size(); i++) {
            geomBufferInst[i].worldMatrix = XMMatrixTranslation(cubes_[i].pos.x, cubes_[i].pos.y, cubes_[i].pos.z);
            geomBufferInst[i].norm = geomBufferInst[i].worldMatrix;
            geomBufferInst[i].shineSpeedTexIdNM = cubes_[i].shineSpeedIdNM;
        }

        D3D11_SUBRESOURCE_DATA data;
        data.pSysMem = &geomBufferInst;
        data.SysMemPitch = sizeof(geomBufferInst);
        data.SysMemSlicePitch = 0;

        result = pDevice_->CreateBuffer(&desc, &data, &pGeomBufferInst_);

        desc.ByteWidth = sizeof(TransparentWorldMatrixBuffer);
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        TransparentWorldMatrixBuffer worldMatrixBuffer;

        data.pSysMem = &worldMatrixBuffer;
        data.SysMemPitch = sizeof(worldMatrixBuffer);
        data.SysMemSlicePitch = 0;
        if (SUCCEEDED(result)) {
            worldMatrixBuffer.worldMatrix = TransparentMatrixs[0];
            worldMatrixBuffer.color = XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
            result = pDevice_->CreateBuffer(&desc, &data, &pPlanesWorldMatrixBuffer_[0]);
        }
        if (SUCCEEDED(result)) {
            worldMatrixBuffer.worldMatrix = TransparentMatrixs[1];
            worldMatrixBuffer.color = XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
            result = pDevice_->CreateBuffer(&desc, &data, &pPlanesWorldMatrixBuffer_[1]);
        }
    }
    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(SceneBuffer);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = pDevice_->CreateBuffer(&desc, nullptr, &pViewMatrixBuffer_[0]);
    }
    if (SUCCEEDED(result)) {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(LightBuffer);
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        desc.StructureByteStride = 0;

        result = pDevice_->CreateBuffer(&desc, nullptr, &pLightBuffer_);
    }
    {
        if (SUCCEEDED(result)) {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = sizeof(VerticesT);
            desc.Usage = D3D11_USAGE_IMMUTABLE;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = &VerticesT;
            data.SysMemPitch = sizeof(VerticesT);
            data.SysMemSlicePitch = 0;

            result = pDevice_->CreateBuffer(&desc, &data, &pVertexBuffer_[2]);
        }
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

            result = pDevice_->CreateBuffer(&desc, &data, &pIndexBuffer_[2]);
        }

        ID3D10Blob* vertexShaderBuffer = nullptr;
        ID3D10Blob* pixelShaderBuffer = nullptr;
        int flags = 0;
#ifdef _DEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        D3D_SHADER_MACRO Shader_Macros[] = { {"USE_LIGHTS"}, {NULL, NULL} };

        if (SUCCEEDED(result)) {
            result = D3DCompileFromFile(L"TVS.hlsl", NULL, &includeObj, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, NULL);
            if (SUCCEEDED(result)) {
                result = pDevice_->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &pVertexShader_[2]);
            }
        }
        if (SUCCEEDED(result)) {
            result = D3DCompileFromFile(L"TPS.hlsl", Shader_Macros, &includeObj, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
            if (SUCCEEDED(result)) {
                result = pDevice_->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pPixelShader_[2]);
            }
        }
        if (SUCCEEDED(result)) {
            int numElements = sizeof(InputDescT) / sizeof(InputDescT[0]);
            result = pDevice_->CreateInputLayout(InputDescT, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &pInputLayout_[2]);
        }

        SAFE_RELEASE(vertexShaderBuffer);
        SAFE_RELEASE(pixelShaderBuffer);
    }
    {
        ID3D10Blob* vertexShaderBuffer = nullptr;
        ID3D10Blob* pixelShaderBuffer = nullptr;
        int flags = 0;
#ifdef _DEBUG
        flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        if (SUCCEEDED(result)) {
            result = D3DCompileFromFile(L"PostEffectVS.hlsl", NULL, NULL, "main", "vs_5_0", flags, 0, &vertexShaderBuffer, NULL);
            if (SUCCEEDED(result)) {
                result = pDevice_->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &pPostEffectVertexShader_);
            }
        }
        if (SUCCEEDED(result)) {
            result = D3DCompileFromFile(L"PostEffectPS.hlsl", NULL, NULL, "main", "ps_5_0", flags, 0, &pixelShaderBuffer, NULL);
            if (SUCCEEDED(result)) {
                result = pDevice_->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &pPostEffectPixelShader_);
            }
        }

        SAFE_RELEASE(vertexShaderBuffer);
        SAFE_RELEASE(pixelShaderBuffer);

        if (SUCCEEDED(result)) {
            D3D11_SAMPLER_DESC samplerDesc;
            ZeroMemory(&samplerDesc, sizeof(samplerDesc));
            samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            samplerDesc.MinLOD = 0;
            samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
            samplerDesc.MaxAnisotropy = D3D11_MAX_MAXANISOTROPY;

            result = pDevice_->CreateSamplerState(&samplerDesc, &pPostEffectSamplerState_);
        }
        if (SUCCEEDED(result)) {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = sizeof(PostEffectConstantBuffer);
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;
            desc.StructureByteStride = 0;

            PostEffectConstantBuffer postEffectConstantBuffer;
            postEffectConstantBuffer.params = XMINT4(withPostEffect_, 0, 0, 0);

            D3D11_SUBRESOURCE_DATA data;
            data.pSysMem = &postEffectConstantBuffer;
            data.SysMemPitch = sizeof(postEffectConstantBuffer);
            data.SysMemSlicePitch = 0;

            result = pDevice_->CreateBuffer(&desc, &data, &pPostEffectConstantBuffer_);
        }
    }
    if (SUCCEEDED(result)) {
        D3D11_RASTERIZER_DESC desc = {};
        desc.AntialiasedLineEnable = false;
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.DepthBias = 0;
        desc.DepthBiasClamp = 0.0f;
        desc.FrontCounterClockwise = false;
        desc.DepthClipEnable = true;
        desc.ScissorEnable = false;
        desc.MultisampleEnable = false;
        desc.SlopeScaledDepthBias = 0.0f;

        result = pDevice_->CreateRasterizerState(&desc, &pRasterizerState_);
    }
    if (SUCCEEDED(result)) {
        std::vector<const wchar_t*> filenames = { L"textures/156.dds", L"textures/198.dds" };
        UINT textureCount = (UINT)filenames.size();

        std::vector<ID3D11Texture2D*> textures(textureCount);

        for (UINT i = 0; i < textureCount; ++i) {
            result = DirectX::CreateDDSTextureFromFile(pDevice_, pDeviceContext_, filenames[i], (ID3D11Resource**)(&textures[i]), nullptr);
        }
        if (FAILED(result)) {
            return result;
        }

        D3D11_TEXTURE2D_DESC textureDesc;
        textures[0]->GetDesc(&textureDesc);

        D3D11_TEXTURE2D_DESC arrayDesc;
        arrayDesc.Width = textureDesc.Width;
        arrayDesc.Height = textureDesc.Height;
        arrayDesc.MipLevels = textureDesc.MipLevels;
        arrayDesc.ArraySize = textureCount;
        arrayDesc.Format = textureDesc.Format;
        arrayDesc.SampleDesc.Count = 1;
        arrayDesc.SampleDesc.Quality = 0;
        arrayDesc.Usage = D3D11_USAGE_DEFAULT;
        arrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        arrayDesc.CPUAccessFlags = 0;
        arrayDesc.MiscFlags = 0;

        ID3D11Texture2D* textureArray = nullptr;
        result = pDevice_->CreateTexture2D(&arrayDesc, 0, &textureArray);
        if (FAILED(result)) {
            return result;
        }

        for (UINT texElement = 0; texElement < textureCount; ++texElement) {
            for (UINT mipLevel = 0; mipLevel < textureDesc.MipLevels; ++mipLevel) {
                const int sourceSubresource = D3D11CalcSubresource(mipLevel, 0, textureDesc.MipLevels);
                const int destSubresource = D3D11CalcSubresource(mipLevel, texElement, textureDesc.MipLevels);
                pDeviceContext_->CopySubresourceRegion(textureArray, destSubresource, 0, 0, 0, textures[texElement], sourceSubresource, nullptr);
            }
        }

        D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
        viewDesc.Format = arrayDesc.Format;
        viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        viewDesc.Texture2DArray.MostDetailedMip = 0;
        viewDesc.Texture2DArray.MipLevels = arrayDesc.MipLevels;
        viewDesc.Texture2DArray.FirstArraySlice = 0;
        viewDesc.Texture2DArray.ArraySize = textureCount;

        result = pDevice_->CreateShaderResourceView(textureArray, &viewDesc, &pTexture_[0]);
        if (FAILED(result)) {
            return result;
        }
        textureArray->Release();
        for (UINT i = 0; i < textureCount; ++i) {
            textures[i]->Release();
        }

    }
    if (SUCCEEDED(result)) {
        result = CreateDDSTextureFromFile(pDevice_, pDeviceContext_, L"textures/156_norm.dds", nullptr, &pTexture_[1]);
    }
    if (SUCCEEDED(result)) {
        D3D11_SAMPLER_DESC desc = {};

        desc.Filter = D3D11_FILTER_ANISOTROPIC;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        desc.MinLOD = -D3D11_FLOAT32_MAX;
        desc.MaxLOD = D3D11_FLOAT32_MAX;
        desc.MipLODBias = 0.0f;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 1.0f;

        result = pDevice_->CreateSamplerState(&desc, &pSampler_);
    }
    if (SUCCEEDED(result)) {
        D3D11_DEPTH_STENCIL_DESC dsDesc = { };
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        dsDesc.DepthFunc = D3D11_COMPARISON_GREATER;
        dsDesc.StencilEnable = FALSE;

        result = pDevice_->CreateDepthStencilState(&dsDesc, &pDepthState_[0]);
    }
    if (SUCCEEDED(result)) {
        D3D11_DEPTH_STENCIL_DESC dsDesc = { };
        dsDesc.DepthEnable = TRUE;
        dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL;
        dsDesc.StencilEnable = FALSE;

        result = pDevice_->CreateDepthStencilState(&dsDesc, &pDepthState_[1]);
    }
    if (SUCCEEDED(result)) {
        D3D11_BLEND_DESC desc = { 0 };
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED |
            D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;

        result = pDevice_->CreateBlendState(&desc, &pBlendState_);
    }

    return result;
}

void Renderer::ProcessPostEffect(D3D11_VIEWPORT viewport) {
    pDeviceContext_->OMSetRenderTargets(1, &pRenderTargetView_, nullptr);
    pDeviceContext_->RSSetViewports(1, &viewport);

    pDeviceContext_->IASetInputLayout(nullptr);
    pDeviceContext_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    pDeviceContext_->VSSetShader(pPostEffectVertexShader_, nullptr, 0);
    pDeviceContext_->PSSetShader(pPostEffectPixelShader_, nullptr, 0);
    pDeviceContext_->PSSetConstantBuffers(0, 1, &pPostEffectConstantBuffer_);
    pDeviceContext_->PSSetShaderResources(0, 1, &pShaderResourceView_);
    pDeviceContext_->PSSetSamplers(0, 1, &pPostEffectSamplerState_);

    pDeviceContext_->Draw(3, 0);

    ID3D11ShaderResourceView* nullsrv[] = { nullptr };
    pDeviceContext_->PSSetShaderResources(0, 1, nullsrv);
}

void Renderer::InputHandler() {
    XMFLOAT3 mouse = pInput_->ReadMouse();
    pCamera_->Rotate(mouse.x / 200.0f, mouse.y / 200.0f);
    pCamera_->Zoom(-mouse.z / 100.0f);

    unsigned char* keyboard = pInput_->ReadKeyboard();
    if (nullptr == keyboard)
        return;
    float di = 0.0, dj = 0.0, dz = 0.0;

    if (keyboard[DIK_UP] || keyboard[DIK_W])
        dj += 0.05f;
    if (keyboard[DIK_DOWN] || keyboard[DIK_S])
        dj -= 0.05f;
    if (keyboard[DIK_LEFT] || keyboard[DIK_A])
        di += 0.05f;
    if (keyboard[DIK_RIGHT] || keyboard[DIK_D])
        di -= 0.05f;
    if (keyboard[DIK_LCONTROL])
        dz -= 0.05f;
    if (keyboard[DIK_LSHIFT])
        dz += 0.05f;

    pCamera_->Move(di, dj, dz);
}

bool Renderer::UpdateScene() {
    HRESULT result;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static bool window = true;
    static bool window2 = true;

    if (window) {
        ImGui::Begin("Lights", &window);

        ImGui::Checkbox("Use normal maps", &useNormalMap_);
        ImGui::Checkbox("Show normals", &showNormals_);
        if (ImGui::Checkbox("Post effect", &withPostEffect_)) {
            PostEffectConstantBuffer postEffectConstantBuffer;
            postEffectConstantBuffer.params = XMINT4(withPostEffect_, 0, 0, 0);
            pDeviceContext_->UpdateSubresource(pPostEffectConstantBuffer_, 0, nullptr, &postEffectConstantBuffer, 0, 0);
        }

        if (ImGui::Button("+")) {
            if (lights_.size() < MAX_LIGHT)
                lights_.push_back({ XMFLOAT4((float)(rand() % 12 - 6), (float)(rand() % 12 - 6), (float)(rand() % 12 - 6), 0.0f),
                    XMFLOAT4((rand() % 255) / 255.0f, (rand() % 255) / 255.0f, (rand() % 255) / 255.0f, 0.0f) });
        }
        ImGui::SameLine();
        if (ImGui::Button("-")) {
            if (lights_.size() > 0)
                lights_.pop_back();
        }

        static float col[MAX_LIGHT][3];
        static float pos[MAX_LIGHT][4];
        for (int i = 0; i < lights_.size(); i++) {
            std::string str = "Light " + std::to_string(i);
            ImGui::Text(str.c_str());

            pos[i][0] = lights_[i].pos.x;
            pos[i][1] = lights_[i].pos.y;
            pos[i][2] = lights_[i].pos.z;
            str = "Pos " + std::to_string(i);
            ImGui::Text(str.c_str());
            ImGui::DragFloat3(str.c_str(), pos[i], 0.1f, -6.0f, 6.0f);
            lights_[i].pos = XMFLOAT4(pos[i][0], pos[i][1], pos[i][2], 1.0f);

            col[i][0] = lights_[i].color.x;
            col[i][1] = lights_[i].color.y;
            col[i][2] = lights_[i].color.z;
            str = "Color " + std::to_string(i);
            ImGui::ColorEdit3(str.c_str(), col[i]);
            lights_[i].color = XMFLOAT4(col[i][0], col[i][1], col[i][2], 1.0f);
        }

        ImGui::End();
    }
    if (window2) {
        ImGui::Begin("Instances", &window2);

        if (ImGui::Button("+")) {
            if (cubesCount_ < MAX_CUBE) {
                ++cubesCount_;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("-")) {
            if (cubesCount_ > 0) {
                --cubesCount_;
            }
        }

        std::string str = "Count: " + std::to_string(cubesCount_);
        ImGui::Text(str.c_str());
        str = "Rendered: " + std::to_string(cubeIndexies_.size());
        ImGui::Text(str.c_str());
        ImGui::Checkbox("Culling", &withCulling_);

        ImGui::End();
    }

    InputHandler();

    XMMATRIX mView = pCamera_->GetViewMatrix();

    XMMATRIX mProjection = XMMatrixPerspectiveFovLH(XM_PI / 3, width_ / (FLOAT)height_, SCREEN_FAR, SCREEN_NEAR);

    static float t = 0.0f;
    static ULONGLONG timeStart = 0;
    ULONGLONG timeCur = GetTickCount64();
    if (timeStart == 0) {
        timeStart = timeCur;
    }
    t = (timeCur - timeStart) / 1000.0f;

    GeomBuffer geomBufferInst[MAX_CUBE];
    for (int i = 0; i < cubesCount_; i++) {
        geomBufferInst[i].worldMatrix = XMMatrixRotationY(cubes_[i].pos.w * t * cubes_[i].shineSpeedIdNM.y) * XMMatrixTranslation(cubes_[i].pos.x, cubes_[i].pos.y, cubes_[i].pos.z);
        geomBufferInst[i].norm = geomBufferInst[i].worldMatrix;
        geomBufferInst[i].shineSpeedTexIdNM = cubes_[i].shineSpeedIdNM;
    }

    pDeviceContext_->UpdateSubresource(pGeomBufferInst_, 0, nullptr, &geomBufferInst, 0, 0);

    pFrustum_->ConstructFrustum(mView, mProjection);
    cubeIndexies_.clear();
    for (int i = 0; i < cubesCount_; i++) {
        XMFLOAT4 min, max;
        XMStoreFloat4(&min, XMVector4Transform(XMLoadFloat4(&AABB[0]), geomBufferInst[i].worldMatrix));
        XMStoreFloat4(&max, XMVector4Transform(XMLoadFloat4(&AABB[1]), geomBufferInst[i].worldMatrix));
        if (!withCulling_ || pFrustum_->CheckRectangle(max.x, max.y, max.z, min.x, min.y, min.z)) {
            cubeIndexies_.push_back(i);
        }
    }

    XMFLOAT3 cameraPos = pCamera_->GetPosition();
    D3D11_MAPPED_SUBRESOURCE subresource;
    result = pDeviceContext_->Map(pViewMatrixBuffer_[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    if (SUCCEEDED(result)) {
        SceneBuffer& sceneBuffer = *reinterpret_cast<SceneBuffer*>(subresource.pData);
        sceneBuffer.viewProjectionMatrix = XMMatrixMultiply(mView, mProjection);
        for (int i = 0; i < cubeIndexies_.size(); i++) {
            sceneBuffer.indexBuffer[i] = XMINT4(cubeIndexies_[i], 0, 0, 0);
        }
        pDeviceContext_->Unmap(pViewMatrixBuffer_[0], 0);
    }
    result = pDeviceContext_->Map(pLightBuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
    if (SUCCEEDED(result)) {
        LightBuffer& lightBuffer = *reinterpret_cast<LightBuffer*>(subresource.pData);
        lightBuffer.cameraPos = XMFLOAT4(cameraPos.x, cameraPos.y, cameraPos.z, 1.0f);
        lightBuffer.ambientColor = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
        lightBuffer.lightParams = XMINT4(int(lights_.size()), (int)useNormalMap_, (int)showNormals_, 0);
        for (int i = 0; i < lights_.size(); i++) {
            lightBuffer.lights[i].pos = lights_[i].pos;
            lightBuffer.lights[i].color = lights_[i].color;
        }
        pDeviceContext_->Unmap(pLightBuffer_, 0);
    }

    if (SUCCEEDED(result)) {
        skybox_->update(pDeviceContext_, pCamera_, mProjection);
    }

    ImGui::Render();

    XMFLOAT4 rectVert[4];
    float maxDist = -D3D11_FLOAT32_MAX;
    for (int i = 0; i < 4; i++) {
        rectVert[i] = XMFLOAT4(VerticesT[i].x, VerticesT[i].y, VerticesT[i].z, 1.0f);
    }
    for (int i = 0; i < 4; i++) {
        XMStoreFloat4(&rectVert[i], XMVector4Transform(XMLoadFloat4(&rectVert[i]), TransparentMatrixs[0]));
        float dist = (rectVert[i].x - cameraPos.x) * (rectVert[i].x - cameraPos.x) +
            (rectVert[i].y - cameraPos.y) * (rectVert[i].y - cameraPos.y) +
            (rectVert[i].z - cameraPos.z) * (rectVert[i].z - cameraPos.z);
        maxDist = max(maxDist, dist);
    }
    float maxDist2 = -D3D11_FLOAT32_MAX;
    for (int i = 0; i < 4; i++) {
        rectVert[i] = XMFLOAT4(VerticesT[i].x, VerticesT[i].y, VerticesT[i].z, 1.0f);
    }
    for (int i = 0; i < 4; i++) {
        XMStoreFloat4(&rectVert[i], XMVector4Transform(XMLoadFloat4(&rectVert[i]), TransparentMatrixs[1]));
        float dist = (rectVert[i].x - cameraPos.x) * (rectVert[i].x - cameraPos.x) +
            (rectVert[i].y - cameraPos.y) * (rectVert[i].y - cameraPos.y) +
            (rectVert[i].z - cameraPos.z) * (rectVert[i].z - cameraPos.z);
        maxDist2 = max(maxDist2, dist);
    }
    isFirst_ = maxDist2 < maxDist;

    return SUCCEEDED(result);
}

bool Renderer::Render() {
    if (!UpdateScene())
        return false;

    pDeviceContext_->ClearState();

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)width_;
    viewport.Height = (FLOAT)height_;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    pDeviceContext_->RSSetViewports(1, &viewport);

    D3D11_RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = width_;
    rect.bottom = height_;
    pDeviceContext_->RSSetScissorRects(1, &rect);

    pDeviceContext_->OMSetRenderTargets(1, &pPostEffectRenderTargetView_, pDepthBufferDSV_);
    static const FLOAT color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    pDeviceContext_->ClearRenderTargetView(pPostEffectRenderTargetView_, color);
    pDeviceContext_->ClearDepthStencilView(pDepthBufferDSV_, D3D11_CLEAR_DEPTH, 0.0f, 0);

    pDeviceContext_->RSSetState(pRasterizerState_);
    pDeviceContext_->OMSetDepthStencilState(pDepthState_[0], 0);

    ID3D11ShaderResourceView* resources[] = { pTexture_[0], pTexture_[1] };
    pDeviceContext_->PSSetShaderResources(0, 2, resources);

    ID3D11SamplerState* samplers[] = { pSampler_ };
    pDeviceContext_->PSSetSamplers(0, 1, samplers);

    pDeviceContext_->IASetIndexBuffer(pIndexBuffer_[0], DXGI_FORMAT_R16_UINT, 0);
    ID3D11Buffer* vertexBuffers[] = { pVertexBuffer_[0] };
    UINT strides[] = { sizeof(Vertex) };
    UINT offsets[] = { 0 };
    pDeviceContext_->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
    pDeviceContext_->IASetInputLayout(pInputLayout_[0]);
    pDeviceContext_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pDeviceContext_->VSSetConstantBuffers(0, 1, &pGeomBufferInst_);
    pDeviceContext_->VSSetConstantBuffers(1, 1, &pViewMatrixBuffer_[0]);
    pDeviceContext_->VSSetConstantBuffers(2, 1, &pLightBuffer_);
    pDeviceContext_->VSSetShader(pVertexShader_[0], nullptr, 0);
    pDeviceContext_->PSSetShader(pPixelShader_[0], nullptr, 0);
    pDeviceContext_->PSSetConstantBuffers(0, 1, &pGeomBufferInst_);
    pDeviceContext_->PSSetConstantBuffers(1, 1, &pViewMatrixBuffer_[0]);
    pDeviceContext_->PSSetConstantBuffers(2, 1, &pLightBuffer_);
    pDeviceContext_->DrawIndexedInstanced(36, (UINT)cubeIndexies_.size(), 0, 0, 0);

    pDeviceContext_->OMSetDepthStencilState(pDepthState_[1], 0);
    skybox_->draw(pDeviceContext_);

    {
        pDeviceContext_->IASetIndexBuffer(pIndexBuffer_[2], DXGI_FORMAT_R16_UINT, 0);
        ID3D11Buffer* vertexBuffers[] = { pVertexBuffer_[2] };
        UINT strides[] = { 12 };
        UINT offsets[] = { 0 };
        pDeviceContext_->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
        pDeviceContext_->IASetInputLayout(pInputLayout_[2]);

        pDeviceContext_->VSSetShader(pVertexShader_[2], nullptr, 0);
        pDeviceContext_->PSSetShader(pPixelShader_[2], nullptr, 0);
        pDeviceContext_->VSSetConstantBuffers(1, 1, &pViewMatrixBuffer_[0]);

        pDeviceContext_->OMSetBlendState(pBlendState_, nullptr, 0xFFFFFFFF);

        if (isFirst_) {
            pDeviceContext_->VSSetConstantBuffers(0, 1, &pPlanesWorldMatrixBuffer_[0]);
            pDeviceContext_->PSSetConstantBuffers(0, 1, &pPlanesWorldMatrixBuffer_[0]);
            pDeviceContext_->DrawIndexed(6, 0, 0);

            pDeviceContext_->VSSetConstantBuffers(0, 1, &pPlanesWorldMatrixBuffer_[1]);
            pDeviceContext_->PSSetConstantBuffers(0, 1, &pPlanesWorldMatrixBuffer_[1]);
            pDeviceContext_->DrawIndexed(6, 0, 0);
        }
        else {
            pDeviceContext_->VSSetConstantBuffers(0, 1, &pPlanesWorldMatrixBuffer_[1]);
            pDeviceContext_->PSSetConstantBuffers(0, 1, &pPlanesWorldMatrixBuffer_[1]);
            pDeviceContext_->DrawIndexed(6, 0, 0);

            pDeviceContext_->VSSetConstantBuffers(0, 1, &pPlanesWorldMatrixBuffer_[0]);
            pDeviceContext_->PSSetConstantBuffers(0, 1, &pPlanesWorldMatrixBuffer_[0]);
            pDeviceContext_->DrawIndexed(6, 0, 0);
        }
    }

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ID3D11RenderTargetView* views[] = { pRenderTargetView_ };
    pDeviceContext_->OMSetRenderTargets(1, views, pDepthBufferDSV_);

    static const FLOAT backColor[4] = { 0.4f, 0.2f, 0.4f, 1.0f };
    pDeviceContext_->ClearRenderTargetView(pRenderTargetView_, backColor);
    pDeviceContext_->ClearDepthStencilView(pDepthBufferDSV_, D3D11_CLEAR_DEPTH, 0.0f, 0);

    ProcessPostEffect(viewport);

    HRESULT result = pSwapChain_->Present(0, 0);

    return SUCCEEDED(result);
}

bool Renderer::Resize(UINT width, UINT height) {
    if (pSwapChain_ == NULL)
        return false;

    SAFE_RELEASE(pRenderTargetView_);

    width_ = max(width, 8);
    height_ = max(height, 8);

    auto result = pSwapChain_->ResizeBuffers(2, width_, height_, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
    if (!SUCCEEDED(result))
        return false;

    ID3D11Texture2D* pBuffer;
    result = pSwapChain_->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer);
    if (!SUCCEEDED(result))
        return false;

    result = pDevice_->CreateRenderTargetView(pBuffer, NULL, &pRenderTargetView_);
    SAFE_RELEASE(pBuffer);
    if (!SUCCEEDED(result))
        return false;

    SAFE_RELEASE(pDepthBuffer_);
    SAFE_RELEASE(pDepthBufferDSV_);
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Format = DXGI_FORMAT_D32_FLOAT;
    desc.ArraySize = 1;
    desc.MipLevels = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.Height = height_;
    desc.Width = width_;
    desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    result = pDevice_->CreateTexture2D(&desc, NULL, &pDepthBuffer_);
    if (!SUCCEEDED(result))
        return false;

    result = pDevice_->CreateDepthStencilView(pDepthBuffer_, NULL, &pDepthBufferDSV_);
    if (!SUCCEEDED(result))
        return false;

    ReleaseRenderTexture();
    result = InitRenderTexture(width_, height_);
    if (!SUCCEEDED(result))
        return false;

    float n = 0.01f;
    float fov = XM_PI / 3;
    float halfW = tanf(fov / 2) * n;
    float halfH = height_ / float(width_) * halfW;
    skybox_->setRadius(sqrtf(n * n + halfH * halfH + halfW * halfW) * 1.1f);

    return true;
}

void Renderer::ReleaseRenderTexture() {
    SAFE_RELEASE(pRenderTargetTexture_);
    SAFE_RELEASE(pPostEffectRenderTargetView_);
    SAFE_RELEASE(pShaderResourceView_);
}

void Renderer::Cleanup() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (pDeviceContext_ != NULL)
        pDeviceContext_->ClearState();

    SAFE_RELEASE(pRenderTargetView_);
    SAFE_RELEASE(pDeviceContext_);
    SAFE_RELEASE(pSwapChain_);
    SAFE_RELEASE(pRasterizerState_);
    SAFE_RELEASE(pSampler_);
    SAFE_RELEASE(pDepthBuffer_);
    SAFE_RELEASE(pDepthBufferDSV_);
    SAFE_RELEASE(pBlendState_);
    SAFE_RELEASE(pLightBuffer_);
    SAFE_RELEASE(pGeomBufferInst_);

    SAFE_RELEASE(pVertexBuffer_[0]);
    SAFE_RELEASE(pVertexBuffer_[1]);
    SAFE_RELEASE(pVertexBuffer_[2]);

    SAFE_RELEASE(pIndexBuffer_[0]);
    SAFE_RELEASE(pIndexBuffer_[1]);
    SAFE_RELEASE(pIndexBuffer_[2]);

    SAFE_RELEASE(pInputLayout_[0]);
    SAFE_RELEASE(pInputLayout_[1]);
    SAFE_RELEASE(pInputLayout_[2]);

    SAFE_RELEASE(pVertexShader_[0]);
    SAFE_RELEASE(pVertexShader_[1]);
    SAFE_RELEASE(pVertexShader_[2]);

    SAFE_RELEASE(pPixelShader_[0]);
    SAFE_RELEASE(pPixelShader_[1]);
    SAFE_RELEASE(pPixelShader_[2]);

    SAFE_RELEASE(pViewMatrixBuffer_[0]);
    SAFE_RELEASE(pViewMatrixBuffer_[1]);

    //SAFE_RELEASE(pSkyboxWorldMatrixBuffer_);
    SAFE_RELEASE(pPlanesWorldMatrixBuffer_[0]);
    SAFE_RELEASE(pPlanesWorldMatrixBuffer_[1]);

    SAFE_RELEASE(pTexture_[0]);
    SAFE_RELEASE(pTexture_[1]);
    SAFE_RELEASE(pTexture_[2]);

    SAFE_RELEASE(pDepthState_[0]);
    SAFE_RELEASE(pDepthState_[1]);

    SAFE_RELEASE(pPostEffectVertexShader_);
    SAFE_RELEASE(pPostEffectPixelShader_);
    SAFE_RELEASE(pPostEffectSamplerState_);
    SAFE_RELEASE(pPostEffectConstantBuffer_);

    ReleaseRenderTexture();

    if (pCamera_) {
        delete pCamera_;
        pCamera_ = NULL;
    }
    if (pInput_) {
        delete pInput_;
        pInput_ = NULL;
    }
    if (pFrustum_) {
        delete pFrustum_;
        pFrustum_ = NULL;
    }
    if (skybox_) {
        delete skybox_;
        skybox_ = NULL;
    }

#ifdef _DEBUG
    if (pDevice_ != NULL) {
        ID3D11Debug* d3dDebug = NULL;
        pDevice_->QueryInterface(IID_PPV_ARGS(&d3dDebug));

        UINT references = pDevice_->Release();
        pDevice_ = NULL;
        if (references > 1) {
            d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        }
        SAFE_RELEASE(d3dDebug);
    }
#endif
    SAFE_RELEASE(pDevice_);
}

Renderer::~Renderer() {
    Cleanup();
}