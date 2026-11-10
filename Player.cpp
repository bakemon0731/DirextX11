#include "Player.h"
#include <windows.h> 

using namespace DirectX;

Player::Player()
{
    m_posX = 0.0f;
    m_posY = 0.0f;
    m_speed = 1.5f; // ★1秒間に1.5単位動くという絶対速度に設定
}

Player::~Player()
{
}

// ★デルタタイムを計算式に乗算します
void Player::Update(float deltaTime)
{
    // 移動量 = スピード * 経過時間（これでどのPCでも同じ速度になります！）
    float moveAmount = m_speed * deltaTime;

    if (GetAsyncKeyState(VK_LEFT) & 0x8000) { m_posX -= moveAmount; }
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) { m_posX += moveAmount; }
    if (GetAsyncKeyState(VK_UP) & 0x8000) { m_posY += moveAmount; }
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) { m_posY -= moveAmount; }
}

XMMATRIX Player::GetWorldMatrix(float t)
{
    XMMATRIX mRotate = XMMatrixRotationRollPitchYaw(t, t, 0.0f);
    XMMATRIX mTranslate = XMMatrixTranslation(m_posX, m_posY, 0.0f);
    return mRotate * mTranslate;
}
