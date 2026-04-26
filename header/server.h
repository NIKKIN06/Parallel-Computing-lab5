#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <WinSock2.h>
#include <chrono>
#include <string>
#include <format>
#include <WS2tcpip.h>
#include <sstream>
#include <fstream>

class Server
{
private:
	int PORT = 8080;
	SOCKET serverSocket;

	std::string getCurrentTime();
	std::string formHttpResponse(std::string statusCode, std::string content);
public:
	Server();
	~Server();
	void handleClient(SOCKET clientSocket);
	void acceptConnection();
};