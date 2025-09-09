#include <windows.h>
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

//Definições sobre a memória compartilhada e o mutex
const wchar_t* NOME_MEMORIA = L"MinhaMemoria";
const wchar_t* NOME_MUTEX   = L"MeuMutex";
const int TAM_MEMORIA = 256;

// Estrtura de dados que serão armazenados na memória compartilhada
struct DadosCompartilhados {
    wchar_t mensagem[TAM_MEMORIA]; //buffer da mensagem
    bool encerrar_flag; //flag para sinalizar encerramento
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

// Logger em JSON para formatar as mensagens de log, para a comunicação com o frontend
void logger(const std::wstring& level, const std::wstring& event,
            const std::wstring& msg, int bytes, const std::wstring& peer) {
    std::wcout << L"{\n"
               << L"  \"module\": \"ipc\",\n"
               << L"  \"role\": \"writer\",\n"
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
    /*Cria memória compartilhada
    cria um handle (idenrificador para o SO) para a memória compartilhada
    - INVALID_HANDLE_VALUE: memória do sistema (RAM)
    - PAGE_READWRITE: permite leitura e escrita
    - NOME_MEMORIA: permite que outros processos a localizem
    - sizeof(DadosCompartilhados): tamanho da memória */
    HANDLE hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(DadosCompartilhados), NOME_MEMORIA);
    // Verifica se o handle é valido, se a memória foi criada com sucesso
    if (!hMapFile) {
        logger(L"error", L"criando memória", L"Erro ao criar memória compartilhada", GetLastError(), L"system");
        return 1;
    }

    /*Mapeia a memória compartilhada para o espaço de endereço do processo
    cria um ponteiro do tipo DadosCompartilhados que aponta para a memoria compartilhada (pontMem)
    - hMapFile: handle da memória compartilhada criada acima
    - FILE_MAP_ALL_ACCESS: acesso de leitura e escrita
    - 0 (dwFileOffsetHigh) e 0 (dwFileOffsetLow): começa a mapear do início da memória
    - sizeof(SharedData): quantidade de bytes que serão mapeados*/
    DadosCompartilhados* pontMem = (DadosCompartilhados*)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DadosCompartilhados));
    // Verifica se o o ponteiro que a aponta para a memória é valido, se a memória foi mapeada com sucesso
    if (!pontMem) {
        logger(L"error", L"mapeando memória", L"Erro ao mapear memória", GetLastError(), L"system");
        CloseHandle(hMapFile); //Fecha o handle da memória compartilhada
        return 1;
    }

    /* Cria mutex para sincronização, apenas um processo acesse a região crítica (memória compartilhada) por vez.
    - nullptr = usa atributos de segurança padrão
    - FALSE = o mutex não é adquirido imediatamente pelo processo que o cria
    - NOME_MUTEX = nome do mutex, permitindo que outros processos abram o mesmo mutex */
    HANDLE hMutex = CreateMutexW(nullptr, FALSE, NOME_MUTEX);
    // Verifica se o handle é valido, se o mutex foi criada com sucesso
    if (!hMutex) {
        logger(L"error", L"criando mutex", L"Erro ao criar Mutex", GetLastError(), L"system");
        UnmapViewOfFile(pontMem); // Desmapeia a memória
        CloseHandle(hMapFile);
        return 1;
    }
    //Mensagem inicial para o usuário
    std::wcout << L"Writer iniciado...\nDigite mensagens. Digite 'sair' para encerrar.\n";
    //Variável para armazenar a entrada do usuário
    std::wstring input;

    while (true) {
        std::getline(std::wcin, input); // lê uma linha do teclado e armazena na variável input, preservando caracteres Unicode.

        /* Espera para adquirir mutex antes de escrever na memória compartilhada
        WaitForSingleObject bloqueia o processo até que o mutex esteja disponível
        INFINITE = espera indefinidamente*/
        DWORD dwWait = WaitForSingleObject(hMutex, INFINITE);
        // Verifica se o mutex foi adiquirido com sucerro
        if (dwWait != WAIT_OBJECT_0) { //waitforSingleObject retornou WAIT_OBJECT_0 (quer dizer que foi adiquirido)
            logger(L"error", L"esperando pelo mutex", L"Falha ao esperar pelo Mutex", GetLastError(), L"system");
            break;
        }

        // Verifica se usuário digitou "sair" para encerrar
        if (input == L"sair") {
            pontMem->encerrar_flag = true; // defini a flag de encerramento como verdadeira para poder encerrar o reader
            //Loggers com as informações sobre o encerramento e do mutex
            logger(L"info", L"mutex adiquiro", L"Escrita protegida por mutex", 0, L"system");
            logger(L"info", L"encerrar", L"Encerramento Solicitado", 0, L"shared_memory");
            logger(L"info", L"mutex liberado", L"Writer liberou o mutex", 0, L"system");
            ReleaseMutex(hMutex); // libera mutex antes de sair do loop
            break;
        }
        // Se a entrada não estiver vazia, escreve na memória compartilhada
        if (!input.empty()) {
            // escreve mensagem na memória compartilhada
            wcsncpy(pontMem->mensagem, input.c_str(), TAM_MEMORIA - 1); //copia a entrada para o buffer message do ponteiro sharedMem, até TAM_MEMORIA - 1 
            pontMem->mensagem[TAM_MEMORIA - 1] = L'\0'; //O último caractere é colocado como '\0'
            //Logger com as informaÇões da mensagem e do mutex
            logger(L"info", L"mutex adiquirido", L"Escrita protegida por mutex", 0, L"system");
            logger(L"info", L"Escrita", input, input.size(), L"shared_memory");
            logger(L"info", L"mutex liberado", L"Writer liberou o mutex", 0, L"system");
        }

        ReleaseMutex(hMutex); // libera mutex para o reader
    }

    // Lopp encerrado, desmapeia a memoria e fecha os handles da memória e do mutex
    UnmapViewOfFile(pontMem);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);
    return 0;
}
