#pragma once

#include <winsock2.h>

class TCPServer
{
public:
	TCPServer();
	~TCPServer();

	// サーバーの起動
	bool Start(unsigned short port);

	// クライアントの接続を受け入れる
	bool AcceptClient();

	// サーバーの終了
	void Shutdown();

	INT32 GetLastErrorCode() const;

private:
	//クライアントからの接続要求を待ち受けるためのソケット
	SOCKET m_listenSocket;
	//実際に接続したクライアントとの通信に使うソケット
	SOCKET m_clientSocket;

	int m_lastErrorCode;
};