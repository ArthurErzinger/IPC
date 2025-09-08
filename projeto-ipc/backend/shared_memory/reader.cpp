#include <windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

// Definições sobre a memória compartilhada e o mutex
const wchar_t* NOME_MEMORIA = L"MinhaMemoria";
const wchar_t* NOME_MUTEX   = L"MeuMutex";
const int TAM_MEMORIA = 256;

// Estrutura de dados que serão armazenados na memória compartilhada
struct SharedData {
    wchar_t mensagem[TAM_MEMORIA]; // buffer da mensagem escrita pelo writer
    bool encerrar_flag;               // flag para sinalizar encerramento (definida pelo writer)
};

// Função para gerar timestamp em formato ISO 8601
std::string getTimestamp() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t now_time = system_clock::to_time_t(now);
    std::tm* utc_tm = std::gmtime(&now_time);  // padrão C
    std::ostringstream oss;
    oss << std::put_time(utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// Logger em JSON
void logger(const std::wstring& level, const std::wstring& event,
            const std::wstring& msg, int bytes, const std::wstring& peer) {
    std::wcout << L"{\n"
               << L"  \"module\": \"ipc\",\n"
               << L"  \"role\": \"reader\",\n"
               << L"  \"level\": \"" << level << L"\",\n"
               << L"  \"event\": \"" << event << L"\",\n"
               << L"  \"ts\": \"" << getTimestamp().c_str() << L"\",\n"
               << L"  \"details\": {\n"
               << L"    \"msg\": \"" << msg << L"\",\n"
               << L"    \"bytes\": " << bytes << L",\n"
               << L"    \"peer\": \"" << peer << L"\"\n"
               << L"  }\n"
               << L"}" << std::endl;
}

int main() {
    /* Abre a memória compartilhada criada pelo writer
    - cria um handle
    - OpenFileMappingW: abre um mapeamento de memória já existente
    - FILE_MAP_ALL_ACCESS: permite tanto leitura quanto escrita
    - FALSE: não herda o handle para processos filhos
    - NOME_MEMORIA*/
    HANDLE hMapFile = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, NOME_MEMORIA);
    if (!hMapFile) { 
        logger(L"error", L"open_memory", L"Erro ao abrir memória compartilhada", GetLastError(), L"system");
        return 1; 
    }
    // Mapeia a memória compartilhada para o espaço de endereços do processo
    SharedData* sharedMem = (SharedData*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedData));
    // Verifica se o o ponteiro que a aponta para a memória é valido, se a memória foi mapeada com sucesso
    if (!sharedMem) { 
        logger(L"error", L"map_memory", L"Erro ao mapear memória", GetLastError(), L"system");
        CloseHandle(hMapFile); //Fecha o handle da memória compartilhada
        return 1; 
    }

    /* Abre o mutex para sincronizar o acesso à memória compartilhada
       - SYNCHRONIZE: permite operações de sincronização (ex: WaitForSingleObject)
       - NOME_MUTEX: nome do mutex criado pelo writer*/
    HANDLE hMutex = OpenMutexW(SYNCHRONIZE, FALSE, NOME_MUTEX);
    // Verifica se o handle é valido, se o mutex foi aberto com sucesso
    if (!hMutex) { 
        logger(L"error", L"open_mutex", L"Erro ao abrir Mutex", GetLastError(), L"system");
        UnmapViewOfFile(sharedMem); // Desmapeia a memória
        CloseHandle(hMapFile); 
        return 1; 
    }

    std::wcout << L"Reader iniciado...\n";

    while (true) {
        // Espera para adquirir o mutex
        DWORD dwWait = WaitForSingleObject(hMutex, INFINITE);
        if (dwWait != WAIT_OBJECT_0) {
            logger(L"error", L"wait_mutex", L"Falha ao esperar pelo Mutex", GetLastError(), L"system");
            break;
        }

        // Lê a mensagem atual da memória compartilhada
        std::wstring current(sharedMem->mensagem);

        // Verifica se a flag de encerramento doi definida como true pelo writer e encerra
        if (sharedMem->encerrar_flag) {
            logger(L"info", L"exit", L"Reader encerrado por flag", 0, L"shared_memory");
            ReleaseMutex(hMutex); // libera o mutex antes de sair
            break;
        }

        // Se há mensagem nova, registra no logger
        if (!current.empty()) {
            logger(L"info", L"mutex_acquire", L"Leitura protegida por mutex", 0, L"system");
            logger(L"info", L"read", current, current.size(), L"shared_memory");
            logger(L"info", L"mutex_release", L"Reader liberou o mutex", 0, L"system");
            sharedMem->mensagem[0] = L'\0'; // Limpa a mensagem após ler, libera o buffer
        }

        ReleaseMutex(hMutex); // libera o mutex para o writer
        Sleep(200); // evita busy wait (pausa pequena antes da próxima leitura)
    }

   // Lopp encerrado, desmapeia a memoria e fecha os handles da memória e do mutex
    UnmapViewOfFile(sharedMem);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);
    return 0;
}
