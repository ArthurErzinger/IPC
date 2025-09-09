import subprocess   # Permite executar programas externos (ex.: writer.exe, reader.exe)
import threading    # Cria threads para ler a saída dos processos sem travar a interface gráfica
import json         # Tenta decodificar a saída dos processos como JSON (para exibição formatada)
import tkinter as tk
from tkinter import ttk, scrolledtext  # Widgets modernos (ttk) e caixa de texto com rolagem
import time         # Usado para adicionar pequenos delays entre inicializações de processos

class FrontEndIPCApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Front-end IPC Monitor")  # Título da janela

        # ----- Frame superior: seleção de método de comunicação e botões -----
        frame_top = ttk.Frame(root)
        frame_top.pack(pady=5, padx=5, fill='x')

        # Label que informa ao usuário o que deve selecionar
        ttk.Label(frame_top, text="Escolha o método de comunicação:").pack(side='left')

        # Combobox para selecionar o tipo de comunicação (SM, Pipe ou Socket)
        self.metodo_var = tk.StringVar(value="Memória Compartilhada")  # Valor inicial
        self.combo = ttk.Combobox(
            frame_top,
            textvariable=self.metodo_var,
            values=["Memória Compartilhada", "Pipe", "Socket"],
            state="readonly",  # Impede digitação manual, apenas seleção
            width=20
        )
        self.combo.pack(side='left', padx=5)

        # Botão para iniciar os processos
        self.btn_start = ttk.Button(frame_top, text="Iniciar", command=self.start_processos)
        self.btn_start.pack(side='left', padx=5)

        # Botão para parar os processos
        self.btn_stop = ttk.Button(frame_top, text="Parar", command=self.parar_processos)
        self.btn_stop.pack(side='left', padx=5)

        # ----- Frame principal: dividido em lado esquerdo (writer) e direito (reader) -----
        frame_main = ttk.Frame(root)
        frame_main.pack(padx=5, pady=5, fill='both', expand=True)

        # ----- Writer -----
        writer_frame = ttk.Frame(frame_main)
        writer_frame.pack(side='left', fill='both', expand=True, padx=(0, 5))
        ttk.Label(writer_frame, text="Writer (SM) / Pai (Pipe) / Cliente (Sockets)").pack()

        # Caixa de texto com rolagem para mostrar a saída do writer
        self.writer_text = scrolledtext.ScrolledText(writer_frame, height=20)
        self.writer_text.pack(fill='both', expand=True)

        # Área para digitar mensagem + botão de envio
        send_frame = ttk.Frame(writer_frame)
        send_frame.pack(fill='x', pady=5)
        self.msg_entry = ttk.Entry(send_frame)  # Campo de entrada para mensagem
        self.msg_entry.pack(side='left', fill='x', expand=True, padx=(0, 5))
        self.send_btn = ttk.Button(send_frame, text="Enviar", command=self.enviar_mensagem)
        self.send_btn.pack(side='left')

        # ----- Reader -----
        reader_frame = ttk.Frame(frame_main)
        reader_frame.pack(side='left', fill='both', expand=True, padx=(5, 0))
        ttk.Label(reader_frame, text="Reader / Server (Sockets)").pack()

        # Caixa de texto com rolagem para mostrar a saída do reader
        self.reader_text = scrolledtext.ScrolledText(reader_frame, height=25, state='disabled')
        self.reader_text.pack(fill='both', expand=True)

        # Variáveis para armazenar os processos e caminhos dos executáveis
        self.writer_proc = None
        self.reader_proc = None
        self.writer_exec = None
        self.reader_exec = None

    def parar_processos(self):
        """Força a parada dos processos caso estejam em execução"""
        if self.writer_proc:
            try:
                self.writer_proc.kill()  # Encerra o writer
            except Exception:
                pass
            self.writer_proc = None

        if self.reader_proc:
            try:
                self.reader_proc.kill()  # Encerra o reader
            except Exception:
                pass
            self.reader_proc = None

    def start_processos(self):
        """Inicia os processos de acordo com o método selecionado"""
        self.parar_processos()  # Garante que não existam processos antigos abertos

        metodo = self.metodo_var.get()  # Lê método selecionado
        self.writer_text.insert(tk.END, f"Iniciando processos ({metodo})...\n")
        self.writer_text.see(tk.END)

        if metodo == "Memória Compartilhada":
            # Define os executáveis para memória compartilhada
            self.writer_exec = r"C:\Users\Arthur\Desktop\IPC\projeto-ipc\backend\shared_memory\writer.exe"
            self.reader_exec = r"C:\Users\Arthur\Desktop\IPC\projeto-ipc\backend\shared_memory\reader.exe"
            self.iniciar_writer()
            time.sleep(1)  # Delay para dar tempo do writer criar a memória antes do reader tentar ler
            self.iniciar_reader()

        elif metodo == "Pipe":
            # Pipes geralmente usam um único processo que gerencia pai/filho
            self.writer_exec = r"C:\Users\Arthur\Desktop\IPC\projeto-ipc\backend\pipes\pipes.exe"
            try:
                # Executa o processo pipes.exe com comunicação via stdin/stdout
                self.writer_proc = subprocess.Popen(
                    [self.writer_exec],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    stdin=subprocess.PIPE,
                    text=True
                )
                # Thread que fica lendo tudo que o pipes.exe imprimir
                threading.Thread(target=self.ler_saida, args=(self.writer_proc, "Pipe"), daemon=True).start()
                self.writer_text.insert(tk.END, "Processo pipes.exe iniciado (Pai irá imprimir).\n")
                self.writer_text.see(tk.END)
            except FileNotFoundError:
                self.writer_text.insert(tk.END, "Erro: pipes.exe não encontrado.\n")

        elif metodo == "Socket":
            # Define os executáveis para sockets (cliente e servidor)
            self.writer_exec = r"C:\Users\artho\OneDrive\Desktop\IPC\projeto-ipc\backend\sockets\client.exe"
            self.reader_exec = r"C:\Users\artho\OneDrive\Desktop\IPC\projeto-ipc\backend\sockets\server.exe"
            self.iniciar_writer()
            time.sleep(1)  # Delay para evitar conflito de conexão
            self.iniciar_reader()

        else:
            self.writer_text.insert(tk.END, "Método desconhecido.\n")

    def iniciar_writer(self):
        """Inicia o writer e cria thread para monitorar sua saída"""
        try:
            self.writer_proc = subprocess.Popen(
                [self.writer_exec],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                stdin=subprocess.PIPE,
                text=True
            )
            # Thread que lê a saída do writer continuamente
            threading.Thread(target=self.ler_saida, args=(self.writer_proc, "Writer"), daemon=True).start()
        except FileNotFoundError:
            self.writer_text.insert(tk.END, f"Erro: {self.writer_exec} não encontrado.\n")
            self.writer_text.see(tk.END)

    def iniciar_reader(self):
        """Inicia o reader e cria thread para monitorar sua saída"""
        try:
            self.reader_proc = subprocess.Popen(
                [self.reader_exec],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True
            )
            # Thread que lê a saída do reader continuamente
            threading.Thread(target=self.ler_saida, args=(self.reader_proc, "Reader"), daemon=True).start()
            self.writer_text.insert(tk.END, f"Reader ({self.metodo_var.get()}) iniciado.\n")
            self.writer_text.see(tk.END)
        except FileNotFoundError:
            self.writer_text.insert(tk.END, f"Erro: {self.reader_exec} não encontrado.\n")
            self.writer_text.see(tk.END)

    def ler_saida(self, proc, nome):
        """Thread que fica lendo a saída (stdout) do processo"""
        for line in proc.stdout:
            line = line.strip()
            if not line:
                continue
            try:
                # Se a saída for JSON, formata para ficar legível
                data = json.loads(line)
                msg = json.dumps(data, indent=2, ensure_ascii=False)
                display = f"[{nome}] {msg}\n"
            except json.JSONDecodeError:
                # Caso não seja JSON, apenas exibe a linha
                display = f"[{nome}] {line}\n"

            # Decide onde exibir a mensagem (esquerda ou direita)
            if nome in ("Pipe", "Writer"):
                self.writer_text.insert(tk.END, display)
                self.writer_text.see(tk.END)
            elif nome == "Reader":
                self.reader_text.config(state='normal')
                self.reader_text.insert(tk.END, display)
                self.reader_text.see(tk.END)
                self.reader_text.config(state='disabled')

    def enviar_mensagem(self):
        """Envia o conteúdo do campo de entrada para o stdin do writer"""
        msg = self.msg_entry.get().strip()
        if not msg:
            return
        if self.writer_proc and self.writer_proc.stdin:
            try:
                self.writer_proc.stdin.write(msg + "\n")
                self.writer_proc.stdin.flush()
                self.writer_text.insert(tk.END, f"[Você] {msg}\n")
                self.writer_text.see(tk.END)
                self.msg_entry.delete(0, tk.END)
            except Exception as e:
                self.writer_text.insert(tk.END, f"Erro ao enviar mensagem: {e}\n")

# Execução principal: cria janela, instancia o app e entra no loop de eventos
if __name__ == "__main__":
    root = tk.Tk()
    app = FrontEndIPCApp(root)
    root.mainloop()
