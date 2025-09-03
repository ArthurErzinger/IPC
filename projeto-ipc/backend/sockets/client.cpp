#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
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

// Logger JSON
void logger(const std::string& level, const std::string& event, const std::string& ts,
            const std::string& msg, int bytes, const std::string& peer) {
    std::cout << "{\n"
              << "  \"module\": \"sockets\",\n"
              << "  \"role\": \"client\",\n"
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
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "[ERRO] WSAStartup: " << WSAGetLastError() << "\n";
        // log de erro (sem bytes/peer relevantes)
        logger("ERROR", "WSAStartup", getTimestamp(), "WSAStartup failed", 0, "N/A");
        return 1;
    } else {
        logger("INFO", "WSAStartup", getTimestamp(), "WSAStartup successful", 0, "N/A");
    }

    SOCKET clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "[ERRO] socket: " << WSAGetLastError() << "\n";
        logger("ERROR", "socket", getTimestamp(), "Socket creation failed", 0, "N/A");
        WSACleanup();
        return 1;
    } else {
        logger("INFO", "socket", getTimestamp(), "Socket created successfully", 0, "N/A");
    }

    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port   = htons(8080);
    serverAddr.sin6_addr   = in6addr_loopback;

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[ERRO] connect: " << WSAGetLastError() << "\n";
        logger("ERROR", "connect", getTimestamp(), "Connect failed", 0, "[::1]:8080");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    } else {
        logger("INFO", "connect", getTimestamp(), "Connected to server", 0, "[::1]:8080");
    }

    while (true)
    {
        std::string message;
        std::cout << "Digite a mensagem para enviar ao servidor: ";
        std::getline(std::cin, message);

        int sent = send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "[ERRO] send: " << WSAGetLastError() << "\n";
            logger("ERROR", "send", getTimestamp(), "Send failed", 0, "[::1]:8080");
            break;
        } else {
            logger("INFO", "send", getTimestamp(), "Message sent to server", sent, "[::1]:8080");
        }

        // Receber resposta do servidor
        char buffer2[5096] = {0};
        int bytesReceived = recv(clientSocket, buffer2, sizeof(buffer2), 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "[ERRO] recv: " << WSAGetLastError() << "\n";
            logger("ERROR", "recv", getTimestamp(), "Recv failed", 0, "[::1]:8080");
            break;
        }
        if (bytesReceived == 0) {
            std::cout << "[INFO] Servidor fechou a conexão.\n";
            logger("INFO", "server_closed", getTimestamp(), "Server closed connection", 0, "[::1]:8080");
            break;
        }

        std::cout << "Mensagem recebida do servidor: " << buffer2 << std::endl;
        logger("INFO", "recv", getTimestamp(), "Message received from server",
               bytesReceived, "[::1]:8080");

        std::string mensagemServidor(buffer2, bytesReceived);
        if (mensagemServidor == "Fechando socket...")
        {
            logger("INFO", "server_signal", getTimestamp(),
                   "Server requested client to close", 0, "[::1]:8080");
            break;
        }
    }

    if (closesocket(clientSocket) == SOCKET_ERROR) {
        std::cerr << "[ERRO] closesocket(client): " << WSAGetLastError() << "\n";
        logger("ERROR", "closesocket", getTimestamp(), "Client socket close failed", 0, "N/A");
    } else {
        logger("INFO", "closesocket", getTimestamp(), "Client socket closed", 0, "N/A");
    }

    if (WSACleanup() == SOCKET_ERROR) {
        std::cerr << "[ERRO] WSACleanup: " << WSAGetLastError() << "\n";
        logger("ERROR", "WSACleanup", getTimestamp(), "WSACleanup failed", 0, "N/A");
    } else {
        logger("INFO", "WSACleanup", getTimestamp(), "WSACleanup successful", 0, "N/A");
    }

    return 0;
}
