# Developer Guide — Port Scanner Local

## Documentación para Desarrolladores

---

## Tabla de Contenidos

1. [Arquitectura General](#1-arquitectura-general)
2. [Estructura de Módulos](#2-estructura-de-módulos)
3. [Descripción Detallada de Módulos](#3-descripción-detallada-de-módulos)
   - [3.1 main.py](#31-mainpy)
   - [3.2 scanner.py](#32-scannerpy)
   - [3.3 banner.py](#33-bannerpy)
   - [3.4 services.py](#34-servicespy)
   - [3.5 output.py](#35-outputpy)
4. [API de Funciones](#4-api-de-funciones)
5. [Flujo de Datos](#5-flujo-de-datos)
6. [Guía de Modificación y Extensión](#6-guía-de-modificación-y-extensión)
7. [Pruebas](#7-pruebas)
8. [Contribución](#8-contribución)

---

## 1. Arquitectura General

El proyecto sigue una arquitectura **modular y orientada a tareas**, donde cada módulo tiene una responsabilidad única y bien definida:

```
┌─────────────────────────────────────────────────────────────┐
│                        main.py                               │
│  ● Configuración global                                     │
│  ● Gestión del entorno virtual                              │
│  ● Orquestación del flujo de escaneo                        │
└───────────────────────────┬─────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                      scanner.py                              │
│  ● Conexión TCP (socket)                                    │
│  ● Paralelización con ThreadPoolExecutor                    │
│  ● Coordinación entre services.py y banner.py               │
└──────┬──────────────────────────────────┬───────────────────┘
       │                                  │
       ▼                                  ▼
┌──────────────┐                  ┌──────────────┐
│ services.py  │                  │  banner.py    │
│ ● Lookup     │                  │ ● Banner      │
│   IANA       │                  │   grabbing    │
└──────────────┘                  └──────────────┘
       │                                  │
       └──────────────┬───────────────────┘
                      ▼
       ┌──────────────────────────┐
       │       output.py          │
       │ ● Formateo de resultados │
       │ ● rich / fallback plano  │
       └──────────────────────────┘
```

### Principios de diseño

- **Separación de responsabilidades**: Cada módulo aborda una capa específica del problema.
- **Resiliencia**: El sistema funciona con o sin la librería `rich` (fallback graceful).
- **Gestión automática del entorno**: El script se encarga de su propio virtualenv.
- **Parallelismo vía threads**: Uso de `concurrent.futures` para escaneo concurrente sin complejidad de async/await.
- **Manejo de errores granular**: Cada nivel captura y reporta errores sin colapsar el escaneo completo.

---

## 2. Estructura de Módulos

```
port_scanner/
├── main.py              # Punto de entrada, configuración, virtualenv
├── scanner.py           # Lógica de escaneo TCP con ThreadPoolExecutor
├── banner.py            # Captura de banners de servicios
├── services.py          # Diccionario IANA puerto → servicio
├── output.py            # Formateo y presentación de resultados
├── requirements.txt     # Dependencia: rich>=13.0.0
├── PLAN.md              # Plan de desarrollo y auditoría
├── AUDIT_REPORT.md      # Reporte de auditoría de código
├── README.md            # Documentación principal
└── docs/
    ├── datasheet.md     # Documentación de usuario
    └── developer.md     # Esta guía
```

### Dependencias entre módulos

| Módulo       | Importa desde              | Es importado por |
|--------------|----------------------------|------------------|
| `main.py`    | `scanner`, `output`        | — (punto de entrada) |
| `scanner.py` | `banner`, `services`       | `main.py`        |
| `banner.py`  | `socket` (estándar)        | `scanner.py`     |
| `services.py`| — (solo constantes)        | `scanner.py`     |
| `output.py`  | `rich` (opcional)          | `main.py`        |

---

## 3. Descripción Detallada de Módulos

### 3.1 `main.py`

**Propósito**: Punto de entrada del programa. Configura el entorno virtual y orquesta el escaneo.

**Responsabilidades**:
1. Definir constantes de configuración (`HOST`, `START_PORT`, `END_PORT`, `TIMEOUT`, `MAX_THREADS`).
2. Gestionar el entorno virtual (creación, instalación de dependencias, re-ejecución).
3. Orquestar el flujo de escaneo: mostrar encabezado, llamar a `scan_ports()`, mostrar resultados.

**Funciones internas**:

| Función               | Visibilidad | Descripción                                      |
|-----------------------|-------------|--------------------------------------------------|
| `_get_python()`       | Privada     | Retorna ruta al intérprete dentro del virtualenv |
| `_get_pip()`          | Privada     | Retorna ruta a pip dentro del virtualenv         |
| `_ensure_venv()`      | Privada     | Crea virtualenv si no existe, instala dependencias |
| `_run_in_venv()`      | Privada     | Re-ejecuta el script dentro del virtualenv       |
| `setup_environment()` | Pública     | Llama a `_ensure_venv()` y `_run_in_venv()`      |
| `main()`              | Pública     | Función principal: orquesta el escaneo completo  |

**Flujo de `main()`**:
1. Registrar tiempo de inicio (`time.time()`).
2. Imprimir encabezado con configuración.
3. Importar `scan_ports` de `scanner` y `print_results` de `output`.
4. Ejecutar `scan_ports()` con los parámetros configurados.
5. Calcular tiempo transcurrido.
6. Imprimir resultados con `print_results()`.
7. Capturar `KeyboardInterrupt` y excepciones generales.

### 3.2 `scanner.py`

**Propósito**: Contiene la lógica de conexión TCP y paralelización.

**Responsabilidades**:
1. Realizar conexiones TCP a puertos individuales.
2. Paralelizar el escaneo mediante `ThreadPoolExecutor`.
3. Coordinar la obtención del nombre del servicio (`services.py`) y el banner (`banner.py`).

**Funciones**:

| Función              | Descripción                                      |
|----------------------|--------------------------------------------------|
| `_tcp_connect()`     | Intenta conexión TCP; retorna `True` si exitosa  |
| `scan_port()`        | Escanea un puerto: conecta, obtiene servicio y banner |
| `scan_ports()`       | Escanea un rango de puertos en paralelo          |

**Detalle de `_tcp_connect()`**:
- Crea socket `AF_INET` / `SOCK_STREAM`.
- Configura timeout con `sock.settimeout(timeout)`.
- Usa `connect_ex()` para evitar excepciones en conexiones fallidas (retorna código de error).
- Retorna `True` si `result == 0` (conexión exitosa).
- Cierra el socket en `finally` para evitar fugas de recursos.

**Detalle de `scan_ports()`**:
- Crea un `ThreadPoolExecutor` con `max_workers=max_threads`.
- Envía todas las tareas con `executor.submit(scan_port, ...)`.
- Recoge resultados con `as_completed()` para procesamiento en orden de finalización.
- Filtra solo puertos abiertos (`result["open"] == True`).
- Ordena resultados por número de puerto antes de retornar.
- Errores individuales se registran en stderr sin detener el escaneo.

### 3.3 `banner.py`

**Propósito**: Captura banners de servicios para identificar versiones.

**Responsabilidades**:
1. Conectarse a un puerto abierto.
2. Enviar sondas genéricas para provocar respuesta del servicio.
3. Capturar y limpiar la respuesta del banner.
4. Retornar el banner como cadena legible.

**Constantes**:

| Constante           | Valor | Descripción                              |
|---------------------|-------|------------------------------------------|
| `BANNER_READ_SIZE`  | 1024  | Bytes máximos a leer de la respuesta     |
| `_PROBES`           | lista | Sondas genéricas para distintos protocolos |

**Sondas definidas**:

```python
_PROBES = [
    b"\r\n",                                   # Servicios banner-first
    b"\n",                                     # Alternativa
    b"HELP\r\n",                               # Servicios tipo comando
    b"GET / HTTP/1.0\r\nHost: localhost\r\n\r\n",  # Servidores HTTP
    b"QUIT\r\n",                               # Servicios SMTP/POP3
]
```

**Funciones**:

| Función             | Descripción                                      |
|---------------------|--------------------------------------------------|
| `_clean_banner()`   | Limpia datos crudos: elimina no imprimibles, decodifica UTF-8 |
| `grab_banner()`     | Intenta obtener banner del servicio en un puerto |

**Detalle de `grab_banner()`**:
1. Conecta socket TCP al host:port.
2. Itera sobre `_PROBES`: envía sonda, espera respuesta con `recv()`.
3. Si recibe datos, los limpia con `_clean_banner()` y retorna.
4. Si ninguna sonda funciona, intenta `recv()` sin enviar nada (servicios que envían banner al conectarse).
5. Retorna cadena vacía si no se pudo obtener banner.
6. Captura `socket.timeout`, `ConnectionRefusedError`, `OSError` y `socket.error`.

**Detalle de `_clean_banner()`**:
- Decodifica bytes como UTF-8 con `errors="replace"`.
- Reemplaza caracteres de reemplazo (`\ufffd`) y no imprimibles (excepto tab, newline, carriage return) por espacios.
- Retorna cadena limpia y recortada (`.strip()`).

### 3.4 `services.py`

**Propósito**: Proporciona el diccionario de puertos conocidos y la función de lookup.

**Responsabilidades**:
1. Mantener un diccionario `known_services` con mapeo puerto → nombre de servicio.
2. Proveer función `get_service_name()` para consultar el nombre de un puerto.

**Diccionario `known_services`**:
- Contiene los 1024 puertos del rango IANA bien conocidos (1-1024).
- Basado en la lista oficial de la Internet Assigned Numbers Authority.
- Incluye servicios como ssh (22), http (80), https (443), etc.
- Puertos no incluidos en el rango retornan `"desconocido"`.

**Función**:

| Función             | Descripción                                      |
|---------------------|--------------------------------------------------|
| `get_service_name()` | Retorna nombre del servicio para un puerto dado  |

**Comportamiento**:
```python
get_service_name(22)    # → "ssh"
get_service_name(80)    # → "http"
get_service_name(9999)  # → "desconocido"
```

### 3.5 `output.py`

**Propósito**: Formatea y presenta los resultados del escaneo en consola.

**Responsabilidades**:
1. Determinar si `rich` está disponible.
2. Renderizar tabla con resultados (colores si rich, plano si no).
3. Mostrar resumen con cantidad de puertos abiertos.

**Función principal**:

| Función             | Descripción                                      |
|---------------------|--------------------------------------------------|
| `print_results()`   | Imprime resultados del escaneo                   |

**Funciones internas**:

| Función             | Descripción                                      |
|---------------------|--------------------------------------------------|
| `_print_rich()`     | Renderiza tabla con rich (colores, formato)      |
| `_print_plain()`    | Renderiza tabla en texto plano (fallback)        |

**Detalle de `_print_rich()`**:
- Crea `rich.console.Console`.
- Construye `rich.table.Table` con título "Resultados del Escaneo de Puertos".
- Columnas: Puerto (cyan), Estado (bold), Servicio (green), Versión (yellow).
- Agrega fila por cada resultado.
- Imprime resumen al final.

**Detalle de `_print_plain()`**:
- Calcula anchos de columna según contenido.
- Imprime encabezados alineados.
- Separa con línea de guiones.
- Imprime filas alineadas.
- Imprime resumen al final.

**Mecanismo de fallback**:
```python
try:
    from rich.console import Console
    from rich.table import Table
    _RICH_AVAILABLE = True
except ImportError:
    _RICH_AVAILABLE = False
```

---

## 4. API de Funciones

### 4.1 `scanner.py`

#### `scan_ports(host, start_port, end_port, timeout, max_threads)`

```python
def scan_ports(
    host: str = "127.0.0.1",
    start_port: int = 1,
    end_port: int = 1024,
    timeout: float = 2.0,
    max_threads: int = 50,
) -> list[dict[str, Any]]:
```

**Parámetros**:

| Parámetro     | Tipo    | Default     | Descripción                              |
|---------------|---------|-------------|------------------------------------------|
| `host`        | `str`   | `"127.0.0.1"` | Dirección IP o hostname del objetivo    |
| `start_port`  | `int`   | `1`         | Primer puerto del rango                  |
| `end_port`    | `int`   | `1024`      | Último puerto del rango (inclusive)      |
| `timeout`     | `float` | `2.0`       | Timeout en segundos para cada conexión   |
| `max_threads` | `int`   | `50`        | Número máximo de hilos concurrentes      |

**Retorno**: Lista de diccionarios, uno por cada puerto abierto:

```python
{
    "port": int,      # Número de puerto
    "open": bool,     # True (siempre, solo puertos abiertos)
    "service": str,   # Nombre del servicio ("ssh", "http", ...)
    "version": str,   # Banner capturado ("SSH-2.0-...") o ""
}
```

**Excepciones**: No lanza excepciones directamente; errores individuales se registran en stderr.

---

#### `scan_port(host, port, timeout)`

```python
def scan_port(
    host: str,
    port: int,
    timeout: float = 2.0,
) -> dict[str, Any]:
```

**Parámetros**:

| Parámetro | Tipo    | Default | Descripción                          |
|-----------|---------|---------|--------------------------------------|
| `host`    | `str`   | —       | Dirección IP o hostname del objetivo |
| `port`    | `int`   | —       | Número de puerto a escanear          |
| `timeout` | `float` | `2.0`   | Timeout en segundos para la conexión |

**Retorno**: Diccionario con `port`, `open`, `service`, `version`.

---

### 4.2 `banner.py`

#### `grab_banner(host, port, timeout)`

```python
def grab_banner(
    host: str,
    port: int,
    timeout: float = 2.0,
) -> str:
```

**Parámetros**:

| Parámetro | Tipo    | Default | Descripción                              |
|-----------|---------|---------|------------------------------------------|
| `host`    | `str`   | —       | Dirección IP o hostname del objetivo     |
| `port`    | `int`   | —       | Número de puerto a sondear               |
| `timeout` | `float` | `2.0`   | Timeout en segundos para la operación    |

**Retorno**: Cadena con el banner obtenido, o cadena vacía (`""`) si no se pudo obtener.

---

### 4.3 `services.py`

#### `get_service_name(port)`

```python
def get_service_name(port: int) -> str:
```

**Parámetros**:

| Parámetro | Tipo  | Default | Descripción                      |
|-----------|-------|---------|----------------------------------|
| `port`    | `int` | —       | Número de puerto (1-1024)        |

**Retorno**: Nombre del servicio (`str`), o `"desconocido"` si el puerto no está en el mapa.

---

### 4.4 `output.py`

#### `print_results(results)`

```python
def print_results(results: list[dict[str, Any]]) -> None:
```

**Parámetros**:

| Parámetro | Tipo                   | Descripción                                      |
|-----------|------------------------|--------------------------------------------------|
| `results` | `list[dict[str, Any]]` | Lista de diccionarios con resultados del escaneo |

**Retorno**: `None`. Imprime directamente en consola.

---

### 4.5 `main.py`

#### `setup_environment()`

```python
def setup_environment() -> None:
```

Configura el entorno virtual y re-ejecuta el script dentro de él. No retorna valor.

#### `main()`

```python
def main() -> None:
```

Punto de entrada principal. Orquesta el escaneo completo y la presentación de resultados.

---

## 5. Flujo de Datos

```
main.py                                      scanner.py
─────────────────                            ─────────────────
start_time = time.time()                     
print(header)                            ──► scan_ports()
                                                │
                                          ports = range(1, 1025)
                                                │
                                          ThreadPoolExecutor(50)
                                                │
                                          for port in ports:
                                              executor.submit(scan_port)
                                                    │
                                                    ▼
                                          scan_port(host, port, timeout)
                                                │
                                          ┌──────┴──────┐
                                          │             │
                                          ▼             ▼
                                    _tcp_connect    services.get_service_name
                                    (host,port,     (port) → "ssh"
                                     timeout)
                                          │
                                          ▼
                                    ¿Conexión       banner.grab_banner(host,
                                     exitosa?        port, timeout) → versión
                                          │
                                          ▼
                                    Retorna dict:
                                    {"port":22,
                                     "open":True,
                                     "service":"ssh",
                                     "version":"SSH-2.0..."}
                                          │
                                    as_completed() recoge resultados
                                          │
                                    Filtra open==True
                                          │
                                    Ordena por port
                                          │
◄─────────────────────────────────────────┘
                                          │
output.py                                 │
─────────────────                          │
print_results(results) ◄───────────────────┘
      │
      ├── _print_rich(results)   (si rich disponible)
      │   └── Table + console.print
      │
      └── _print_plain(results)  (fallback)
          └── Formato texto plano

elapsed = time.time() - start_time
print(f"Tiempo total: {elapsed:.2f} segundos")
```

---

## 6. Guía de Modificación y Extensión

### 6.1 Agregar soporte para escaneo UDP

Actualmente el escáner solo soporta TCP. Para agregar UDP:

1. En `scanner.py`, crear `_udp_connect()`:

```python
def _udp_connect(host: str, port: int, timeout: float) -> bool:
    """Intenta enviar un paquete UDP al puerto especificado."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(timeout)
    try:
        sock.sendto(b"", (host, port))
        sock.recvfrom(1024)
        return True
    except (socket.timeout, socket.error, OSError):
        return False
    finally:
        sock.close()
```

2. Agregar un parámetro `protocol="tcp"` a `scan_port()`.

3. En `main.py`, agregar constante `PROTOCOL`.

### 6.2 Agregar soporte para escaneo de IPs remotas

Ya es posible cambiando `HOST` en `main.py`. Para hacerlo más seguro:

1. Agregar validación de IP en `main()`:

```python
import ipaddress
try:
    ipaddress.ip_address(HOST)
except ValueError:
    print(f"[!] Dirección IP inválida: {HOST}")
    sys.exit(1)
```

2. Agregar advertencia si no es loopback:

```python
if HOST != "127.0.0.1":
    print("[!] ATENCIÓN: Escaneando un host remoto. Asegúrate de tener permiso.")
```

### 6.3 Exportar resultados a JSON

Agregar función en `output.py`:

```python
def export_json(results: list[dict[str, Any]], filepath: str) -> None:
    """Exporta resultados a un archivo JSON."""
    import json
    with open(filepath, "w") as f:
        json.dump(results, f, indent=2)
```

Llamar desde `main.py`:

```python
export_json(results, "scan_results.json")
```

### 6.4 Agregar líneas de progreso durante el escaneo

Para mostrar progreso, modificar `scan_ports()`:

```python
total = end_port - start_port + 1
processed = 0

for future in as_completed(future_to_port):
    processed += 1
    if processed % 100 == 0:
        print(f"[*] Progreso: {processed}/{total} puertos escaneados", end="\r")
    # ... resto del código
```

### 6.5 Personalizar las sondas de banner

Edita `_PROBES` en `banner.py` para agregar sondas específicas:

```python
_PROBES = [
    b"\r\n",
    b"\n",
    b"HELP\r\n",
    b"GET / HTTP/1.0\r\nHost: localhost\r\n\r\n",
    b"QUIT\r\n",
    b"EHLO localhost\r\n",       # SMTP
    b"USER anonymous\r\n",       # FTP
]
```

### 6.6 Agregar filtro de puertos por servicio

En `main.py`, agregar:

```python
FILTER_SERVICE: str | None = None  # Ej: "ssh", "http"

# En main(), después de obtener results:
if FILTER_SERVICE:
    results = [r for r in results if r["service"] == FILTER_SERVICE]
```

---

## 7. Pruebas

### 7.1 Pruebas manuales

```bash
# Prueba básica
python main.py

# Prueba con puerto específico abierto (iniciar servicio)
sudo systemctl start ssh
python main.py
# Debe mostrar puerto 22 como abierto

# Prueba de interrupción
python main.py
# Presionar Ctrl+C durante el escaneo
# Debe mostrar: "[!] Escaneo interrumpido por el usuario."

# Prueba sin rich
pip uninstall rich
python main.py
# Debe funcionar con tabla de texto plano
```

### 7.2 Casos de prueba (de PLAN.md)

| Caso | Descripción | Resultado Esperado |
|------|-------------|-------------------|
| 1 | Puerto abierto conocido (SSH 22) | Puerto listado como "Abierto", servicio "ssh", banner con versión |
| 2 | Puerto cerrado | No aparece en la lista |
| 3 | Puerto filtrado (firewall) | No aparece (timeout) |
| 4 | Sin servicios activos | Mensaje "No se encontraron puertos abiertos" |
| 5 | Banner con datos | Versión extraída correctamente del banner |

### 7.3 Pruebas unitarias (futuro)

Se recomienda implementar pruebas con `unittest`:

```python
# tests/test_scanner.py
import unittest
from scanner import scan_port, _tcp_connect

class TestScanner(unittest.TestCase):
    def test_tcp_connect_open_port(self):
        # Asumiendo que SSH está corriendo en puerto 22
        result = _tcp_connect("127.0.0.1", 22, 2.0)
        self.assertTrue(result)

    def test_tcp_connect_closed_port(self):
        result = _tcp_connect("127.0.0.1", 9999, 1.0)
        self.assertFalse(result)

    def test_get_service_name(self):
        from services import get_service_name
        self.assertEqual(get_service_name(22), "ssh")
        self.assertEqual(get_service_name(80), "http")
        self.assertEqual(get_service_name(9999), "desconocido")
```

---

## 8. Contribución

### 8.1 Estilo de código

- Seguir **PEP 8** para estilo de código Python.
- Usar **type hints** en todas las funciones (ya implementado).
- Documentar funciones con **docstrings** estilo Google (ya implementado).
- Mantener líneas de máximo 100 caracteres.

### 8.2 Estructura de commits

Formato recomendado:

```
tipo(ámbito): descripción breve

Cuerpo opcional con detalles.
```

Tipos: `feat`, `fix`, `docs`, `refactor`, `test`, `chore`.

### 8.3 Proceso de contribución

1. Fork del repositorio.
2. Crear rama: `git checkout -b feat/mi-mejora`.
3. Realizar cambios siguiendo el estilo del proyecto.
4. Ejecutar pruebas manuales.
5. Commit y push.
6. Crear Pull Request describiendo los cambios.

---

## Apéndice A: Historial de Correcciones (Post-Auditoría)

### I-001: Excepciones en hilos silenciadas

**Problema**: En `scanner.py`, el bloque `except Exception: pass` silenciaba errores en hilos individuales.
**Corrección**: Reemplazado por:

```python
except Exception as exc:
    print(f"[!] Error en escaneo de puerto: {exc}", file=sys.stderr)
```

### I-002: Constantes duplicadas

**Problema**: Las constantes `HOST`, `START_PORT`, `END_PORT`, `TIMEOUT`, `MAX_THREADS` estaban duplicadas en `scanner.py`.
**Corrección**: Eliminadas de `scanner.py`. Ahora solo viven en `main.py` y se pasan como argumentos.

### I-003: Constante `BANNER_TIMEOUT` no utilizada

**Problema**: `BANNER_TIMEOUT = 3.0` en `banner.py` no se usaba.
**Corrección**: Eliminada. `grab_banner()` usa `timeout: float = 2.0` como parámetro por defecto.
