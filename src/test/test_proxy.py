#!/usr/bin/env python3
import asyncio
import socket

PROXY_HOST = "127.0.0.1"
PROXY_PORT = 1080
USERNAME = b"username"
PASSWORD = b"password"
TARGET_HOST = "www.example.org"
TARGET_PORT = 80

CONCURRENCY = 200   # Cambialo a 500 cuando lo tengas estable
TIMEOUT = 5


def make_hello():
    return b"\x05\x01\x02"   # VER=5, NMETHODS=1, METHOD=2 (username/pass)


def make_auth():
    return (
        b"\x01"
        + bytes([len(USERNAME)]) + USERNAME
        + bytes([len(PASSWORD)]) + PASSWORD
    )


def make_connect():
    host = TARGET_HOST.encode()
    return (
        b"\x05\x01\x00\x03"
        + bytes([len(host)]) + host
        + TARGET_PORT.to_bytes(2, 'big')
    )


def make_http():
    return (
        f"GET / HTTP/1.1\r\n"
        f"Host: {TARGET_HOST}\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    

async def socks5_request(i, reference_body):
    """
    Realiza todo el flujo SOCKS5 paso a paso.
    Devuelve True si la respuesta coincide con la referencia, False si no.
    """
    try:
        reader, writer = await asyncio.wait_for(
            asyncio.open_connection(PROXY_HOST, PROXY_PORT),
            timeout=TIMEOUT
        )

        # HELLO
        writer.write(make_hello())
        await writer.drain()
        resp = await reader.readexactly(2)
        if resp[1] not in (0x00, 0x02):
            writer.close()
            return False

        # AUTH (si corresponde)
        if resp[1] == 0x02:
            writer.write(make_auth())
            await writer.drain()
            auth_resp = await reader.readexactly(2)
            if auth_resp[1] != 0x00:
                writer.close()
                return False

        # CONNECT
        writer.write(make_connect())
        await writer.drain()

        # Leer header
        conn_hdr = await reader.readexactly(4)
        if conn_hdr[1] != 0x00:   # REP != succeeded
            writer.close()
            return False

        atyp = conn_hdr[3]
        if atyp == 0x01:
            await reader.readexactly(4)
        elif atyp == 0x04:
            await reader.readexactly(16)
        elif atyp == 0x03:
            ln = await reader.readexactly(1)
            await reader.readexactly(ln[0])
        else:
            writer.close()
            return False
        
        await reader.readexactly(2)  # puerto

        # HTTP GET
        writer.write(make_http())
        await writer.drain()

        # Read full HTTP response
        body = b""
        while True:
            chunk = await reader.read(4096)
            if not chunk:
                break
            body += chunk

        writer.close()
        await writer.wait_closed()

        # Compare against reference
        return body == reference_body

    except:
        return False


async def get_reference():
    """
    Obtiene la respuesta correcta con 1 sola conexión.
    Esto permite comparar el contenido exacto en las 500 conexiones.
    """
    reader, writer = await asyncio.open_connection(PROXY_HOST, PROXY_PORT)

    # HELLO
    writer.write(make_hello())
    await writer.drain()
    resp = await reader.readexactly(2)

    # AUTH
    if resp[1] == 0x02:
        writer.write(make_auth())
        await writer.drain()
        await reader.readexactly(2)

    # CONNECT
    writer.write(make_connect())
    await writer.drain()

    # Leer bind addr
    hdr = await reader.readexactly(4)
    atyp = hdr[3]
    if atyp == 0x01:
        await reader.readexactly(4)
    elif atyp == 0x04:
        await reader.readexactly(16)
    else:
        ln = await reader.readexactly(1)
        await reader.readexactly(ln[0])
    await reader.readexactly(2)

    # HTTP
    writer.write(make_http())
    await writer.drain()

    body = b""
    while True:
        chunk = await reader.read(4096)
        if not chunk:
            break
        body += chunk

    writer.close()
    await writer.wait_closed()
    return body


async def main():
    print("Obteniendo respuesta de referencia...")
    reference = await get_reference()
    print("Referencia obtenida. Iniciando test concurrente...\n")

    tasks = [
        socks5_request(i, reference)
        for i in range(CONCURRENCY)
    ]

    results = await asyncio.gather(*tasks)

    # Reporte final
    ok = sum(results)
    fail = CONCURRENCY - ok

    print(f"TOTAL={CONCURRENCY}")
    print(f"OK={ok}")
    print(f"FAIL={fail}")

    if fail > 0:
        print("\n❌ ALGO FALLÓ EN LAS CONEXIONES\n")
    else:
        print("\n✔ TODAS LAS CONEXIONES CORRECTAS\n")


if __name__ == "__main__":
    asyncio.run(main())
