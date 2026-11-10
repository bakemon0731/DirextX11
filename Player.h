#pragma once 
#include <d3d11.h>
#include <directxmath.h>

class Player
{
public:
    Player();
    ~Player();

    // デルタタイムを受け取れるように引数を追加
    void Update(float deltaTime);
    DirectX::XMMATRIX GetWorldMatrix(float t);

    float GetPosX() { return m_posX; }
    float GetPosY() { return m_posY; }

private:
    float m_posX;
    float m_posY;
    float m_speed; // 移動スピードの追加
};
