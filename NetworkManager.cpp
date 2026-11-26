#include "NetworkManager.h"

#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")

NetworkManager::NetworkManager()
    : m_isInitialized(false),
    m_lastErrorCode(0)
{
}

NetworkManager::~NetworkManager()
{
    Shutdown();
}

bool NetworkManager::Initialize()
{
    // すでに初期化されている場合は何もしない
    if (m_isInitialized)
    {
        return true;
    }

    WSADATA wsaData{};

    // Winsock 2.2を初期化
    const int result = WSAStartup(
        MAKEWORD(2, 2),
        &wsaData
    );

    // 初期化失敗
    if (result != 0)
    {
        m_lastErrorCode = result;
        return false;
    }

    m_isInitialized = true;
    m_lastErrorCode = 0;

    return true;
}

bool NetworkManager::Shutdown()
{
    // 初期化されていなければ終了処理は不要
    if (!m_isInitialized)
    {
        return true;
    }

    // Winsockを終了
    const int result = WSACleanup();

    if (result == SOCKET_ERROR)
    {
        m_lastErrorCode = WSAGetLastError();
        return false;
    }

    m_isInitialized = false;
    m_lastErrorCode = 0;

    return true;
}

bool NetworkManager::IsInitialized() const
{
    return m_isInitialized;
}

int NetworkManager::GetLastErrorCode() const
{
    return m_lastErrorCode;
}