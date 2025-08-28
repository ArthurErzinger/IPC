#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSocket = socket(AF_INET6, SOCK_STREAM, 0);

    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(8080);
    serverAddr.sin6_addr = in6addr_loopback;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);

    std::cout << "Servidor aguardando conexões em [::1]:8080...\n";

    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);

    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);

    std::cout << "Mensagem recebida do cliente: " << buffer << std::endl;

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
}
