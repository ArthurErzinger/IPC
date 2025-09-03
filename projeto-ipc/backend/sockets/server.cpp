#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#pragma comment(lib, "Ws2_32.lib")

// Função para gerar timestamp em formato ISO 8601 (UTC)
std::string getTimestamp() {
    using namespace std::chrono;

    auto now = system_clock::now();
    std::time_t now_time = system_clock::to_time_t(now);

    std::tm utc_tm;
    gmtime_s(&utc_tm, &now_time);  // versão segura no Windows

    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

void logger(const std::string& level, const std::string& event, const std::string& ts,
            const std::string& msg, int bytes, const std::string& peer) {
    std::cout << "{\n"
              << "  \"module\": \"sockets\",\n"
              << "  \"role\": \"server\",\n"
              << "  \"level\": \"" << level << "\",\n"
              << "  \"event\": \"" << event << "\",\n"
              << "  \"ts\": \"" << ts << "\",\n"
              << "  \"details\": {\n"
              << "    \"msg\": \"" << msg << "\",\n"
              << "    \"bytes\": " << bytes << ",\n"
              << "    \"peer\": \"" << peer << "\"\n"
              << "  }\n"
              << "}" << std::endl;
}

int main() {
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[ERRO] WSAStartup: " << WSAGetLastError() << "\n";
        return 1;

    } else {
        logger("INFO", "WSAStartup", getTimestamp(), "WSAStartup successful", 0, "N/A");
    }

    SOCKET serverSocket = socket(AF_INET6, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "[ERRO] socket: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;

    } else {
        logger("INFO", "socket", getTimestamp(), "Socket created successfully", 0, "N/A");
    }

    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;          // IPv6
    serverAddr.sin6_port   = htons(8080);       // porta 8080
    serverAddr.sin6_addr   = in6addr_loopback;  // ::1

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[ERRO] bind: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;

    } else {
        logger("INFO", "bind", getTimestamp(), "Bind successful on [::1]:8080", 0, "::1:8080");
    }

    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "[ERRO] listen: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;

    } else {
        logger("INFO", "listen", getTimestamp(), "Listening for connections", 0, "::1:8080");
    }

    std::cout << "Servidor aguardando conexões em [::1]:8080...\n";

    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);

    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "[ERRO] accept: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;

    } else {
        logger("INFO", "accept", getTimestamp(), "Client connected", 0, "::1:random_port");
    }

    while (true) {
        char buffer2[5096] = {0};

        int bytesReceived = recv(clientSocket, buffer2, sizeof(buffer2), 0);

        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "[ERRO] recv: " << WSAGetLastError() << "\n";
            break;

        } else {
            logger("INFO", "recv", getTimestamp(), "Message received from client", bytesReceived, "::1:random_port");
        }

        if (bytesReceived == 0) {
            std::cout << "[INFO] Cliente fechou a conexão.\n";
            break;
        }

        std::string mensagemCliente(buffer2, bytesReceived);
        std::cout << "Mensagem recebida do cliente: " << mensagemCliente << std::endl;

        int sent = 0;

        if (mensagemCliente == "oi") {
            sent = send(clientSocket, "hello", 5, 0);

        } else if (mensagemCliente == "sair") {
            std::cout << "Fechando socket..." << std::endl;
            sent = send(clientSocket, "Fechando socket...", 18, 0);

            if (sent == SOCKET_ERROR) {
                std::cerr << "[ERRO] send('Fechando socket...'): " << WSAGetLastError() << "\n";
            } else {
                logger("INFO", "send", getTimestamp(), "Sent closing message to client", sent, "::1:random_port");
            }

            break;

        } else if (mensagemCliente == "ping") {
            sent = send(clientSocket, "pong", 4, 0);

        } else {
            sent = send(clientSocket, "Comando Desconhecido", 20, 0);
        }

        if (sent == SOCKET_ERROR) {
            std::cerr << "[ERRO] send: " << WSAGetLastError() << "\n";
            break;

        } else {
            logger("INFO", "send", getTimestamp(), "Message sent to client" , sent, "::1:random_port");
        }

    }

    if (closesocket(clientSocket) == SOCKET_ERROR) {
            std::cerr << "[ERRO] closesocket(client): " << WSAGetLastError() << "\n";
        } else {
            logger("INFO", "closesocket", getTimestamp(), "Client socket closed", 0, "::1:random_port");
        }

        if (closesocket(serverSocket) == SOCKET_ERROR) {
            std::cerr << "[ERRO] closesocket(server): " << WSAGetLastError() << "\n";
        } else {
            logger("INFO", "closesocket", getTimestamp(), "Server socket closed", 0, "::1:8080");
        }

        if (WSACleanup() == SOCKET_ERROR) {
            std::cerr << "[ERRO] WSACleanup: " << WSAGetLastError() << "\n";
        } else {
            logger("INFO", "WSACleanup", getTimestamp(), "WSACleanup successful", 0, "N/A");
        }
}
