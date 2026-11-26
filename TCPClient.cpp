#include "TCPClient.h"

#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

TCPClient::TCPClient()
    : m_socket(INVALID_SOCKET),
    m_lastErrorCode(0)
{
}

TCPClient::~TCPClient()
{
    Disconnect();
}

bool TCPClient::Connect(
    const char* serverIp,
    unsigned short port
)
{
    // 通信用ソケットを作成
    m_socket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (m_socket == INVALID_SOCKET)
    {
        m_lastErrorCode = WSAGetLastError();
        return false;
    }

    // 接続先の情報を設定
    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    // IPアドレスの文字列を変換
    const int addressResult = InetPtonA(
        AF_INET,
        serverIp,
        &serverAddress.sin_addr
    );

    if (addressResult != 1)
    {
        m_lastErrorCode =
            (addressResult == 0) ? WSAEINVAL : WSAGetLastError();

        Disconnect();
        return false;
    }

    // サーバーに接続
    if (connect(
        m_socket,
        reinterpret_cast<sockaddr*>(&serverAddress),
        sizeof(serverAddress)
    ) == SOCKET_ERROR)
    {
        m_lastErrorCode = WSAGetLastError();
        Disconnect();
        return false;
    }

    m_lastErrorCode = 0;
    return true;
}

void TCPClient::Disconnect()
{
    if (m_socket != INVALID_SOCKET)
    {
        shutdown(m_socket, SD_BOTH);
        closesocket(m_socket);

        m_socket = INVALID_SOCKET;
    }
}

int TCPClient::GetLastErrorCode() const
{
    return m_lastErrorCode;
}