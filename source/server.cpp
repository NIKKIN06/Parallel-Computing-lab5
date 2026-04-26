#include "../header/server.h"

using namespace std;

std::string Server::getCurrentTime()
{
	auto now = chrono::current_zone()->to_local(chrono::system_clock::now());
	std::string time = format("{:%T}", now);

	return time;
}

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
		cerr << "[Main Thread] [" << getCurrentTime() << "] WinSock initialization ERROR!\n";
		return;
	}

	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET)
	{
		cerr << "[Main Thread] [" << getCurrentTime() << "] Socket creation ERROR!\n";
		WSACleanup();
		return;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	if (::bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cerr << "[Main Thread] [" << getCurrentTime() << "] Bind ERROR: " << WSAGetLastError() << "\n";
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "[Main Thread] [" << getCurrentTime() << "] Listen ERROR: " << WSAGetLastError() << "\n";
		closesocket(serverSocket);
		WSACleanup();
		return;
	}

	cout << "[Main Thread] [" << getCurrentTime() << "] Server has been launched! Waiting for connections on PORT " << PORT << "...\n\n";
}

Server::~Server()
{
	closesocket(serverSocket);
	WSACleanup();
}

void Server::handleClient(SOCKET clientSocket)
{
	char buffer[4096] = { 0 };

	int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

	if (bytesReceived > 0)
	{
		std::string request(buffer);
		std::istringstream requestStream(request);
		std::string method, path, protocol;

		requestStream >> method >> path >> protocol;

		cout << "[Main Thread] Request: " << method << " " << path << "\n";

		if (path == "/")
		{
			path = "/index.html";
		}

		std::string filePath = "./public" + path;

		ifstream file(filePath, ios::in | ios::binary);
		std::string httpResponse;

		if (file.is_open())
		{
			std::ostringstream fileContentStream;
			fileContentStream << file.rdbuf();
			std::string fileContent = fileContentStream.str();
			file.close();

			httpResponse = formHttpResponse("200 OK", fileContent);
		}
		else
		{
			std::string errorHtml = "<html><body><h1>404 Not Found</h1></body></html>";
			httpResponse = formHttpResponse("404 Not Found", errorHtml);

			cout << "[Main Thread] File not found. Sending 404.\n";
		}

		send(clientSocket, httpResponse.c_str(), httpResponse.size(), 0);
	}

	closesocket(clientSocket);
}

void Server::acceptConnection()
{
	while (true)
	{
		sockaddr_in clientAddress;
		int clientAddressSize = sizeof(clientAddress);

		SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);

		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "[Main Thread] [" << getCurrentTime() << "] Accept ERROR: " << WSAGetLastError() << "\n";
			continue;
		}

		//int currentClientId = clientIdCounter++;

		char clientIp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientAddress.sin_addr, clientIp, INET_ADDRSTRLEN);
		int clientPort = ntohs(clientAddress.sin_port);

		cout << "[Main Thread] Accepted connection from IP: " << clientIp << ", Port: " << clientPort /* << " -> Assigned Client ID: #" << currentClientId */ << "\n";

		cout << "[Main Thread] Sending to ThreadPool.\n";

		pool.add_task([this, clientSocket]()
			{
				this->handleClient(clientSocket);
			});
	}
}