#include "Enemy.h"
#include <cmath>
#include <algorithm>

using namespace DirectX;

Enemy::Enemy()
{
    m_posX = 0.0f;
    m_posY = 0.0f;
    // 逃げるスピード
    m_speed = 1.5f;
    m_isAlive = true; 
}

Enemy::~Enemy()
{
}

void Enemy::Update(float deltaTime, float playerX, float playerY)
{
    // プレイヤーと敵のX, Yの差分を計算
    float dx = m_posX - playerX;
    float dy = m_posY - playerY;

    // 2点間の距離を計算 (三平方の定理: √(dx^2 + dy^2))
    float distance = sqrtf(dx * dx + dy * dy);

    // プレイヤーとの距離が 1.0 未満のときだけ逃げる
    float escapeRadius = 1.2f;

    if (distance < escapeRadius&& distance > 0.0001f)
    {
        // 距離で割ることで「方向（長さ1の単位ベクトル）」に変換する（正規化）
        float dirX = dx / distance;
        float dirY = dy / distance;

        // 逃げる方向に移動量を加算
        m_posX += dirX * m_speed * deltaTime;
        m_posY += dirY * m_speed * deltaTime;
    }

    // 画面外に行かないように座標を制限（クランプ）する
    float minX = -2.5f;
    float maxX = 2.5f;
    float minY = -1.8f;
    float maxY = 1.8f;

    m_posX = std::clamp(m_posX, minX, maxX);
    m_posY = std::clamp(m_posY, minY, maxY);

}


XMMATRIX Enemy::GetWorldMatrix(float t)
{
    XMMATRIX mRotate = XMMatrixRotationRollPitchYaw(t, 0.0f, t);
    XMMATRIX mTranslate = XMMatrixTranslation(m_posX, m_posY, 0.0f);
    return mRotate * mTranslate;
}
