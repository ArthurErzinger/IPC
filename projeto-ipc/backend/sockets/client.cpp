#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET clientSocket = socket(AF_INET6, SOCK_STREAM, 0);

    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port = htons(8080);
    serverAddr.sin6_addr = in6addr_loopback;

    connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));


    while (true)
    {
        std::string message;
        std::cout << "Digite a mensagem para enviar ao servidor: ";
        std::getline(std::cin, message);

        send(clientSocket, message.c_str(), message.size(), 0);


        // Receber resposta do servidor
        char buffer2[5096] = {0};
        int bytesReceived = recv(clientSocket, buffer2, sizeof(buffer2), 0);
        std::cout << "Mensagem recebida do servidor: " << buffer2 << std::endl;

        std::string mensagemServidor(buffer2, bytesReceived);

        if (mensagemServidor == "Fechando socket...")
        {
            break;
        }
    }
    

    closesocket(clientSocket);
    WSACleanup();
}
