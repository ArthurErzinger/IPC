# Comunicação entre Processos (IPC) – RA1  

Este repositório contém o desenvolvimento da atividade avaliativa da disciplina **Comunicação entre Processos (RA1)**.  
O projeto implementa e demonstra, de forma prática e visual, os principais mecanismos de **Inter-Process Communication (IPC)** fornecidos pelos sistemas operacionais.  

---

## 🔹 Mecanismos de IPC Implementados
- **Pipes Anônimos**
- **Sockets Locais**
- **Memória Compartilhada**

---

## 🔹 Estrutura do Projeto
```
projeto-ipc/
├── backend/
│   ├── pipes/          # Implementação com Pipes Anônimos
│   ├── sockets/        # Implementação com Sockets Locais
│   └── shared_memory/  # Implementação com Memória Compartilhada
│
├── frontend/           # Interface gráfica (Python ou HTML/JS)
├── tests/              # Testes unitários e de integração
└── docs/               # Documentação do projeto
```

---

## 🔹 Objetivos do Projeto
- Permitir que o usuário selecione o mecanismo de IPC desejado.  
- Enviar e receber mensagens entre processos de forma interativa.  
- Exibir **logs**, **estados** e **representações gráficas** de cada mecanismo.  
- Proporcionar aprendizado prático sobre **sincronização e comunicação entre processos**.  

---

## 🔹 Tecnologias Utilizadas
- **Backend:** C++23 / C / Rust  
- **Frontend:** Python (PyQt/Tkinter) ou HTML/JavaScript  
- **Controle de versão:** Git e GitHub  

---

## 🔹 Como Executar
1. Clone o repositório:  
   ```bash
   git clone https://github.com/usuario/repositorio.git
   cd repositorio
   ```
2. Compile o backend (exemplo com CMake):  
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ```
3. Execute o frontend (Python ou HTML/JS).  
4. Interaja com os processos pela interface gráfica.  

---

## 🔹 Contribuição
- Fazer commits frequentes e descritivos.  
- Sempre executar `git pull` antes de iniciar o trabalho.  
- Documentar cada parte desenvolvida no `README.md`.  
- Resolver conflitos de forma colaborativa.  

---

## 🔹 Licença
Projeto desenvolvido para fins acadêmicos.  
