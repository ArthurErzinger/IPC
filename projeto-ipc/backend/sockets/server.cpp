#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Abertura do socket do servidor
    //Função socket() cria um socket IPv6 (AF_INET6) do tipo TCP.
    // O socket é do tipo STREAM (SOCK_STREAM), que é usado para conexões 
    //orientadas a conexão, como TCP. Ele diferentemente do DGRAM (SOCK_DGRAM)
    // que é usado para conexões sem conexão, como UDP, e além disto o STREAM envia
    //byte a byte, enquanto o DGRAM envia mensagens completas.
    // O terceiro parâmetro (0) indica que o protocolo padrão para o tipo de socket
    //e família de endereços deve ser usado. Para SOCK_STREAM, o protocolo padrão
    //é TCP (Transmission Control Protocol).

    SOCKET serverSocket = socket(AF_INET6, SOCK_STREAM, 0);

    // sockaddr_in6 é uma estrutura (struct) usada para representar um endereço IPv6.
    // Ela possui campos como família (AF_INET6), porta (sin6_port), endereço IP (sin6_addr),
    // além de informações adicionais (flowinfo e scope_id).
    // A variável serverAddr é uma instância dessa estrutura, onde definimos os valores
    // específicos que o servidor usará (ex.: família = AF_INET6, porta = 8080, IP = ::1).
    // O "{}" no final inicializa todos os campos da struct com zero por padrão.
    //Basicamente, o sockaddr_in6 é um struct que terá todas as informações necessárias para se referenciar
    //um endereço IPv6. 


    sockaddr_in6 serverAddr{};
    serverAddr.sin6_family = AF_INET6; // Família de endereços IPv6
    serverAddr.sin6_port = htons(8080); // Porta 8080 (htons converte para formato de rede)
    serverAddr.sin6_addr = in6addr_loopback; // Endereço de loopback IPv6 (::1) - aceita somente conexões locais


    // Função bind() associa o socket do servidor (serverSocket) ao endereço e porta
    //especificados na estrutura serverAddr. Isso permite que o servidor escute
    //conexões na porta 8080 do endereço de loopback IPv6 (::1).
    //serverSocket é o socket que foi criado anteriormente com a função socket().
    //serverAddr é um ponteiro para a estrutura sockaddr_in6 que contém o endereço e a porta
    //aos quais o socket será associado.
    //sizeof(serverAddr) fornece o tamanho da estrutura serverAddr, necessário para a função bind().

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));

    // A função listen() coloca o socket do servidor em modo passivo, ou seja,
    // pronto para aceitar conexões de clientes. Antes do listen, o socket existe
    // e está associado a um endereço (via bind), mas ainda não escuta pedidos.
    //
    // listen(serverSocket, 5);
    //
    // 1º parâmetro (serverSocket): o socket do servidor, previamente criado com socket()
    //    e associado a um IP/porta. Esse socket NÃO será usado para enviar/receber dados,
    //    apenas para escutar novas conexões.
    //
    // 2º parâmetro (5): define o backlog, isto é, o tamanho da fila de conexões pendentes.
    //    Quando vários clientes tentam se conectar ao mesmo tempo, o kernel os coloca
    //    em uma fila de espera. Aqui, até 5 conexões podem ficar pendentes aguardando
    //    que o servidor chame accept(). Se a fila encher, novas conexões podem ser
    //    rejeitadas.
    //
    // Em resumo: bind() dá um endereço ao socket, listen() abre a "sala de espera"
    // para conexões, e accept() tira um cliente dessa fila e cria um novo socket
    // exclusivo para conversar com ele.

    listen(serverSocket, 5);

    std::cout << "Servidor aguardando conexões em [::1]:8080...\n";


    // A função accept() retira da fila (criada pelo listen) a próxima conexão
    // pendente e cria um novo socket exclusivo para se comunicar com aquele cliente.
    //
    // SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
    //
    // 1º parâmetro (serverSocket): é o socket do servidor que está em modo passivo
    //    (após socket(), bind() e listen()). Ele serve apenas como "porta de entrada"
    //    para novas conexões. O serverSocket continua existindo depois do accept()
    //    para aceitar outros clientes.
    //
    // 2º parâmetro (nullptr): se não for nullptr, poderia ser um ponteiro para uma
    //    struct sockaddr que receberia informações sobre o cliente (ex.: IP e porta).
    //    Como aqui usamos nullptr, essas informações são descartadas.
    //
    // 3º parâmetro (nullptr): se não for nullptr, indicaria o tamanho da struct
    //    passada no segundo parâmetro. Também está como nullptr porque não queremos
    //    capturar os dados do cliente.
    //
    // Retorno: accept() retorna um NOVO socket (clientSocket), diferente do serverSocket.
    // Esse novo socket representa a conexão direta com um cliente específico e é nele
    // que usaremos recv() e send() para trocar dados. Cada cliente aceito gera um
    // socket diferente. O serverSocket permanece aberto para aceitar futuras conexões.
    //
    // Em resumo: listen() cria a "sala de espera" das conexões,
    // e accept() chama o próximo cliente dessa fila para dentro,
    // entregando um novo "canal privado" (clientSocket) entre servidor e cliente.

    SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);


    // A função recv() lê dados enviados pelo cliente através de um socket já conectado.
    // Os bytes recebidos são copiados para o buffer fornecido.
    //
    // int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    //
    // 1º parâmetro (clientSocket): é o socket retornado pelo accept(), ou seja,
    //    a conexão direta com um cliente específico. É nesse canal que chegam
    //    os dados do cliente.
    //
    // 2º parâmetro (buffer): ponteiro para um array de char onde os bytes lidos
    //    serão armazenados. Aqui usamos "buffer[5096]", logo até 5096 bytes podem
    //    ser copiados em uma única chamada.
    //
    // 3º parâmetro (sizeof(buffer)): define o tamanho máximo do buffer.
    //    Garante que recv() não copie mais dados do que o espaço disponível.
    //    Se o cliente enviar mais dados do que cabe no buffer, o restante ficará
    //    armazenado no buffer interno do socket e poderá ser lido em chamadas
    //    futuras de recv().
    //
    // 4º parâmetro (0): flags de controle (normalmente 0).
    //    Existem opções avançadas como MSG_OOB (dados fora de banda),
    //    mas em aplicações simples usamos sempre 0.
    //
    // Retorno (bytesReceived):
    //    > 0 : número de bytes realmente lidos do socket.
    //    == 0: o cliente fechou a conexão de forma ordenada (não há mais dados).
    //    < 0 : ocorreu um erro (usar WSAGetLastError() para detalhes).
    //
    // Observação: o buffer não é automaticamente terminado em '\0'. Se quiser
    // tratá-lo como string em C/C++, é preciso garantir isso manualmente, por
    // exemplo:
    //     buffer[bytesReceived] = '\0';
    // Assim, o conteúdo pode ser manipulado como std::string ou impresso em cout.
    //
    // Em resumo: recv() lê os dados que chegaram do cliente, devolve quantos bytes
    // foram lidos, e cabe ao servidor interpretar esses bytes (texto, comando, etc.).


    // char buffer[5096] = {0};
    // char bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

    // std::cout << "Mensagem recebida do cliente: " << buffer << std::endl;

    // A função send() envia dados através de um socket já conectado a um cliente.
    //
    // send(clientSocket, mensagem, tamanho, 0);
    //
    // 1º parâmetro (clientSocket): é o socket retornado pelo accept(),
    //    representando a conexão direta com um cliente específico.
    //    O send() escreve bytes nesse canal.
    //
    // 2º parâmetro (mensagem): ponteiro para os dados que serão enviados.
    //    Pode ser um array de char ou o resultado de message.c_str() no caso
    //    de std::string. O conteúdo não precisa terminar em '\0', pois o tamanho
    //    é indicado no próximo parâmetro.
    //
    // 3º parâmetro (tamanho): número de bytes que devem ser enviados.
    //    Ex.: para "hello" usamos 5, já que são 5 caracteres.
    //    O send() não adiciona '\0' automaticamente — cabe ao programa decidir
    //    o que e quantos bytes enviar.
    //
    // 4º parâmetro (0): flags de controle (normalmente 0).
    //    Existem opções avançadas como MSG_OOB (dados fora de banda), mas em
    //    aplicações simples usamos sempre 0.
    //
    // Retorno:
    //    > 0 : número de bytes efetivamente aceitos para envio.
    //    == SOCKET_ERROR: ocorreu um erro (usar WSAGetLastError() para detalhes).
    //
    // Observações importantes:
    // - Em sockets TCP (SOCK_STREAM), o send() pode enviar menos bytes do que o
    //   solicitado, principalmente em mensagens grandes. Nesse caso, é preciso
    //   chamar send() repetidamente até que todos os dados tenham sido enviados.
    // - Em sockets UDP (SOCK_DGRAM), cada send() corresponde a um datagrama
    //   completo.
    //
    // Em resumo: send() coloca bytes no canal da conexão para que o cliente os
    // receba. O servidor decide exatamente quantos bytes mandar, e cabe ao cliente
    // interpretar esses dados corretamente.

    // Responder ao cliente de acordo com a mensagem recebida


    while (true){
        char buffer2[5096] = {0};

        int bytesReceived = recv(clientSocket, buffer2, sizeof(buffer2), 0);

        std::string mensagemCliente(buffer2, bytesReceived);

        std::cout << "Mensagem recebida do cliente: " << mensagemCliente << std::endl;

        if (mensagemCliente == "oi")
        {
            send(clientSocket, "hello", 5, 0);
        } else if (mensagemCliente == "sair")
        {
            std::cout << "Fechando socket..." << std::endl;
            send(clientSocket, "Fechando socket...", 18, 0);
            break;
        } else if (mensagemCliente == "ping")
        {
            send(clientSocket, "pong", 4, 0);
        }
        
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
}
