# TPE Protocolos de Comunicación

## 1. Estructura de Archivos

La ubicación de los materiales del proyecto es la siguiente:

* **Informe:** `/docs`
* **Código fuente:** `/src`
* **Makefiles:** El principal en `/`, uno en `/src/client` y otro en `/src/server`.

## 2. Construcción

Para generar los ejecutables del proyecto, sitúese en la raíz y ejecute:

```bash
make clean
make all
```

## 3. Ubicación de Artefactos

Una vez finalizada la compilación, los ejecutables se encontrarán en:

```
./bin/
```

Los archivos generados son:

* `server`
* `client`

## 4. Ejecución y Opciones

Los ejecutables del proyecto se generan en el directorio `bin/`. Asegúrese de estar en la raíz del proyecto antes de ejecutarlos.

### 4.1 Servidor (`server`)

El servidor maneja las conexiones y el tráfico.

#### Comando básico

```bash
./bin/server [OPTIONS]
```

#### Opciones disponibles

```bash
-h                  Imprime la ayuda y termina la ejecución.
-l <SOCKS addr>     Dirección IP donde se servirá el proxy SOCKS.
-p <SOCKS port>     Puerto TCP para conexiones entrantes SOCKS.
-L <MNG addr>       Dirección IP para el servicio de gestión.
-P <MNG port>       Puerto UDP para las conexiones del manager.
-u <usr>:<pass>     Usuario y contraseña iniciales para autenticación.
-v                  Imprime información sobre la versión del servidor.
```

#### Ejemplo de ejecución

```bash
./bin/server -p 8080 -P 8081 -u admin:admin123
```

### 4.2 Cliente de Gestión (`client`)

La herramienta de monitoreo permite realizar cambios en tiempo de ejecución (como agregar usuarios) y consultar métricas sin detener el servidor.

#### Comando básico

```bash
./bin/client <host:port> -l <admin:pass> [ACCIÓN]
```

#### Requisitos

```bash
<host:port>         Dirección y puerto del servicio de gestión (coinciden con -L y -P del servidor). Debe ser el primer parámetro.
-l <user:pass>      Credenciales de administrador. Obligatorio para cualquier operación.
```

#### Acciones disponibles

```bash
-a <user:pass>      Da de alta un usuario con privilegios estándar.
-A <user:pass>      Da de alta un usuario con privilegios de administrador.
-d <user>           Elimina un usuario existente.
-c <user:newpass>   Modifica la contraseña de un usuario.
-s                  Muestra estadísticas del servidor (bytes, conexiones, etc.).
-h                  Muestra la ayuda.
```

#### Ejemplo - Ver estadísticas

```bash
./bin/client 127.0.0.1:8081 -l admin:admin123 -s
```

#### Ejemplo - Agregar un usuario nuevo

```bash
./bin/client 127.0.0.1:8081 -l admin:admin123 -a kaladin:stormblessed
```