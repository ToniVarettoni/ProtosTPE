import asyncio
from aiohttp import ClientSession
from aiohttp_socks import ProxyConnector

TARGET_URL = "http://www.example.org"
PROXY = "socks5://username:password@localhost:1080"
CONCURRENCY = 200


async def fetch(session):
    try:
        async with session.get(TARGET_URL) as resp:
            return await resp.text()
    except Exception as e:
        return None


async def main():
    # --- 1. Obtener respuesta de referencia ---
    print("Obteniendo respuesta de referencia...")

    connector = ProxyConnector.from_url(PROXY)
    async with ClientSession(connector=connector) as session:
        reference = await fetch(session)

        if reference is None:
            print("ERROR: No se pudo obtener la respuesta de referencia. ¿El proxy está funcionando?")
            return

    print("Respuesta de referencia obtenida. Lanzando pruebas...\n")

    # --- 2. Correr las 500 requests concurrentes ---
    async def run_test(i):
        connector = ProxyConnector.from_url(PROXY)
        async with ClientSession(connector=connector) as session:
            response = await fetch(session)

            if response == reference:
                print(f"[{i:03d}] OK")
            else:
                print(f"[{i:03d}] FAIL")

    tasks = [run_test(i) for i in range(1, CONCURRENCY + 1)]
    await asyncio.gather(*tasks)


if __name__ == "__main__":
    asyncio.run(main())
