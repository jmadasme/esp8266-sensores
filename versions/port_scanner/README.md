# Port Scanner Local

<p align="center">
  <strong>Escáner de puertos TCP local — detección de servicios y banner grabbing</strong>
  <br>
  Python 3.10+ | Multi-threading | Sin dependencias obligatorias
</p>

---

## Tabla de Contenidos

- [Descripción](#descripción)
- [Principio de Funcionamiento](#principio-de-funcionamiento)
- [Requisitos del Sistema](#requisitos-del-sistema)
- [Instalación y Ejecución](#instalación-y-ejecución)
- [Forma de Operar](#forma-de-operar)
- [Configuración (Pinout)](#configuración-pinout)
- [Ejemplos de Uso](#ejemplos-de-uso)
- [Diagrama de Arquitectura](#diagrama-de-arquitectura)
- [Calibración y Ajuste](#calibración-y-ajuste)
- [Advertencias y Precauciones](#advertencias-y-precauciones)
- [Estructura del Proyecto](#estructura-del-proyecto)
- [Licencia](#licencia)

---

## Descripción

**Port Scanner Local** es una herramienta de diagnóstico de red escrita en Python que escanea los puertos TCP del 1 al 1024 en la interfaz de loopback (`127.0.0.1`) del equipo local. Para cada puerto abierto, identifica el servicio asociado (basado en el registro IANA) y realiza **banner grabbing** para obtener la versión del software que está escuchando.

Está diseñado para ser autónomo: crea y gestiona su propio entorno virtual, instala dependencias automáticamente, y presenta los resultados en una tabla formateada con o sin la librería `rich`.

---

## Principio de Funcionamiento

### Base TCP/IP

El escáner opera sobre el protocolo **TCP (Transmission Control Protocol)**, que es orientado a conexión. Para determinar si un puerto está abierto, el programa intenta establecer una conexión TCP completa mediante el **three-way handshake**:

1.  **SYN** — El cliente (scanner) envía un paquete SYN al puerto destino.
2.  **SYN-ACK** — Si el puerto está abierto, el servidor responde con SYN-ACK.
3.  **ACK** — El cliente completa el handshake y la conexión se establece.

Si el puerto está cerrado, el servidor responde con un paquete **RST** (reset). Si no hay respuesta (timeout), se asume que el puerto está filtrado por un firewall o que no hay equipo reachable.

### Banner Grabbing

Una vez establecida la conexión, el módulo de banner grabbing envía una serie de sondas genéricas (`\r\n`, `HELP\r\n`, `GET / HTTP/1.0...`, etc.) y captura hasta 1024 bytes de respuesta. Este banner suele contener el nombre y la versión del servicio (ej: `SSH-2.0-OpenSSH_9.2p1`).

### Identificación de Servicios

El proyecto incluye un diccionario completo de los puertos 1–1024 según la asignación **IANA** (Internet Assigned Numbers Authority), permitiendo traducir números de puerto a nombres de servicio legibles.

---

## Requisitos del Sistema

| Requisito            | Versión Mínima |
|----------------------|----------------|
| Python               | 3.10           |
| Sistema Operativo    | Linux, macOS, Windows |
| Conexión de Red      | Loopback (127.0.0.1) |
| Espacio en Disco     | ~5 MB (incluyendo virtualenv) |

### Dependencias

- **`rich`** (≥13.0.0) — Opcional. Proporciona tablas con colores y formato mejorado. Si no está disponible, el programa usa un formato de texto plano como fallback.
- `socket` — Biblioteca estándar de Python (incluida).
- `concurrent.futures` — Biblioteca estándar de Python (incluida).

---

## Instalación y Ejecución

### Método rápido (recomendado)

```bash
git clone <repositorio> port_scanner
cd port_scanner
python main.py
```

El script **crea automáticamente** un entorno virtual (`.venv/`), instala las dependencias y se re-ejecuta dentro de él. No requiere intervención manual.

### Instalación manual paso a paso

```bash
# 1. Crear entorno virtual
python3 -m venv .venv

# 2. Activar el entorno
source .venv/bin/activate       # Linux/macOS
# .venv\Scripts\activate        # Windows

# 3. Instalar dependencias
pip install -r requirements.txt

# 4. Ejecutar
python main.py
```

### Verificar instalación

```bash
python main.py --help   # Si se implementa: muestra ayuda
# Sin --help: ejecuta directamente el escaneo
```

---

## Forma de Operar

1.  Abre una terminal en el directorio del proyecto.
2.  Ejecuta `python main.py`.
3.  El script muestra un encabezado con la configuración actual (host, rango de puertos, timeout, threads).
4.  El escaneo comienza automáticamente y puede tomar entre 10 y 30 segundos para los 1024 puertos.
5.  Al finalizar, se imprime una tabla con los resultados: solo los puertos **abiertos** aparecen listados.
6.  Si no hay puertos abiertos, se muestra el mensaje `No se encontraron puertos abiertos.`
7.  Presiona `Ctrl+C` en cualquier momento para interrumpir el escaneo.

### Salida típica

```
============================================================
  PORT SCANNER LOCAL
============================================================
  Host:    127.0.0.1
  Puertos: 1-1024
  Timeout: 2.0s
  Threads: 50
============================================================

[*] Escaneando 127.0.0.1 (puertos 1-1024)...
[*] Esto puede tomar unos segundos...

┌────────┬────────┬──────────┬──────────────────────────────┐
│ Puerto │ Estado │ Servicio │ Versión                      │
├────────┼────────┼──────────┼──────────────────────────────┤
│     22 │ Abierto│ ssh      │ SSH-2.0-OpenSSH_9.2p1        │
│     80 │ Abierto│ http     │ HTTP/1.1 200 OK              │
│    631 │ Abierto│ ipp      │ CUPS/2.4.6                   │
└────────┴────────┴──────────┴──────────────────────────────┘

  Tiempo total: 12.34 segundos
```

---

## Configuración (Pinout)

Toda la configuración del escáner se define mediante constantes en `main.py`. Puedes modificarlas directamente para adaptar el comportamiento.

| Parámetro       | Constante       | Valor por Defecto | Descripción |
|-----------------|-----------------|-------------------|-------------|
| **Host**        | `HOST`          | `"127.0.0.1"`     | Dirección IP o hostname a escanear. |
| **Puerto inicial** | `START_PORT`  | `1`               | Primer puerto del rango a escanear. |
| **Puerto final**   | `END_PORT`    | `1024`            | Último puerto del rango (inclusive). |
| **Timeout**     | `TIMEOUT`       | `2.0`             | Tiempo máximo (segundos) para esperar una conexión TCP. |
| **Threads**     | `MAX_THREADS`   | `50`              | Número máximo de hilos concurrentes. |
| **Tamaño banner** | `BANNER_READ_SIZE` (en `banner.py`) | `1024` | Bytes máximos a leer del banner del servicio. |

> **Nota**: `BANNER_READ_SIZE` se define en `banner.py` y no en `main.py`.

### Ejemplo de modificación

Para escanear un rango personalizado en una IP remota:

```python
# En main.py
HOST = "192.168.1.1"
START_PORT = 1
END_PORT = 1000
TIMEOUT = 3.0
MAX_THREADS = 100
```

---

## Ejemplos de Uso

### Ejemplo 1: Escaneo básico local

```bash
$ python main.py
```

Escanea los puertos 1–1024 en localhost. Resultado típico en un equipo con SSH y CUPS activos:

```
Puerto | Estado  | Servicio | Versión
------ + -------- + -------- + -----------------------------
22     | Abierto | ssh      | SSH-2.0-OpenSSH_9.2p1
631    | Abierto | ipp      | CUPS/2.4.6
```

### Ejemplo 2: Escaneo rápido (solo puertos conocidos)

Modifica `main.py` para escanear solo los puertos más comunes:

```python
START_PORT = 1
END_PORT = 100
```

Luego ejecuta `python main.py`. El escaneo será mucho más rápido (~2 segundos).

### Ejemplo 3: Escaneo con más threads (mayor velocidad)

```python
MAX_THREADS = 200  # Usa 200 hilos concurrentes
```

Advertencia: aumentar los threads puede saturar la red o el sistema en equipos con recursos limitados.

### Ejemplo 4: Puerto sin servicio conocido

Si un puerto abierto no está en el diccionario IANA, aparecerá como `desconocido`:

```
Puerto | Estado  | Servicio     | Versión
------ + -------- + ------------ + -----------------
8080   | Abierto | desconocido  | Apache/2.4.41
```

### Ejemplo 5: Sin puertos abiertos

```
No se encontraron puertos abiertos.
```

---

## Diagrama de Arquitectura

```
                     ┌─────────────────────────────────┐
                     │          main.py                 │
                     │  ● Configuración (HOST, puertos, │
                     │    timeout, threads)              │
                     │  ● Gestión de virtualenv          │
                     │  ● Orquestación del escaneo       │
                     └──────────┬──────────────────────┘
                                │
              Genera lista de puertos (1-1024)
                                │
                                ▼
              ┌─────────────────────────────────┐
              │       scanner.py                 │
              │  ┌───────────────────────────┐   │
              │  │  ThreadPoolExecutor        │   │
              │  │  (max_workers=50)          │   │
              │  └───────────────────────────┘   │
              │         │         │         │    │
              │         ▼         ▼         ▼    │
              │  ┌───────┐ ┌───────┐ ┌───────┐  │
              │  │ hilo1 │ │ hilo2 │ │ hiloN │  │
              │  └───┬───┘ └───┬───┘ └───┬───┘  │
              │      │         │         │       │
              │      ▼         ▼         ▼       │
              │  scan_port(host, port, timeout)  │
              └──────────┬──────────────────────┘
                         │
                    ┌────┴────┐
                    │         │
                    ▼         ▼
         ┌──────────────┐  ┌──────────────┐
         │  services.py  │  │  banner.py    │
         │  get_service  │  │  grab_banner  │
         │  _name(port)  │  │  (host,port,  │
         │               │  │   timeout)    │
         └──────────────┘  └──────────────┘
                    │         │
                    └────┬────┘
                         │
                         ▼
              ┌─────────────────────────────────┐
              │        output.py                 │
              │  print_results(results)          │
              │  ● Con rich → tabla coloreada    │
              │  ● Sin rich → texto plano        │
              └─────────────────────────────────┘
                         │
                         ▼
                    ┌──────────┐
                    │ Consola  │
                    │ (stdout) │
                    └──────────┘

Flujo de datos:
  1. main.py genera lista de puertos (1-1024)
  2. scanner.py escanea en paralelo con ThreadPoolExecutor
  3. Por cada puerto → services.py lookup + banner.py grab
  4. output.py formatea y muestra resultados
```

---

## Calibración y Ajuste

### Ajuste del Timeout

El timeout determina cuánto tiempo esperar por cada conexión TCP. Valores recomendados:

| Valor   | Uso Recomendado                           |
|---------|-------------------------------------------|
| 0.5s    | Red local / loopback de alta velocidad     |
| 1.0s    | Red local estándar                         |
| 2.0s    | **Valor por defecto** — balance velocidad/confiabilidad |
| 5.0s    | Redes lentas o conexiones remotas          |

### Ajuste del Número de Threads

| Threads | Efecto                                    |
|---------|-------------------------------------------|
| 10      | Bajo uso de CPU, escaneo lento (~2 min)   |
| 50      | **Valor por defecto** — buen balance      |
| 100     | Escaneo rápido (~10-15s)                  |
| 200     | Muy rápido (~5-8s), puede saturar el sistema |

### Ajuste del Rango de Puertos

Para escaneos específicos (ej: solo puertos web):

```python
START_PORT = 80
END_PORT = 443
```

Esto reduce drásticamente el tiempo de escaneo.

### Calibración del Banner Grabbing

Si el banner grabbing no funciona correctamente, puedes aumentar el timeout de lectura o modificar las sondas en `banner.py`:

```python
# En banner.py
_PROBES = [
    b"\r\n",
    b"\n",
    b"HELP\r\n",
    b"GET / HTTP/1.0\r\nHost: localhost\r\n\r\n",
    b"QUIT\r\n",
]
```

---

## Advertencias y Precauciones

### ⚠️ Legal y Ético

- **Uso responsable**: Esta herramienta está diseñada exclusivamente para escanear el **propio equipo** (loopback). Escanear equipos sin autorización explícita puede ser ilegal en muchas jurisdicciones.
- **Privacidad**: No almacena ni transmite datos fuera del equipo local.
- **Entorno controlado**: Úsala solo en redes y dispositivos sobre los que tengas control.

### ⚠️ Técnico

- **Cortafuegos**: Los firewalls pueden interferir con el escaneo, mostrando falsos negativos (puertos abiertos que no responden) o falsos positivos.
- **Rendimiento**: Aumentar `MAX_THREADS` por encima de 200 puede degradar el rendimiento del sistema o causar agotamiento de recursos (archivos de socket).
- **Servicios sensibles**: Algunos servicios pueden registrar el intento de conexión como un evento de seguridad. Usar con precaución en sistemas de producción.
- **Detección**: Soluciones de seguridad (IDS/IPS) pueden detectar el escaneo como actividad maliciosa incluso en loopback.
- **Banners incompletos**: No todos los servicios envían banner automáticamente; algunos requieren autenticación o una solicitud específica.

### 🛡️ Buenas Prácticas

1. No ejecutar el escáner en equipos que no sean de tu propiedad.
2. No usarlo para vulnerar sistemas o redes sin permiso por escrito.
3. En entornos de producción, programar escaneos en ventanas de mantenimiento.
4. Verificar que los puertos reportados como abiertos sean realmente servicios intencionales.

---

## Estructura del Proyecto

```
port_scanner/
├── main.py              # Punto de entrada y gestión del virtualenv
├── scanner.py           # Lógica de escaneo TCP con ThreadPoolExecutor
├── banner.py            # Banner grabbing para detección de versiones
├── services.py          # Mapa IANA de puertos → nombres de servicio
├── output.py            # Formateo de resultados en consola
├── requirements.txt     # Dependencias (rich)
├── README.md            # Este archivo
├── PLAN.md              # Plan de desarrollo y auditoría
├── AUDIT_REPORT.md      # Reporte de auditoría de código (si aplica)
├── docs/
│   ├── datasheet.md     # Documentación de usuario detallada
│   └── developer.md     # Documentación para desarrolladores
└── .venv/               # Entorno virtual (creado automáticamente)
```

---

## Licencia

Este proyecto se proporciona con fines educativos y de diagnóstico de red. No se concede ninguna garantía explícita o implícita. El uso es bajo la responsabilidad del usuario final.
