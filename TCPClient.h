#pragma once

#include <winsock2.h>

class TCPClient
{
public:
    TCPClient();
    ~TCPClient();

    // サーバーに接続
    bool Connect(const char* serverIp, unsigned short port);

    // 接続を終了
    void Disconnect();

    int GetLastErrorCode() const;

private:
    SOCKET m_socket;
    int m_lastErrorCode;
};