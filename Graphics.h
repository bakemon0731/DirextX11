#pragma once
#include <d3d11.h>
#include <directxmath.h>

class Graphics
{
public:
    Graphics();
    ~Graphics();

    // 初期化、描画開始、描画終了（画面更新）の関数
    bool Initialize(HWND hWnd, UINT width, UINT height);
    void BeginRender(const float clearColor[4]);
    void EndRender();

    // 外部からデバイスやコンテキスト、定数バッファを安全に取得するための関数（Get関数）
    ID3D11Device* GetDevice() { return m_pDevice; }
    ID3D11DeviceContext* GetContext() { return m_pImmediateContext; }
    ID3D11Buffer* GetConstantBuffer() { return m_pConstantBuffer; }

private:
    ID3D11Device* m_pDevice;
    ID3D11DeviceContext* m_pImmediateContext;
    IDXGISwapChain* m_pSwapChain;
    ID3D11RenderTargetView* m_pRenderTargetView;

    ID3D11Texture2D* m_pDepthStencilTexture;
    ID3D11DepthStencilView* m_pDepthStencilView;
    ID3D11DepthStencilState* m_pDepthStencilState;
    ID3D11RasterizerState* m_pRasterizerState;

    ID3D11Buffer* m_pConstantBuffer;
};
