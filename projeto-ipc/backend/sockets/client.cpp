#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#pragma comment(lib, "Ws2_32.lib")

std::string getTimestamp() {
    // Get the current calendar time as a time_t object
    time_t currentTime = time(NULL); 

    // Convert the time_t object to a human-readable string (local time)
    char* dateTimeString = ctime(&currentTime);

    return std::string(dateTimeString);
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

    // WSAStartup inicializa a DLL do Winsock no Windows (obrigatório).
    // MAKEWORD(2,2) pede a versão 2.2 da API. Se retornar != 0, houve falha.
    // Em caso de erro, imprimimos mensagem, registramos no logger e encerramos.
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        std::cerr << "[ERRO] WSAStartup: " << WSAGetLastError() << "\n";
        // logger() e getTimestamp() são utilitários do seu projeto:
        // - logger(nível, função, timestamp, mensagem, bytes, peer)
        // - getTimestamp() retorna o horário atual como string.
        // Aqui registramos o erro sem bytes/peer (0 e "N/A").
        logger("ERROR", "WSAStartup", getTimestamp(), "WSAStartup failed", 0, "N/A");
        return 1;
    } else {
        // Log de sucesso para auditoria/diagnóstico.
        logger("INFO", "WSAStartup", getTimestamp(), "WSAStartup successful", 0, "N/A");
    }

    // Criação do socket do cliente:
    // - AF_INET6: IPv6
    // - SOCK_STREAM: TCP (fluxo orientado à conexão)
    // - protocolo = 0: usa o padrão para STREAM em IPv6 (TCP)
    // Se retornar INVALID_SOCKET, a criação falhou.
    SOCKET clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "[ERRO] socket: " << WSAGetLastError() << "\n";
        logger("ERROR", "socket", getTimestamp(), "Socket creation failed", 0, "N/A");
        WSACleanup();
        return 1;
    } else {
        logger("INFO", "socket", getTimestamp(), "Socket created successfully", 0, "N/A");
    }

    // Monta o endereço do servidor (IPv6):
    // sockaddr_in6 é a estrutura padrão para endereços IPv6.
    // - sin6_family = AF_INET6 (família IPv6)
    // - sin6_port   = htons(8080) → porta 8080 em ordem de rede (big-endian)
    // - sin6_addr   = in6addr_loopback → ::1 (localhost). Só conecta na própria máquina.
    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;
    serverAddr.sin6_port   = htons(8080);
    serverAddr.sin6_addr   = in6addr_loopback;

    // connect() inicia o 3-way handshake TCP com o servidor (::1:8080).
    // Sucesso → 0; erro → SOCKET_ERROR. Em erro, fechamos socket e limpamos Winsock.
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[ERRO] connect: " << WSAGetLastError() << "\n";
        logger("ERROR", "connect", getTimestamp(), "Connect failed", 0, "[::1]:8080");
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    } else {
        logger("INFO", "connect", getTimestamp(), "Connected to server", 0, "[::1]:8080");
    }

    // Loop principal de interação:
    // - Lê uma linha do usuário (std::getline)
    // - Envia ao servidor (send)
    // - Aguarda resposta (recv)
    // - Se a resposta for "Fechando socket...", encerra o cliente (break)
    while (true)
    {
        // Entrada do usuário: a mensagem será enviada exatamente como digitada,
        // sem '\0' implícito; o tamanho é message.size().
        std::string message;
        std::cout << "Digite a mensagem para enviar ao servidor: ";
        std::getline(std::cin, message);

        // send() envia bytes ao servidor pelo socket TCP.
        // Retorno: n° de bytes enviados (>0) ou SOCKET_ERROR em falha.
        // Obs.: Em TCP, send pode enviar menos bytes do que o solicitado em casos de buffers/pressão de rede.
        // Nas mensagens curtas deste trabalho, isso é raro; o log registra o tamanho informado.
        int sent = send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0);
        if (sent == SOCKET_ERROR) {
            std::cerr << "[ERRO] send: " << WSAGetLastError() << "\n";
            logger("ERROR", "send", getTimestamp(), "Send failed", 0, "[::1]:8080");
            break; // sai do loop e finaliza cliente
        } else {
            // Informamos quantos bytes foram "aceitos" para envio pelo stack TCP.
            logger("INFO", "send", getTimestamp(), "Message sent to server", sent, "[::1]:8080");
        }

        // Recebe resposta do servidor.
        // - buffer2 é zerado só para facilitar leitura/depuração (não é obrigatório).
        // - recv retorna:
        //   >0: bytes efetivamente recebidos
        //   0: servidor fechou a conexão de forma ordenada
        //   SOCKET_ERROR: falha
        char buffer2[5096] = {0};
        int bytesReceived = recv(clientSocket, buffer2, sizeof(buffer2), 0);
        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "[ERRO] recv: " << WSAGetLastError() << "\n";
            logger("ERROR", "recv", getTimestamp(), "Recv failed", 0, "[::1]:8080");
            break;
        }
        if (bytesReceived == 0) {
            // 0 bytes significa que o peer (servidor) fechou a conexão.
            std::cout << "[INFO] Servidor fechou a conexão.\n";
            logger("INFO", "server_closed", getTimestamp(), "Server closed connection", 0, "[::1]:8080");
            break;
        }

        // Imprime a resposta do servidor. Como zeramos o buffer, cout tende a funcionar;
        // mas o jeito mais seguro é construir a std::string usando (buffer2, bytesReceived),
        // o que fazemos logo abaixo.
        std::cout << "Mensagem recebida do servidor: " << buffer2 << std::endl;
        logger("INFO", "recv", getTimestamp(), "Message received from server",
               bytesReceived, "[::1]:8080");

        // Converte bytes → string exatamente com o tamanho recebido (não depende de '\0').
        std::string mensagemServidor(buffer2, bytesReceived);

        // Protocolo simples: se o servidor mandar "Fechando socket...", encerramos.
        if (mensagemServidor == "Fechando socket...")
        {
            logger("INFO", "server_signal", getTimestamp(),
                   "Server requested client to close", 0, "[::1]:8080");
            break;
        }
    }

    // Fecha o socket do cliente (encerra a conexão TCP) e registra resultado.
    if (closesocket(clientSocket) == SOCKET_ERROR) {
        std::cerr << "[ERRO] closesocket(client): " << WSAGetLastError() << "\n";
        logger("ERROR", "closesocket", getTimestamp(), "Client socket close failed", 0, "N/A");
    } else {
        logger("INFO", "closesocket", getTimestamp(), "Client socket closed", 0, "N/A");
    }

    // Libera a DLL do Winsock (final da vida da API para este processo).
    if (WSACleanup() == SOCKET_ERROR) {
        std::cerr << "[ERRO] WSACleanup: " << WSAGetLastError() << "\n";
        logger("ERROR", "WSACleanup", getTimestamp(), "WSACleanup failed", 0, "N/A");
    } else {
        logger("INFO", "WSACleanup", getTimestamp(), "WSACleanup successful", 0, "N/A");
    }

    return 0;
}
