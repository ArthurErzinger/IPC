# Comunicação entre Processos (IPC) — RA1

Este repositório implementa **três mecanismos de IPC** em C++ com **interface gráfica em Python (Tkinter)** para visualizar o fluxo de dados em tempo real: **Pipes Anônimos**, **Sockets (locais)** e **Memória Compartilhada**. O projeto segue a organização, os requisitos funcionais e não‑funcionais, e a rubrica do enunciado da atividade avaliativa (RA1).

> ⚠️ Observação importante: os computadores de laboratório podem ter restrições de instalação. Compile e teste **no mesmo ambiente** onde será feita a prova de autoria.


---

## 1) Objetivo Geral
- Desenvolver uma aplicação com backend em **C++ (Windows)** para três mecanismos de IPC (Pipes, Sockets, Memória Compartilhada);
- Permitir, via **frontend em Python/Tkinter**, que o usuário **inicie/encerre os processos**, **envie mensagens** e **acompanhe logs** e **estados** dos mecanismos em tempo real;
- **Separar** claramente **backend** (C++) e **frontend** (Python), com um **protocolo simples (JSON via stdout)**.


---

## 2) Arquitetura & Protocolo

### 2.1 Visão Geral
```
projeto-ipc/
├── backend/                      # C++ (Windows)
│   ├── pipes/                    # Pipes anônimos (pai ↔ filho)
│   │   └── pipes.cpp
│   ├── sockets/                  # TCP IPv6 local (::1:8080)
│   │   ├── server.cpp
│   │   └── client.cpp
│   └── shared_memory/            # Memória compartilhada + mutex
│       ├── writer.cpp
│       └── reader.cpp
├── frontend/
│   └── frontend.py               # Tkinter: UI + orquestração de processos
└── teste_sockets.py              # Testes rápidos do par server/client
```

### 2.2 Protocolo de logs (JSON via stdout)
Todos os módulos **escrevem em stdout** mensagens JSON que o frontend consome e renderiza.

**Exemplo (Sockets — Server):**
```json
{
  "module": "sockets",
  "role": "server",
  "level": "INFO",
  "event": "listen",
  "ts": "Wed Sep 04 12:34:56 2025",
  "details": { "msg": "Listening for connections", "bytes": 0, "peer": "::1:8080" }
}
```

**Exemplo (Memória Compartilhada — Writer):**
```json
{
  "module": "ipc",
  "role": "writer",
  "level": "info",
  "event": "escrita",
  "ts": "2025-09-04T15:22:10Z",
  "details": { "msg": "conteúdo escrito", "bytes": 12, "peer": "local" }
}
```

**Exemplo (Pipes — Pai):**
```json
{
  "module": "ipc",
  "role": "pai",
  "level": "info",
  "event": "envio",
  "ts": "2025-09-04T15:30:01Z",
  "details": { "msg": "hello", "bytes": 5 }
}
```

> **Observação:** nomes de campos e formatos podem variar levemente entre módulos; a UI tolera e exibe o melhor possível.


---

## 3) Requisitos Atendidos (resumo)

- **UI clara e intuitiva:** combobox para selecionar mecanismo; áreas de log para **Writer/Pai/Cliente** (esquerda) e **Reader/Servidor** (direita); campo de input + botão **Enviar**.
- **Iniciar/Parar processos** pelo frontend (cada mecanismo).
- **Envio de mensagens** a partir da UI (redirigidas para stdin do processo correspondente).
- **Logs estruturados (JSON)** exibidos em tempo real.
- **Representação do estado:** mensagens e eventos de cada mecanismo (listen/accept/recv/send; criação/uso de pipes; escrita/leitura na memória, lock/unlock de mutex).
- **Frontend não bloqueia:** leitura assíncrona de stdout em threads separadas.


---

## 4) Compilação (Windows)

> Testado no Windows (10/11). Os códigos usam **Windows API** (`windows.h`) e **Winsock2**.

### 4.1 MinGW‑w64 (g++)
1) Instale o **MinGW‑w64** e adicione `g++` ao `PATH`.
2) A partir da raiz do repositório, rode:

```bash
# Sockets (linka Ws2_32)
g++ -std=c++17 -O2 -Wall backend/sockets/server.cpp -o backend/sockets/server.exe -lws2_32
g++ -std=c++17 -O2 -Wall backend/sockets/client.cpp -o backend/sockets/client.exe -lws2_32

# Pipes
g++ -std=c++17 -O2 -Wall backend/pipes/pipes.cpp -o backend/pipes/pipes.exe

# Memória compartilhada
g++ -std=c++17 -O2 -Wall backend/shared_memory/writer.cpp -o backend/shared_memory/writer.exe
g++ -std=c++17 -O2 -Wall backend/shared_memory/reader.cpp -o backend/shared_memory/reader.exe
```


## 5) Execução (Frontend)

### 5.1 Requisitos
- **Python 3.10+** (Tkinter já incluso nas builds “completas” de Python for Windows).
- Nenhuma dependência extra é necessária.

### 5.2 Rodando
1) **Ajuste os caminhos** no `frontend/frontend.py` para **usar caminhos relativos** (recomendado). Exemplos sugeridos:

```python
# Memória Compartilhada
self.writer_exec = r".\backend\shared_memory\writer.exe"
self.reader_exec = r".\backend\shared_memory\reader.exe"

# Pipes
self.writer_exec = r".\backend\pipes\pipes.exe"

# Sockets
self.reader_exec = r".\backend\sockets\server.exe"  # inicia o servidor
self.writer_exec = r".\backend\sockets\client.exe"  # inicia o cliente
```

> O arquivo pode vir com **caminhos absolutos** do ambiente de desenvolvimento. **Troque para relativos** como acima.

2) Execute:
```bash
python frontend/frontend.py
```

3) Na UI:
- Selecione **Memória Compartilhada** | **Pipe** | **Socket**;
- Clique **Iniciar** (a UI vai iniciar os processos e começar a ler os logs);
- Digite a mensagem e clique **Enviar**;
- Clique **Parar** para encerrar processos em execução.


---

## 6) Testes

### 6.1 Teste rápido (sockets)
Há um script simples para validar o par **server.exe** + **client.exe**:

```bash
python teste_sockets.py
```
Ele executa:
- `ping → pong`;
- `oi → hello`;
- um comando **desconhecido** (deve ser tratado).

### 6.2 Boas práticas de testes
- **Back‑end (C++):** priorize testes de troca de dados e tratamento de erros (timeouts, portas ocupadas, peer desconectado, etc.).
- **Integração (Frontend/Backend):** valide o parsing do JSON e a não‑bloqueio da UI.


---

## 7) Organização, Colaboração e Workflow

Sugerido (para grupos de até 4):
- **Anderson — Pipes + Coordenação:** pipes anônimos, padronização de logs;
- **Arthur — Sockets:** cliente/servidor TCP local, testes unitários;
- **Mateus Marochi — Memória Compartilhada:** estrutura de dados + mutex/semáforo;
- **Matheus Mazzucco — Frontend & Integração:** UI Tkinter + threads de leitura de stdout.

---
