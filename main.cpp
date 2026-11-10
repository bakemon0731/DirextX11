#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h> 
#include <directxmath.h> 
#include <memory>     // std::unique_ptr
#include <string>     // std::wstring
#include <vector>     // std::vector
#include <wincodec.h> // WIC (Windows Imaging Component)
#include "Player.h" 
#include "Enemy.h" 
#include "Graphics.h" 

#pragma comment(lib, "windowscodecs.lib")

using namespace DirectX;

const UINT SCREEN_WIDTH = 800;
const UINT SCREEN_HEIGHT = 600;

ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11PixelShader* g_pPixelShaderEnemy = nullptr; // 敵用ピクセルシェーダー
ID3D11InputLayout* g_pVertexLayout = nullptr;
ID3D11Buffer* g_pVertexBuffer = nullptr;
ID3D11Buffer* g_pIndexBuffer = nullptr;
ID3D11ShaderResourceView* g_pTextureRV = nullptr;  // テクスチャ
ID3D11ShaderResourceView* g_pTextureEnemyRV = nullptr;
ID3D11SamplerState* g_pSamplerState = nullptr;     // サンプラー

struct ConstantBufferData {
    XMMATRIX mWorld;
    XMMATRIX mView;
    XMMATRIX mProjection;
};

// UV座標（tu, tv）を追加した頂点構造体
struct SimpleVertex {
    float x, y, z;
    float nx, ny, nz;
    float tu, tv;
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void RenderText(HWND hWnd, IDXGISwapChain* swapChain, const wchar_t* text, int top, int size, COLORREF color) {
    HDC hdc = GetDC(hWnd);
    HFONT hFont = CreateFontW(size, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, color);
    RECT rect = { 0, top, SCREEN_WIDTH, top + size + 20 };
    DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_NOCLIP);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    ReleaseDC(hWnd, hdc);
}

// exeと同じフォルダのパスを返す
std::wstring GetExeDirPath(const wchar_t* fileName) {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring path(buf);
    path = path.substr(0, path.find_last_of(L"\\/") + 1);
    return path + fileName;
}

// シェーダーをコンパイルする関数
bool CompileShader(const wchar_t* fileName, const char* entry, const char* target, ID3DBlob** ppBlob) {
    ID3DBlob* pError = nullptr;
    std::wstring exePath = GetExeDirPath(fileName);
    HRESULT hr = D3DCompileFromFile(exePath.c_str(), nullptr, nullptr, entry, target, 0, 0, ppBlob, &pError);
    if (FAILED(hr)) {
        if (pError) { pError->Release(); pError = nullptr; }
        hr = D3DCompileFromFile(fileName, nullptr, nullptr, entry, target, 0, 0, ppBlob, &pError);
    }

    if (FAILED(hr)) {
        if (pError) {
            MessageBoxA(nullptr, (char*)pError->GetBufferPointer(), "Shader compile error", MB_OK);
            pError->Release();
        }
        else {
            std::wstring msg = L"Shader.hlsl が見つかりません。\n次のフォルダに置いてください:\n" + exePath;
            MessageBoxW(nullptr, msg.c_str(), L"エラー", MB_OK);
        }
        return false;
    }
    if (pError) pError->Release();
    return true;
}

// WICを使って画像ファイルを読み込み、テクスチャを生成する関数
bool LoadTextureFromFile(ID3D11Device* pDevice, const wchar_t* filename, ID3D11ShaderResourceView** ppTextureView) {
    IWICImagingFactory* pFactory = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory));
    if (FAILED(hr)) return false;

    // exeのある場所から探す、なければそのまま
    std::wstring fullPath = GetExeDirPath(filename);
    IWICBitmapDecoder* pDecoder = nullptr;
    hr = pFactory->CreateDecoderFromFilename(fullPath.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    if (FAILED(hr)) {
        hr = pFactory->CreateDecoderFromFilename(filename, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
    }
    if (FAILED(hr)) { pFactory->Release(); return false; }

    IWICBitmapFrameDecode* pFrame = nullptr;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) { pDecoder->Release(); pFactory->Release(); return false; }

    IWICFormatConverter* pConverter = nullptr;
    hr = pFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) { pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    hr = pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeMedianCut);
    if (FAILED(hr)) { pConverter->Release(); pFrame->Release(); pDecoder->Release(); pFactory->Release(); return false; }

    UINT width = 0, height = 0;
    pConverter->GetSize(&width, &height);

    std::vector<BYTE> pixels(width * height * 4);
    hr = pConverter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data());

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA subResource = {};
    subResource.pSysMem = pixels.data();
    subResource.SysMemPitch = width * 4;

    ID3D11Texture2D* pTexture = nullptr;
    hr = pDevice->CreateTexture2D(&desc, &subResource, &pTexture);
    if (SUCCEEDED(hr)) {
        hr = pDevice->CreateShaderResourceView(pTexture, nullptr, ppTextureView);
        pTexture->Release();
    }

    pConverter->Release();
    pFrame->Release();
    pDecoder->Release();
    pFactory->Release();

    return SUCCEEDED(hr);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // WICを使うためのCOM初期化
    CoInitialize(nullptr);

    WNDCLASSEX wcex = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInstance, nullptr, nullptr, nullptr, nullptr, L"DX11Class", nullptr };
    RegisterClassEx(&wcex);
    RECT rc = { 0, 0, (LONG)SCREEN_WIDTH, (LONG)SCREEN_HEIGHT };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    HWND hWnd = CreateWindow(L"DX11Class", L"DirectX 11 統合バージョン", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);
    ShowWindow(hWnd, nCmdShow);

    Graphics graphics;
    if (!graphics.Initialize(hWnd, SCREEN_WIDTH, SCREEN_HEIGHT)) {
        MessageBoxW(nullptr, L"Graphics::Initialize に失敗しました", L"エラー", MB_OK);
        CoUninitialize();
        return 0;
    }

    // ---- 頂点シェーダー ----
    ID3DBlob* pVSBlob = nullptr;
    if (!CompileShader(L"Shader.hlsl", "VS", "vs_4_0", &pVSBlob)) { CoUninitialize(); return 0; }

    HRESULT hr = graphics.GetDevice()->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"CreateVertexShader に失敗しました", L"エラー", MB_OK);
        pVSBlob->Release();
        CoUninitialize();
        return 0;
    }

    // 入力レイアウト（POSITION, NORMAL, TEXCOORD）
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    hr = graphics.GetDevice()->CreateInputLayout(layout, 3, pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &g_pVertexLayout);
    pVSBlob->Release();
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"CreateInputLayout に失敗しました", L"エラー", MB_OK);
        CoUninitialize();
        return 0;
    }

    // ---- ピクセルシェーダー（プレイヤー用） ----
    ID3DBlob* pPSBlob = nullptr;
    if (!CompileShader(L"Shader.hlsl", "PS", "ps_4_0", &pPSBlob)) { CoUninitialize(); return 0; }
    hr = graphics.GetDevice()->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
    pPSBlob->Release();
    if (FAILED(hr)) { MessageBoxW(nullptr, L"CreatePixelShader に失敗しました", L"エラー", MB_OK); CoUninitialize(); return 0; }

    // ---- ピクセルシェーダー（敵用） ----
    ID3DBlob* pPSEnemyBlob = nullptr;
    if (!CompileShader(L"Shader.hlsl", "PS_Enemy", "ps_4_0", &pPSEnemyBlob)) { CoUninitialize(); return 0; }
    hr = graphics.GetDevice()->CreatePixelShader(pPSEnemyBlob->GetBufferPointer(), pPSEnemyBlob->GetBufferSize(), nullptr, &g_pPixelShaderEnemy);
    pPSEnemyBlob->Release();
    if (FAILED(hr)) { MessageBoxW(nullptr, L"CreatePixelShader(Enemy) に失敗しました", L"エラー", MB_OK); CoUninitialize(); return 0; }

    // ---- 頂点データ（UV座標付き立方体） ----
    SimpleVertex vertices[] = {
        { -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f }, {  0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f }, { -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f }, {  0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f },
        {  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f }, { -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f }, {  0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f }, { -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f }, { -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f }, { -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f }, { -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f },
        {  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f }, {  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f }, {  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f }, {  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f },
        { -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f }, {  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f }, { -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f }, {  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f },
        { -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f }, {  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f }, { -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f }, {  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f },
    };

    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DEFAULT; bd.ByteWidth = sizeof(SimpleVertex) * 24; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA InitData = {}; InitData.pSysMem = vertices;
    graphics.GetDevice()->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);

    WORD indices[] = {
        0, 1, 2,   2, 1, 3,   4, 5, 6,   6, 5, 7,   8, 9, 10,  10,9, 11,
        12,13,14,  14,13,15,  16,17,18,  18,17,19,  20,21,22,  22,21,23
    };

    D3D11_BUFFER_DESC ibd = {}; ibd.Usage = D3D11_USAGE_DEFAULT; ibd.ByteWidth = sizeof(WORD) * 36; ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA InitDataIdx = {}; InitDataIdx.pSysMem = indices;
    graphics.GetDevice()->CreateBuffer(&ibd, &InitDataIdx, &g_pIndexBuffer);

    // プレイヤー用テクスチャのロード
    LoadTextureFromFile(graphics.GetDevice(), L"texture.png", &g_pTextureRV);

    // 敵用テクスチャのロード 
    LoadTextureFromFile(graphics.GetDevice(), L"texture_enemy.png", &g_pTextureEnemyRV);



    D3D11_SAMPLER_DESC sampDesc = {};
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    graphics.GetDevice()->CreateSamplerState(&sampDesc, &g_pSamplerState);

    float t = 0.0f;
    int gameState = 0;
    float gameTimer = 10.0f;

    std::unique_ptr<Player> player = std::make_unique<Player>();
    std::unique_ptr<Enemy>  enemy = std::make_unique<Enemy>();

    LARGE_INTEGER frequency;
    LARGE_INTEGER prevTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&prevTime);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            float deltaTime = static_cast<float>(currentTime.QuadPart - prevTime.QuadPart) / static_cast<float>(frequency.QuadPart);
            prevTime = currentTime;

            if (deltaTime > 0.1f) deltaTime = 0.16f;

            t += deltaTime * 0.5f;

            // --------------------------------------------------------
            // ① ゲームの更新処理
            // --------------------------------------------------------
            if (gameState == 0)
            {
                if (GetAsyncKeyState(VK_RETURN) & 0x8000) {
                    gameState = 1;
                    gameTimer = 10.0f;
                }
            }
            else if (gameState == 1)
            {
                player->Update(deltaTime);
                enemy->Update(deltaTime, player->GetPosX(), player->GetPosY()); // 逃げるAI

                gameTimer -= deltaTime;
                if (gameTimer <= 0.0f) {
                    gameState = 3;
                }

                float pX = player->GetPosX(); float pY = player->GetPosY();
                float eX = enemy->GetPosX();  float eY = enemy->GetPosY();
                float size = 0.6f;

                if (pX + size > eX && pX < eX + size &&
                    pY + size > eY && pY < eY + size)
                {
                    enemy->Destroy();
                    gameState = 2;
                }
            }
            else if (gameState == 2 || gameState == 3)
            {
                if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                    player = std::make_unique<Player>();
                    enemy = std::make_unique<Enemy>();
                    gameState = 0;
                }
            }

            // --------------------------------------------------------
            // ② 画面クリア
            // --------------------------------------------------------
            float clearColor[4];
            if (gameState == 0) { clearColor[0] = 0.05f; clearColor[1] = 0.1f;  clearColor[2] = 0.2f;  clearColor[3] = 1.0f; }
            else if (gameState == 1) { clearColor[0] = 0.1f;  clearColor[1] = 0.4f;  clearColor[2] = 0.7f;  clearColor[3] = 1.0f; }
            else if (gameState == 2) { clearColor[0] = 0.9f;  clearColor[1] = 0.6f;  clearColor[2] = 0.1f;  clearColor[3] = 1.0f; }
            else { clearColor[0] = 0.4f;  clearColor[1] = 0.05f; clearColor[2] = 0.05f; clearColor[3] = 1.0f; }

            graphics.BeginRender(clearColor);

            // --------------------------------------------------------
            // ③ 【3Dオブジェクトの描画】
            // --------------------------------------------------------
            if (gameState == 1 || gameState == 2)
            {
                ConstantBufferData cb;
                XMVECTOR Eye = XMVectorSet(0.0f, 2.0f, -4.0f, 0.0f);
                XMVECTOR At = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
                XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
                cb.mView = XMMatrixLookAtLH(Eye, At, Up);
                cb.mProjection = XMMatrixPerspectiveFovLH(XM_PIDIV4, SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
                cb.mView = XMMatrixTranspose(cb.mView);
                cb.mProjection = XMMatrixTranspose(cb.mProjection);

                ID3D11DeviceContext* ctx = graphics.GetContext();
                ctx->IASetInputLayout(g_pVertexLayout);
                ctx->VSSetShader(g_pVertexShader, nullptr, 0);

                ID3D11Buffer* cbPtr = graphics.GetConstantBuffer();
                ctx->VSSetConstantBuffers(0, 1, &cbPtr);

                // テクスチャとサンプラーをセット
                ctx->PSSetShaderResources(0, 1, &g_pTextureRV);
                ctx->PSSetSamplers(0, 1, &g_pSamplerState);

                UINT stride = sizeof(SimpleVertex); UINT offset = 0;
                ctx->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);
                ctx->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
                ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

                // --- プレイヤーの描画（通常シェーダー） ---
                ctx->PSSetShader(g_pPixelShader, nullptr, 0);
                // プレイヤー用テクスチャをバインド
                ctx->PSSetShaderResources(0, 1, &g_pTextureRV);
                cb.mWorld = XMMatrixTranspose(player->GetWorldMatrix(t));
                ctx->UpdateSubresource(graphics.GetConstantBuffer(), 0, nullptr, &cb, 0, 0);
                ctx->DrawIndexed(36, 0, 0);

                // --- 敵の描画（敵用シェーダー） ---
                if (enemy->IsAlive()) {
                    ctx->PSSetShader(g_pPixelShaderEnemy, nullptr, 0);
                    // 敵用テクスチャに切り替え
                    ctx->PSSetShaderResources(0, 1, &g_pTextureEnemyRV);
                    cb.mWorld = XMMatrixTranspose(enemy->GetWorldMatrix(t));
                    ctx->UpdateSubresource(graphics.GetConstantBuffer(), 0, nullptr, &cb, 0, 0);
                    ctx->DrawIndexed(36, 0, 0);
                }
            }

            // --------------------------------------------------------
            // ④ 画面の最前面への文字の描画
            // --------------------------------------------------------
            if (gameState == 0)
            {
                RenderText(hWnd, nullptr, L"CUBIC ATTACK 3D", 180, 50, RGB(255, 255, 255));
                RenderText(hWnd, nullptr, L"Press [ ENTER ] to Start", 320, 24, RGB(200, 200, 200));
            }
            else if (gameState == 1)
            {
                wchar_t timerStr[100];
                wsprintfW(timerStr, L"TIME: %d", (int)gameTimer);
                RenderText(hWnd, nullptr, timerStr, 30, 28, RGB(255, 255, 255));
            }
            else if (gameState == 2)
            {
                RenderText(hWnd, nullptr, L"GAME CLEAR !!", 200, 55, RGB(255, 255, 255));
                RenderText(hWnd, nullptr, L"Press [ SPACE ] to Return Title", 350, 22, RGB(240, 240, 240));
            }
            else if (gameState == 3)
            {
                RenderText(hWnd, nullptr, L"GAME OVER", 200, 60, RGB(255, 50, 50));
                RenderText(hWnd, nullptr, L"TIME UP...", 280, 24, RGB(200, 200, 200));
                RenderText(hWnd, nullptr, L"Press [ SPACE ] to Retry", 380, 22, RGB(240, 240, 240));
            }
            graphics.EndRender();
        }
    }

    // 終了時解放
    if (g_pSamplerState)     g_pSamplerState->Release();
    if (g_pTextureEnemyRV)    g_pTextureEnemyRV->Release();
    if (g_pTextureRV)        g_pTextureRV->Release();
    if (g_pIndexBuffer)      g_pIndexBuffer->Release();
    if (g_pVertexBuffer)     g_pVertexBuffer->Release();
    if (g_pVertexLayout)     g_pVertexLayout->Release();
    if (g_pVertexShader)     g_pVertexShader->Release();
    if (g_pPixelShaderEnemy) g_pPixelShaderEnemy->Release();
    if (g_pPixelShader)      g_pPixelShader->Release();

    CoUninitialize();
    return (int)msg.wParam;
}