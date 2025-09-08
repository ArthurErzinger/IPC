import subprocess
import threading
import json
import tkinter as tk
from tkinter import ttk, scrolledtext
import time  # <-- adicionado

class FrontEndIPCApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Front-end IPC Monitor")

        # Frame superior para escolher o método
        frame_top = ttk.Frame(root)
        frame_top.pack(pady=5, padx=5, fill='x')
        ttk.Label(frame_top, text="Escolha o método de comunicação:").pack(side='left')
        self.metodo_var = tk.StringVar(value="Memória Compartilhada")
        self.combo = ttk.Combobox(
            frame_top,
            textvariable=self.metodo_var,
            values=["Memória Compartilhada", "Pipe", "Socket"],
            state="readonly",
            width=20
        )
        self.combo.pack(side='left', padx=5)
        self.btn_start = ttk.Button(frame_top, text="Iniciar", command=self.start_processos)
        self.btn_start.pack(side='left', padx=5)
        self.btn_stop = ttk.Button(frame_top, text="Parar", command=self.parar_processos)
        self.btn_stop.pack(side='left', padx=5)

        # Frame principal com duas janelas lado a lado
        frame_main = ttk.Frame(root)
        frame_main.pack(padx=5, pady=5, fill='both', expand=True)

        # Writer (esquerda)
        writer_frame = ttk.Frame(frame_main)
        writer_frame.pack(side='left', fill='both', expand=True, padx=(0,5))
        ttk.Label(writer_frame, text="Writer / Pai (Pipe)").pack()
        self.writer_text = scrolledtext.ScrolledText(writer_frame, height=20)
        self.writer_text.pack(fill='both', expand=True)

        # Entrada e botão de envio
        send_frame = ttk.Frame(writer_frame)
        send_frame.pack(fill='x', pady=5)
        self.msg_entry = ttk.Entry(send_frame)
        self.msg_entry.pack(side='left', fill='x', expand=True, padx=(0,5))
        self.send_btn = ttk.Button(send_frame, text="Enviar", command=self.enviar_mensagem)
        self.send_btn.pack(side='left')

        # Reader (direita) - usado para Memória e Socket
        reader_frame = ttk.Frame(frame_main)
        reader_frame.pack(side='left', fill='both', expand=True, padx=(5,0))
        ttk.Label(reader_frame, text="Reader / Cliente").pack()
        self.reader_text = scrolledtext.ScrolledText(reader_frame, height=25, state='disabled')
        self.reader_text.pack(fill='both', expand=True)

        self.writer_proc = None
        self.reader_proc = None
        self.writer_exec = None
        self.reader_exec = None

    def parar_processos(self):
        if self.writer_proc:
            try:
                self.writer_proc.kill()
            except Exception:
                pass
            self.writer_proc = None
        if self.reader_proc:
            try:
                self.reader_proc.kill()
            except Exception:
                pass
            self.reader_proc = None

    def start_processos(self):
        self.parar_processos()  # garante que processos antigos foram encerrados

        metodo = self.metodo_var.get()
        self.writer_text.insert(tk.END, f"Iniciando processos ({metodo})...\n")
        self.writer_text.see(tk.END)

        if metodo == "Memória Compartilhada":
            self.writer_exec = r"C:\Users\Arthur\Desktop\coding\IPC\projeto-ipc\backend\shared_memory\writer.exe"
            self.reader_exec = r"C:\Users\Arthur\Desktop\coding\IPC\projeto-ipc\backend\shared_memory\reader.exe"
            self.iniciar_writer()
            time.sleep(1)  # <-- delay de 1 segundo antes de iniciar o reader
            self.iniciar_reader()

        elif metodo == "Pipe":
            self.writer_exec = r"C:\Users\Arthur\Desktop\coding\IPC\projeto-ipc\backend\pipes\pipes.exe"
            try:
                self.writer_proc = subprocess.Popen(
                    [self.writer_exec], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE, text=True
                )
                threading.Thread(target=self.ler_saida, args=(self.writer_proc, "Pipe"), daemon=True).start()
                self.writer_text.insert(tk.END, "Processo pipes.exe iniciado (Pai irá imprimir).\n")
                self.writer_text.see(tk.END)
            except FileNotFoundError:
                self.writer_text.insert(tk.END, "Erro: pipes.exe não encontrado.\n")

        elif metodo == "Socket":
            self.writer_exec = r"C:\Users\Arthur\Desktop\coding\IPC\projeto-ipc\backend\sockets\client.exe"
            self.reader_exec = r"C:\Users\Arthur\Desktop\coding\IPC\projeto-ipc\backend\sockets\server.exe"
            self.iniciar_writer()
            time.sleep(1)  # <-- delay antes do client socket
            self.iniciar_reader()

        else:
            self.writer_text.insert(tk.END, "Método desconhecido.\n")

    def iniciar_writer(self):
        try:
            self.writer_proc = subprocess.Popen(
                [self.writer_exec], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE, text=True
            )
            threading.Thread(target=self.ler_saida, args=(self.writer_proc, "Writer"), daemon=True).start()
        except FileNotFoundError:
            self.writer_text.insert(tk.END, f"Erro: {self.writer_exec} não encontrado.\n")
            self.writer_text.see(tk.END)

    def iniciar_reader(self):
        try:
            self.reader_proc = subprocess.Popen(
                [self.reader_exec], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
            )
            threading.Thread(target=self.ler_saida, args=(self.reader_proc, "Reader"), daemon=True).start()
            self.writer_text.insert(tk.END, f"Reader ({self.metodo_var.get()}) iniciado.\n")
            self.writer_text.see(tk.END)
        except FileNotFoundError:
            self.writer_text.insert(tk.END, f"Erro: {self.reader_exec} não encontrado.\n")
            self.writer_text.see(tk.END)

    def ler_saida(self, proc, nome):
        for line in proc.stdout:
            line = line.strip()
            if not line:
                continue
            try:
                data = json.loads(line)
                msg = json.dumps(data, indent=2, ensure_ascii=False)
                display = f"[{nome}] {msg}\n"
            except json.JSONDecodeError:
                display = f"[{nome}] {line}\n"

            if nome in ("Pipe", "Writer"):
                self.writer_text.insert(tk.END, display)
                self.writer_text.see(tk.END)
            elif nome == "Reader":
                self.reader_text.config(state='normal')
                self.reader_text.insert(tk.END, display)
                self.reader_text.see(tk.END)
                self.reader_text.config(state='disabled')

    def enviar_mensagem(self):
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

if __name__ == "__main__":
    root = tk.Tk()
    app = FrontEndIPCApp(root)
    root.mainloop()
