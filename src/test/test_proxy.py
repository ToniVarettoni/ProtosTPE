#!/usr/bin/env python3
import socket
import threading
import traceback

PROXY_HOST = "127.0.0.1"
PROXY_PORT = 1080
USERNAME = b"username"
PASSWORD = b"password"
TARGET_HOST = "example.org"
TARGET_PORT = 80
TIMEOUT = 5

CONCURRENT_CLIENTS = 50   # <<< ajustá este número para stress-test
results = []              # lista global con resultados de cada hilo


def read_http_response_debug(sock, client_id):
    print(f"\n[Cliente {client_id}] Leyendo respuesta...")

    chunks = []
    total_bytes = 0

    while True:
        try:
            chunk = sock.recv(4096)

            if chunk == b"":
                print(f"[Cliente {client_id}] ❗ Recibido EOF/FIN — no llegaron más datos")

                import errno
                err = sock.getsockopt(socket.SOL_SOCKET, socket.SO_ERROR)

                if err != 0:
                    print(f"[Cliente {client_id}] ❗ SO_ERROR = {errno.errorcode.get(err, err)} ({err})")
                else:
                    print(f"[Cliente {client_id}] SO_ERROR = 0 (cierre normal)")

                if total_bytes > 0:
                    print(f"[Cliente {client_id}] Se recibieron {total_bytes} bytes antes del cierre")
                    preview = b"".join(chunks)[:500]
                    print(preview.decode("utf-8", errors="replace"))
                else:
                    print(f"[Cliente {client_id}] ❗ No se recibió NINGÚN byte antes del cierre")

                return b"".join(chunks)

            chunks.append(chunk)
            total_bytes += len(chunk)

        except socket.timeout:
            print(f"[Cliente {client_id}] ❗ Timeout mientras se leía respuesta")
            break

    return b"".join(chunks)



# ------------------------- CLIENTE INDIVIDUAL -----------------------------

def run_single_client(client_id):
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(TIMEOUT)
        sock.connect((PROXY_HOST, PROXY_PORT))

        # 1) HELLO
        sock.sendall(b"\x05\x01\x02")
        resp = sock.recv(2)
        if len(resp) != 2 or resp[0] != 0x05:
            raise RuntimeError("HELLO inválido")
        method = resp[1]

        # 2) AUTH
        if method == 0x02:
            msg = (
                b"\x01"
                + bytes([len(USERNAME)]) + USERNAME
                + bytes([len(PASSWORD)]) + PASSWORD
            )
            sock.sendall(msg)
            r = sock.recv(2)
            if len(r) != 2 or r[1] != 0x00:
                raise RuntimeError("Auth falló")
        elif method != 0x00:
            raise RuntimeError("Método no soportado")

        # 3) CONNECT
        host_b = TARGET_HOST.encode()
        req = (
            b"\x05\x01\x00\x03" +
            bytes([len(host_b)]) + host_b +
            TARGET_PORT.to_bytes(2, "big")
        )
        sock.sendall(req)

        h = sock.recv(4)
        if len(h) != 4 or h[1] != 0x00:
            raise RuntimeError("CONNECT falló")

        atyp = h[3]
        if atyp == 0x01:
            sock.recv(4)
        elif atyp == 0x04:
            sock.recv(16)
        elif atyp == 0x03:
            ln = sock.recv(1)[0]
            sock.recv(ln)
        else:
            raise RuntimeError("ATYP inválido")
        sock.recv(2)

        # 4) HTTP REQUEST
        http = (
            f"GET / HTTP/1.1\r\n"
            f"Host: {TARGET_HOST}\r\n"
            f"Connection: close\r\n\r\n"
        ).encode()
        sock.sendall(http)

        # 5) READ RESPONSE
        data = read_http_response_debug(sock, client_id)


        if b"Example Domain" not in data:
            print(f"\n--- Cliente {client_id}: respuesta HTTP recibida (preview 500 chars) ---")
            print(data[:500].decode("utf-8", errors="replace"))
            print("--- FIN RESPUESTA ---\n")
            raise RuntimeError("Respuesta HTTP incorrecta")

        sock.close()
        results.append((client_id, "OK"))

    except Exception as e:
        results.append((client_id, f"FAIL: {e}"))
        traceback.print_exc()


# ------------------------- MAIN CONCURRENCY TEST -----------------------------

def main():
    threads = []

    for i in range(CONCURRENT_CLIENTS):
        t = threading.Thread(target=run_single_client, args=(i,))
        threads.append(t)
        t.start()

    for t in threads:
        t.join()

    ok = sum(1 for _, r in results if r == "OK")
    fail = CONCURRENT_CLIENTS - ok

    print("\n==========================================")
    print(f"   TEST DE CONCURRENCIA: {CONCURRENT_CLIENTS} clientes")
    print("==========================================")
    print(f"✔ Éxitos: {ok}")
    print(f"❌ Fallos : {fail}")

    if fail > 0:
        print("\nLista de fallos:")
        for cid, r in results:
            if r != "OK":
                print(f"  Cliente {cid}: {r}")

    print("\nFin del test\n")


if __name__ == "__main__":
    main()

