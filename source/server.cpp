#include "../header/server.h"

using namespace std;

std::string Server::formHttpResponse(std::string statusCode, std::string content)
{
	return "HTTP/1.1 " + statusCode + "\r\n"
		"Content-Length: " + to_string(content.length()) + "\r\n"
		"Connection: close\r\n"
		"\r\n" +
		content;
}

Server::Server()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cerr << "[Main Thread] WinSock initialization ERROR!\n";
		return;
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		cerr << "[Main Thread] Socket creation ERROR!\n";
		WSACleanup();
		return;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (::bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cerr << "[Main Thread] Bind ERROR: " << WSAGetLastError() << "\n";
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "[Main Thread] Listen ERROR: " << WSAGetLastError() << "\n";
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	cout << "[Main Thread] Server has been launched! Waiting for connections on PORT " << PORT << "...\n\n";
}

Server::~Server()
{
	closesocket(serverSocket);
	WSACleanup();
}

void Server::handleClient(SOCKET clientSocket)
{
	char buffer[4096];
	std::string request;
	int bytesReceived;

	while (true)
	{
		bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
		if (bytesReceived > 0)
		{
			buffer[bytesReceived] = '\0';
			request.append(buffer, bytesReceived);

			if (request.find("\r\n\r\n") != std::string::npos)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if (!request.empty())
	{
		std::istringstream requestStream(request);
		std::string method, path, protocol;
		requestStream >> method >> path >> protocol;

		if (path == "/") path = "/index.html";
		std::string filePath = "./public" + path;

		std::string httpResponse;
		std::ifstream file(filePath, std::ios::in | std::ios::binary);

		if (file.is_open())
		{
			std::ostringstream fileContentStream;
			fileContentStream << file.rdbuf();
			httpResponse = formHttpResponse("200 OK", fileContentStream.str());
			file.close();
		}
		else
		{
			std::string errorHtml = "<html><body><h1>404 Not Found</h1></body></html>";
			httpResponse = formHttpResponse("404 Not Found", errorHtml);
		}

		send(clientSocket, httpResponse.c_str(), (int)httpResponse.size(), 0);
	}

	shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
}

void Server::acceptConnection()
{
	while (true)
	{
		sockaddr_in clientAddress;
		int clientAddressSize = sizeof(clientAddress);

		SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);

		//DWORD timeout = 500;
		//setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
		//setsockopt(clientSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
		
		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "[Main Thread] Accept ERROR: " << WSAGetLastError() << "\n";
			continue;
		}

		pool.add_task([this, clientSocket]()
			{
				this->handleClient(clientSocket);
			});
	}
}