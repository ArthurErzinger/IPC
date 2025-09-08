#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>

// Função para gerar timestamp em formato ISO 8601
std::string getTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t now_time = system_clock::to_time_t(now);
    std::tm* utc_tm = std::gmtime(&now_time);
    std::ostringstream oss;
    oss << std::put_time(utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// Função para escapar aspas e caracteres especiais em JSON
std::string escape_json(const std::string &s) {
    std::string result;
    for (char c : s) {
        if (c == '\"') result += "\\\"";
        else result += c;
    }
    return result;
}

// Logger em JSON para pipes
void logger(const std::string& role, const std::string& level, 
            const std::string& event, const std::string& msg, int bytes = 0) {
    std::ostringstream oss;
    oss << "{\n"
        << "  \"module\": \"ipc\",\n"
        << "  \"role\": \"" << role << "\",\n"
        << "  \"level\": \"" << level << "\",\n"
        << "  \"event\": \"" << event << "\",\n"
        << "  \"ts\": \"" << getTimestamp() << "\",\n"
        << "  \"details\": {\n"
        << "    \"msg\": \"" << escape_json(msg) << "\",\n"
        << "    \"bytes\": " << bytes << "\n"
        << "  }\n"
        << "}";
    std::cout << oss.str() << std::endl;
}

// Função de tratamento de erro simples
void ErrorExit(const std::string& msg) {
    logger("pai", "error", "Erro crítico", msg);
    exit(1);
}

int main(int argc, char* argv[]) {

    // Processo filho
    if (argc > 1 && std::string(argv[1]) == "child") {
        HANDLE hRead = (HANDLE)std::stoull(argv[2]);
        HANDLE hWrite = (HANDLE)std::stoull(argv[3]);
        DWORD bytesRead;
        char buffer[256];

        while (true) {
            if (!ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0) break;
            buffer[bytesRead] = '\0';
            logger("filho", "info", "Mensagem recebida", buffer, bytesRead);

            std::string resp = "Filho recebeu: " + std::string(buffer);
            DWORD bytesWritten;
            WriteFile(hWrite, resp.c_str(), (DWORD)resp.size(), &bytesWritten, NULL);
            logger("filho", "info", "Mensagem enviada", resp, bytesWritten);

            if (std::string(buffer) == "sair") break;
        }

        CloseHandle(hRead);
        CloseHandle(hWrite);
        return 0;
    }
    
    // Processo pai
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE parentRead, parentWrite, childRead, childWrite;

    if (!CreatePipe(&parentRead, &childWrite, &sa, 0)) {
        ErrorExit("Falha ao criar pipe pai->filho");
    }
    if (!CreatePipe(&childRead, &parentWrite, &sa, 0)) {
        ErrorExit("Falha ao criar pipe filho->pai");
    }
    logger("pai", "info", "Pipes criados", "Pipes pai->filho e filho->pai criados com sucesso");

    char modulePath[MAX_PATH];
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);

    std::string cmdStr = "\"" + std::string(modulePath) + "\" child " +
                         std::to_string((unsigned long long)childRead) + " " +
                         std::to_string((unsigned long long)childWrite);
    char cmd[1024];
    strcpy(cmd, cmdStr.c_str());

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        ErrorExit("Falha ao criar processo filho");
    }
    logger("pai", "info", "Processo filho criado", "Child process iniciado com sucesso");

    CloseHandle(childRead);
    CloseHandle(childWrite);

    std::string msg;
    DWORD bytesWritten, bytesRead;
    char buffer[256];

    while (true) {
        std::cout << "Digite mensagem para filho (sair para terminar): ";
        std::getline(std::cin, msg);

        // Envio da mensagem
        if (!WriteFile(parentWrite, msg.c_str(), (DWORD)msg.size(), &bytesWritten, NULL)) {
            logger("pai", "error", "Erro ao enviar mensagem", msg);
        } else {
            logger("pai", "info", "Mensagem enviada", msg, bytesWritten);
        }

        if (!ReadFile(parentRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) || bytesRead == 0) break;
        buffer[bytesRead] = '\0';
        logger("pai", "info", "Mensagem recebida", buffer, bytesRead);

        if (msg == "sair") break;
    }

    CloseHandle(parentRead);
    CloseHandle(parentWrite);

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    logger("pai", "info", "Finalização", "Programa encerrado com sucesso");

    return 0;
}
