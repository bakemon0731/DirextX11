#pragma once

class NetworkManager
{
public:
    NetworkManager();
    ~NetworkManager();

    // ネットワーク機能の初期化
    bool Initialize();

    // ネットワーク機能の終了
    bool Shutdown();

    // 初期化状態の取得
    bool IsInitialized() const;

    // エラーコードの取得
    int GetLastErrorCode() const;

private:
    bool m_isInitialized;
    int m_lastErrorCode;
};