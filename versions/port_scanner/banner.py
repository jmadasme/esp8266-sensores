"""
Banner grabbing para detectar versiones de servicios.

Tras una conexión TCP exitosa, envía una petición genérica
y captura la respuesta del banner para identificar la versión.
"""

import socket
from typing import Optional

# Constantes
BANNER_READ_SIZE: int = 1024

# Probes genéricas para diferentes servicios
_PROBES: list[bytes] = [
    b"\r\n",
    b"\n",
    b"HELP\r\n",
    b"GET / HTTP/1.0\r\nHost: localhost\r\n\r\n",
    b"QUIT\r\n",
]

# Caracteres no imprimibles que se limpiarán del banner
_NON_PRINTABLE = bytes(
    [c for c in range(256) if c not in range(32, 127) and c not in (9, 10, 13)]
)


def _clean_banner(data: bytes) -> str:
    """Limpia el banner eliminando caracteres no imprimibles.

    Args:
        data: Datos crudos del banner.

    Returns:
        Cadena limpia con caracteres imprimibles.
    """
    # Reemplazar caracteres nulos y no imprimibles por espacios
    result = data.decode("utf-8", errors="replace")
    # Reemplazar caracteres de reemplazo y no imprimibles
    cleaned = []
    for ch in result:
        if ch == "\ufffd" or (ord(ch) < 32 and ch not in ("\t", "\n", "\r")):
            cleaned.append(" ")
        else:
            cleaned.append(ch)
    return "".join(cleaned).strip()


def grab_banner(host: str, port: int, timeout: float = 2.0) -> str:
    """Intenta obtener el banner de un servicio en un puerto específico.

    Args:
        host: Dirección IP o hostname del objetivo.
        port: Número de puerto a sondear.
        timeout: Timeout en segundos para la operación.

    Returns:
        Cadena con el banner obtenido, o cadena vacía si no se pudo obtener.
    """
    sock: Optional[socket.socket] = None
    try:
        # Crear socket TCP
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(timeout)
        sock.connect((host, port))

        # Intentar distintas probes para obtener banner
        for probe in _PROBES:
            try:
                sock.send(probe)
            except (socket.error, OSError):
                continue

            try:
                response = sock.recv(BANNER_READ_SIZE)
                if response:
                    cleaned = _clean_banner(response)
                    if cleaned:
                        return cleaned
            except (socket.timeout, socket.error, OSError):
                continue

        # Si ninguna probe funcionó, intentar recibir sin enviar nada
        try:
            response = sock.recv(BANNER_READ_SIZE)
            if response:
                return _clean_banner(response)
        except (socket.timeout, socket.error, OSError):
            pass

        return ""

    except (socket.timeout, ConnectionRefusedError, OSError, socket.error):
        return ""
    finally:
        if sock is not None:
            try:
                sock.close()
            except OSError:
                pass
