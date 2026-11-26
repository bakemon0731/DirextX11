#include "TCPServer.h"

#pragma comment(lib, "Ws2_32.lib")

TCPServer::TCPServer()
    : m_listenSocket(INVALID_SOCKET),
    m_clientSocket(INVALID_SOCKET),
    m_lastErrorCode(0)
{
}

TCPServer::~TCPServer()
{
    Shutdown();
}

bool TCPServer::Start(unsigned short port)
{
    // ソケットを作成する
    m_listenSocket = socket(
        AF_INET,
        SOCK_STREAM,
        IPPROTO_TCP
    );

    if (m_listenSocket == INVALID_SOCKET)
    {
        m_lastErrorCode = WSAGetLastError();
        return false;
    }

    // サーバーのアドレス情報を設定する
    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(port);

    // IPアドレスとポートを割り当てる
    if (bind(
        m_listenSocket,
        reinterpret_cast<sockaddr*>(&serverAddress),
        sizeof(serverAddress)
    ) == SOCKET_ERROR)
    {
        m_lastErrorCode = WSAGetLastError();
        Shutdown();
        return false;
    }

    // 接続要求を待ち受ける
    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        m_lastErrorCode = WSAGetLastError();
        Shutdown();
        return false;
    }

    m_lastErrorCode = 0;
    return true;
}

bool TCPServer::AcceptClient()
{
    // クライアントの接続を待つ
    m_clientSocket = accept(
        m_listenSocket,
        nullptr,
        nullptr
    );

    if (m_clientSocket == INVALID_SOCKET)
    {
        m_lastErrorCode = WSAGetLastError();
        return false;
    }

    return true;
}

void TCPServer::Shutdown()
{
    if (m_clientSocket != INVALID_SOCKET)
    {
        shutdown(m_clientSocket, SD_BOTH);
        closesocket(m_clientSocket);

        m_clientSocket = INVALID_SOCKET;
    }

    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);

        m_listenSocket = INVALID_SOCKET;
    }
}

int TCPServer::GetLastErrorCode() const
{
    return m_lastErrorCode;
}