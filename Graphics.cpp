#include "Graphics.h"
#include <d3dcompiler.h>

Graphics::Graphics() :
    m_pDevice(nullptr), m_pImmediateContext(nullptr), m_pSwapChain(nullptr), m_pRenderTargetView(nullptr),
    m_pDepthStencilTexture(nullptr), m_pDepthStencilView(nullptr), m_pDepthStencilState(nullptr),
    m_pRasterizerState(nullptr), m_pConstantBuffer(nullptr)
{
}

Graphics::~Graphics()
{
    if (m_pDepthStencilState)   m_pDepthStencilState->Release();
    if (m_pDepthStencilView)    m_pDepthStencilView->Release();
    if (m_pDepthStencilTexture) m_pDepthStencilTexture->Release();
    if (m_pRasterizerState)     m_pRasterizerState->Release();
    if (m_pConstantBuffer)      m_pConstantBuffer->Release();
    if (m_pRenderTargetView)    m_pRenderTargetView->Release();
    if (m_pSwapChain)           m_pSwapChain->Release();
    if (m_pImmediateContext)    m_pImmediateContext->Release();
    if (m_pDevice)              m_pDevice->Release();
}

bool Graphics::Initialize(HWND hWnd, UINT width, UINT height)
{
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1, D3D11_SDK_VERSION, &sd, &m_pSwapChain, &m_pDevice, nullptr, &m_pImmediateContext);
    if (FAILED(hr)) return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    m_pDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    pBackBuffer->Release();

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = width; td.Height = height; td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    m_pDevice->CreateTexture2D(&td, nullptr, &m_pDepthStencilTexture);
    m_pDevice->CreateDepthStencilView(m_pDepthStencilTexture, nullptr, &m_pDepthStencilView);

    D3D11_DEPTH_STENCIL_DESC dsd = {};
    dsd.DepthEnable = TRUE; dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; dsd.DepthFunc = D3D11_COMPARISON_LESS;
    m_pDevice->CreateDepthStencilState(&dsd, &m_pDepthStencilState);
    m_pImmediateContext->OMSetDepthStencilState(m_pDepthStencilState, 1);
    m_pImmediateContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

    D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
    m_pImmediateContext->RSSetViewports(1, &vp);

    D3D11_RASTERIZER_DESC rd = {};
    rd.CullMode = D3D11_CULL_NONE; rd.FillMode = D3D11_FILL_SOLID;
    m_pDevice->CreateRasterizerState(&rd, &m_pRasterizerState);
    m_pImmediateContext->RSSetState(m_pRasterizerState);

    D3D11_BUFFER_DESC cbd = {};
    cbd.Usage = D3D11_USAGE_DEFAULT;
    cbd.ByteWidth = sizeof(DirectX::XMMATRIX) * 3; // 行列3個分
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    m_pDevice->CreateBuffer(&cbd, nullptr, &m_pConstantBuffer);

    return true;
}

void Graphics::BeginRender(const float clearColor[4])
{
    m_pImmediateContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);
    m_pImmediateContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void Graphics::EndRender()
{
    m_pSwapChain->Present(1, 0);
}
