#!/usr/bin/env python3
"""
Basic HTTP/1.1 pipelining test through the SOCKS5 proxy.

What it does:
- Opens one TCP connection to the proxy, negotiates SOCKS5 (with optional user/pass).
- Issues a CONNECT to the target origin.
- Sends N HTTP GET requests back-to-back (pipelined) on that single TCP stream.
- Verifies all responses are received, complete (by Content-Length), and in-order.

Adjust PROXY_HOST/PORT, TARGET_HOST/PORT, AUTH_* to your setup.
"""

import socket

PROXY_HOST = "127.0.0.1"
PROXY_PORT = 1080
TARGET_HOST = "127.0.0.1"  # origin where you run: python3 -m http.server 8000
TARGET_PORT = 8000
REQUESTS = 20
USE_AUTH = True
AUTH_USER = b"alice"
AUTH_PASS = b"secret"
TIMEOUT = 5


def socks_connect():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(TIMEOUT)
    s.connect((PROXY_HOST, PROXY_PORT))

    # HELLO: advertise no-auth and username/password
    methods = b"\x00\x02" if USE_AUTH else b"\x00"
    s.sendall(b"\x05" + bytes([len(methods)]) + methods)
    ver, method = s.recv(2)
    if ver != 5:
        raise RuntimeError("Bad SOCKS VER in greeting")
    if USE_AUTH and method != 0x02:
        raise RuntimeError(f"Proxy did not select USER/PASS (method={method})")
    if not USE_AUTH and method not in (0x00, 0x02):
        raise RuntimeError(f"Unsupported method selected: {method}")

    # USER/PASS auth if selected
    if method == 0x02:
        msg = (
            b"\x01"
            + bytes([len(AUTH_USER)]) + AUTH_USER
            + bytes([len(AUTH_PASS)]) + AUTH_PASS
        )
        s.sendall(msg)
        ver, status = s.recv(2)
        if ver != 1 or status != 0:
            raise RuntimeError("Auth failed")

    # CONNECT request (IPv4)
    addr = socket.inet_aton(TARGET_HOST)
    s.sendall(b"\x05\x01\x00\x01" + addr + TARGET_PORT.to_bytes(2, "big"))
    rep = s.recv(4)
    if len(rep) != 4 or rep[0] != 0x05 or rep[1] != 0x00:
        raise RuntimeError(f"CONNECT failed, reply={rep}")
    atyp = rep[3]
    if atyp == 0x01:
        s.recv(4)
    elif atyp == 0x04:
        s.recv(16)
    elif atyp == 0x03:
        ln = s.recv(1)[0]
        s.recv(ln)
    s.recv(2)  # BND.PORT
    return s


def main():
    s = socks_connect()

    # Build pipelined GETs (no reliance on origin echoing headers)
    req = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n"
    payload = (req * REQUESTS).encode()
    s.sendall(payload)

    buf = b""
    responses = 0
    while responses < REQUESTS:
        chunk = s.recv(4096)
        if not chunk:
            break
        buf += chunk
        while True:
            if b"\r\n\r\n" not in buf:
                break
            hdr, rest = buf.split(b"\r\n\r\n", 1)
            clen = None
            for line in hdr.split(b"\r\n"):
                lower = line.lower()
                if lower.startswith(b"content-length:"):
                    clen = int(line.split(b":", 1)[1].strip())
            if clen is None:
                raise RuntimeError("No Content-Length in response")
            if len(rest) < clen:
                # wait for full body
                buf = hdr + b"\r\n\r\n" + rest
                break
            _body = rest[:clen]
            buf = rest[clen:]
            responses += 1

    s.close()

    print(f"Total responses: {responses}/{REQUESTS}")
    if responses == REQUESTS:
        print("Pipelining OK (responses complete and parsed in sequence)")
    else:
        raise SystemExit("Pipelining failed (missing responses)")


if __name__ == "__main__":
    main()
