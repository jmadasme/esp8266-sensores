"""
Lógica de escaneo de puertos.

Realiza conexiones TCP paralelizadas para detectar puertos abiertos
y obtener información del servicio y versión.
"""

import socket
from concurrent.futures import ThreadPoolExecutor, as_completed
from typing import Any

from banner import grab_banner
from services import get_service_name


def _tcp_connect(host: str, port: int, timeout: float) -> bool:
    """Intenta una conexión TCP al puerto especificado.

    Args:
        host: Dirección IP o hostname del objetivo.
        port: Número de puerto a probar.
        timeout: Timeout en segundos para la conexión.

    Returns:
        True si la conexión fue exitosa, False en caso contrario.
    """
    sock: socket.socket | None = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        result = sock.connect_ex((host, port))
        return result == 0
    except (socket.error, OSError):
        return False
    finally:
        if sock is not None:
            try:
                sock.close()
            except OSError:
                pass


def scan_port(host: str, port: int, timeout: float = 2.0) -> dict[str, Any]:
    """Escanea un puerto específico y retorna un diccionario con el resultado.

    Args:
        host: Dirección IP o hostname del objetivo.
        port: Número de puerto a escanear.
        timeout: Timeout en segundos para la conexión.

    Returns:
        Diccionario con:
            - port: Número de puerto.
            - open: True si está abierto, False en caso contrario.
            - service: Nombre del servicio conocido.
            - version: Banner de versión obtenido (cadena vacía si no disponible).
    """
    is_open = _tcp_connect(host, port, timeout)
    service = get_service_name(port)
    version = ""

    if is_open:
        # Intentar banner grabbing para obtener la versión
        version = grab_banner(host, port, timeout)

    return {
        "port": port,
        "open": is_open,
        "service": service,
        "version": version,
    }


def scan_ports(
    host: str = "127.0.0.1",
    start_port: int = 1,
    end_port: int = 1024,
    timeout: float = 2.0,
    max_threads: int = 50,
) -> list[dict[str, Any]]:
    """Escanea un rango de puertos en paralelo usando un pool de threads.

    Args:
        host: Dirección IP o hostname del objetivo.
        start_port: Primer puerto del rango a escanear.
        end_port: Último puerto del rango a escanear (inclusive).
        timeout: Timeout en segundos para cada conexión.
        max_threads: Número máximo de threads concurrentes.

    Returns:
        Lista de diccionarios con los resultados de los puertos abiertos.
    """
    ports = range(start_port, end_port + 1)
    open_results: list[dict[str, Any]] = []

    with ThreadPoolExecutor(max_workers=max_threads) as executor:
        # Enviar todas las tareas
        future_to_port = {
            executor.submit(scan_port, host, port, timeout): port
            for port in ports
        }

        # Recoger resultados a medida que se completan
        for future in as_completed(future_to_port):
            try:
                result = future.result()
                if result["open"]:
                    open_results.append(result)
            except Exception as exc:
                # Registrar errores de tareas individuales
                import sys
                print(f"[!] Error en escaneo de puerto: {exc}", file=sys.stderr)

    # Ordenar resultados por número de puerto
    open_results.sort(key=lambda r: r["port"])

    return open_results
