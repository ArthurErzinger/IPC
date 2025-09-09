import subprocess
import time

SERVER = r"projeto-ipc\backend\sockets\server.exe"
CLIENT = r"projeto-ipc\backend\sockets\client.exe"

def run_test(comandos):
    # inicia servidor
    srv = subprocess.Popen([SERVER], stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    time.sleep(1)  # espera ele dar listen

    # inicia cliente
    cli = subprocess.Popen([CLIENT], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    saida_cliente, _ = cli.communicate("\n".join(comandos) + "\n", timeout=5)
    srv.terminate()
    saida_servidor = srv.stdout.read()

    print("=== Saída do cliente ===")
    print(saida_cliente)
    print("=== Saída do servidor ===")
    print(saida_servidor)

    return saida_cliente, saida_servidor

if __name__ == "__main__":
    # Teste 1: ping → pong
    run_test(["ping", "sair"])

    # Teste 2: oi → hello
    run_test(["oi", "sair"])

    # Teste 3: comando desconhecido
    run_test(["abc", "sair"])
