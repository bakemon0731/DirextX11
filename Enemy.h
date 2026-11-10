#pragma once
#include <d3d11.h>
#include <directxmath.h>

class Enemy
{
public:
    Enemy();
    ~Enemy();

    // プレイヤーの座標を受け取るように引数を追加
    void Update(float deltatime, float playerX, float playerY);
    DirectX::XMMATRIX GetWorldMatrix(float t);

    float GetPosX() { return m_posX; }
    float GetPosY() { return m_posY; }

    bool IsAlive() { return m_isAlive; }
    void Destroy() { m_isAlive = false; }

private:
    float m_posX;
    float m_posY;
    // 動スピードや周期用
    float m_speed;
    // 軌道計算用の経過時間蓄積
    float m_timeAcc;
    bool m_isAlive;
};
