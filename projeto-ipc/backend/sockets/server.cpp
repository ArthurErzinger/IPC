#include <winsock2.h>     // API de sockets do Windows (Winsock)
#include <ws2tcpip.h>     // Extensões (IPv6, funções utilitárias)
#include <iostream>       // I/O em C++
#include <chrono>         // (não usado diretamente aqui) utilidades de tempo modernas
#include <iomanip>        // (não usado diretamente aqui) formatação de I/O
#include <sstream>        // (não usado diretamente aqui) string streams
#include <ctime>          // time_t, time(), ctime()
#pragma comment(lib, "Ws2_32.lib") // Linka a biblioteca Ws2_32.lib (necessária no Windows)

// -----------------------------------------------------------------------------
// Utilitário: devolve um timestamp simples (string) usando ctime()
// Observação: ctime() retorna uma string com newline no final; aqui apenas
// embrulhamos em std::string sem remover o '\n'.
// -----------------------------------------------------------------------------
std::string getTimestamp() {
    // Pega o tempo atual (epoch time)
    time_t currentTime = time(NULL);

    // Converte para string legível (horário local); retorna ponteiro char*
    // Ex.: "Wed Sep  4 12:34:56 2025\n"
    char* dateTimeString = ctime(&currentTime);

    // Constrói std::string a partir do char* retornado
    return std::string(dateTimeString);
}

// -----------------------------------------------------------------------------
// logger(): função de logging no formato JSON-like para stdout.
// Parâmetros:
//  - level: nível do log (INFO/ERROR/etc.)
//  - event: nome curto do evento (ex.: "accept", "recv", "send")
//  - ts: timestamp (string) — aqui usamos getTimestamp()
//  - msg: mensagem descritiva
//  - bytes: quantidade de bytes (quando fizer sentido; 0 caso não se aplique)
//  - peer: identificação do peer (ex.: "::1:8080" ou placeholder se desconhecido)
// -----------------------------------------------------------------------------
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

    // -----------------------------------------------------------------------------
    // WSAStartup: inicializa a DLL do Winsock (obrigatório no Windows).
    // Sucesso → 0; erro → != 0. Em caso de erro, WSAGetLastError traz o código.
    // -----------------------------------------------------------------------------
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "[ERRO] WSAStartup: " << WSAGetLastError() << "\n";
        return 1;

    } else {
        // Log de sucesso (útil para auditoria/diagnóstico)
        logger("INFO", "WSAStartup", getTimestamp(), "WSAStartup successful", 0, "N/A");
    }

    // -----------------------------------------------------------------------------
    // Criação do socket do servidor:
    // - AF_INET6: família IPv6
    // - SOCK_STREAM: TCP (fluxo confiável, ordenado; sem fronteiras de "mensagem")
    // - protocolo = 0: escolhe o padrão para STREAM/IPv6 → TCP
    // Retorno: SOCKET válido ou INVALID_SOCKET em erro.
    // -----------------------------------------------------------------------------
    SOCKET serverSocket = socket(AF_INET6, SOCK_STREAM, 0);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "[ERRO] socket: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;

    } else {
        logger("INFO", "socket", getTimestamp(), "Socket created successfully", 0, "N/A");
    }

    // -----------------------------------------------------------------------------
    // Montagem do endereço local (onde o servidor escuta):
    // sockaddr_in6 descreve um endpoint IPv6:
    //  - sin6_family = AF_INET6 (IPv6)
    //  - sin6_port   = htons(8080) → porta 8080 em ordem de rede (big-endian)
    //  - sin6_addr   = in6addr_loopback (= ::1), logo só aceita conexões locais.
    // Obs.: "{}" zera a struct (boa prática para evitar lixo de memória).
    // -----------------------------------------------------------------------------
    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6;          // IPv6
    serverAddr.sin6_port   = htons(8080);       // porta 8080
    serverAddr.sin6_addr   = in6addr_loopback;  // ::1

    // -----------------------------------------------------------------------------
    // bind: associa o socket ao endereço/porta configurados acima.
    // Sucesso → 0; erro → SOCKET_ERROR.
    // Sem bind, o SO não saberia em qual IP/porta seu servidor atende.
    // -----------------------------------------------------------------------------
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[ERRO] bind: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;

    } else {
        logger("INFO", "bind", getTimestamp(), "Bind successful on [::1]:8080", 0, "::1:8080");
    }

    // -----------------------------------------------------------------------------
    // listen: coloca o socket em modo passivo (servidor), criando a fila de espera.
    // backlog = 5 → até 5 conexões podem ficar pendentes aguardando accept().
    // Sucesso → 0; erro → SOCKET_ERROR.
    // -----------------------------------------------------------------------------
    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "[ERRO] listen: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;

    } else {
        logger("INFO", "listen", getTimestamp(), "Listening for connections", 0, "::1:8080");
    }

    std::cout << "Servidor aguardando conexões em [::1]:8080...\n";

    // -----------------------------------------------------------------------------
    // accept: bloqueia até chegar uma conexão pendente, então retorna um NOVO socket
    // exclusivo para falar com aquele cliente. O serverSocket continua aberto
    // para aceitar outros (não fazemos isso aqui, mas é o comportamento).
    // Sucesso → SOCKET válido; erro → INVALID_SOCKET.
    // Como passamos nullptr para addr e addrlen, não coletamos IP/porta do cliente.
    // -----------------------------------------------------------------------------
    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);

    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "[ERRO] accept: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;

    } else {
        // Como não coletamos o endpoint real do cliente (nullptr), usamos um placeholder.
        logger("INFO", "accept", getTimestamp(), "Client connected", 0, "::1:random_port");
    }

    // -----------------------------------------------------------------------------
    // Loop principal do servidor: recebe comandos do cliente e responde.
    // Observações de TCP:
    //  - recv() lê "até N bytes" do fluxo (pode vir menos; stream não tem fronteiras).
    //  - bytesReceived == 0 → cliente fechou a conexão ordenadamente.
    //  - bytesReceived == SOCKET_ERROR → houve falha; use WSAGetLastError().
    // -----------------------------------------------------------------------------
    while (true) {
        char buffer2[5096] = {0}; // Zeramos para facilitar prints; não é obrigatório.

        // recv: lê até sizeof(buffer2) bytes.
        int bytesReceived = recv(clientSocket, buffer2, sizeof(buffer2), 0);

        if (bytesReceived == SOCKET_ERROR) {
            std::cerr << "[ERRO] recv: " << WSAGetLastError() << "\n";
            break; // encerra a sessão com este cliente

        } else {
            // Loga o total de bytes lidos nesta chamada (pode variar)
            logger("INFO", "recv", getTimestamp(), "Message received from client", bytesReceived, "::1:random_port");
        }

        if (bytesReceived == 0) {
            // 0 = peer fechou a conexão (fim ordenado)
            std::cout << "[INFO] Cliente fechou a conexão.\n";
            break;
        }

        // Constrói uma std::string exata a partir dos bytes recebidos.
        // Importante: não dependemos de '\0' — usamos (buffer2, bytesReceived).
        std::string mensagemCliente(buffer2, bytesReceived);
        std::cout << "Mensagem recebida do cliente: " << mensagemCliente << std::endl;

        int sent = 0;

        // -------------------------------------------------------------------------
        // Protocolo simples por comandos de texto:
        //  - "oi"   → responde "hello"
        //  - "ping" → responde "pong"
        //  - "sair" → responde "Fechando socket..." e encerra a sessão
        //  - default → "Comando Desconhecido"
        // Obs.: TCP é stream — aqui assumimos que cada recv traz "uma" mensagem,
        // o que funciona em prática para comandos curtos neste trabalho.
        // -------------------------------------------------------------------------
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

            // Após avisar o cliente, saímos do loop (encerrando a sessão)
            break;

        } else if (mensagemCliente == "ping") {
            sent = send(clientSocket, "pong", 4, 0);

        } else {
            // Comando não reconhecido
            sent = send(clientSocket, "Comando Desconhecido", 20, 0);
        }

        // send: retorna bytes enviados (>0) ou SOCKET_ERROR em falha.
        // Em TCP, "envio parcial" pode ocorrer (retorno < tamanho do pedido), mas
        // como as mensagens aqui são curtas, normalmente isso não acontece.
        if (sent == SOCKET_ERROR) {
            std::cerr << "[ERRO] send: " << WSAGetLastError() << "\n";
            break;

        } else {
            logger("INFO", "send", getTimestamp(), "Message sent to client" , sent, "::1:random_port");
        }

    } // fim do while de atendimento ao cliente

    // -----------------------------------------------------------------------------
    // Encerramento e limpeza de recursos (ordem: client → server → Winsock).
    // closesocket retorna 0 em sucesso; SOCKET_ERROR em falha.
    // -----------------------------------------------------------------------------
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
