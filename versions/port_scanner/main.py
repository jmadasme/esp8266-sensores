"""
Port Scanner Local - Punto de entrada principal.

Escanea los puertos TCP 1-1024 en localhost (127.0.0.1) y muestra
una tabla con los puertos abiertos, su servicio y versión detectada.

Uso:
    python main.py

El script gestiona automáticamente el entorno virtual y las dependencias.
"""

import os
import subprocess
import sys
import time
from pathlib import Path
from typing import NoReturn

# ──────────────────────────────────────────────
# Configuración
# ──────────────────────────────────────────────
HOST: str = "127.0.0.1"
START_PORT: int = 1
END_PORT: int = 1024
TIMEOUT: float = 2.0
MAX_THREADS: int = 50

# ──────────────────────────────────────────────
# Gestión de entorno virtual
# ──────────────────────────────────────────────

PROJECT_DIR = Path(__file__).resolve().parent
VENV_DIR = PROJECT_DIR / ".venv"
REQUIREMENTS_FILE = PROJECT_DIR / "requirements.txt"


def _get_python() -> str:
    """Retorna la ruta al intérprete de Python dentro del virtualenv."""
    if sys.platform == "win32":
        return str(VENV_DIR / "Scripts" / "python.exe")
    return str(VENV_DIR / "bin" / "python")


def _get_pip() -> str:
    """Retorna la ruta a pip dentro del virtualenv."""
    if sys.platform == "win32":
        return str(VENV_DIR / "Scripts" / "pip.exe")
    return str(VENV_DIR / "bin" / "pip")


def _ensure_venv() -> None:
    """Verifica que el virtualenv exista; si no, lo crea e instala dependencias."""
    if not VENV_DIR.exists():
        print(f"[+] Creando entorno virtual en {VENV_DIR}...")
        subprocess.check_call(
            [sys.executable, "-m", "venv", str(VENV_DIR)]
        )
        print("[+] Entorno virtual creado.")
    else:
        print("[+] Entorno virtual encontrado.")

    # Verificar e instalar dependencias
    pip_path = _get_pip()
    if REQUIREMENTS_FILE.exists():
        print("[+] Verificando dependencias...")
        try:
            subprocess.check_call(
                [pip_path, "install", "-r", str(REQUIREMENTS_FILE), "--quiet"]
            )
            print("[+] Dependencias instaladas.")
        except subprocess.CalledProcessError:
            print("[!] Error al instalar dependencias, continuando sin rich.")


def _run_in_venv() -> None:
    """Re-ejecuta el script dentro del virtualenv si no lo está ya."""
    venv_python = _get_python()

    # Si ya estamos dentro del virtualenv correcto, no hacer nada
    if sys.executable == venv_python:
        return

    # Si el virtualenv no existe aún, no podemos re-ejecutar
    if not VENV_DIR.exists():
        return

    print(f"[+] Re-ejecutando dentro del virtualenv: {venv_python}")
    os.execv(venv_python, [venv_python, __file__] + sys.argv[1:])


def setup_environment() -> None:
    """Configura el entorno virtual si es necesario y re-ejecuta dentro de él."""
    _ensure_venv()
    _run_in_venv()


# ──────────────────────────────────────────────
# Orquestación del escaneo
# ──────────────────────────────────────────────

def main() -> None:
    """Punto de entrada principal del port scanner."""
    # Medir tiempo de inicio
    start_time = time.time()

    print(f"{'='*60}")
    print(f"  PORT SCANNER LOCAL")
    print(f"{'='*60}")
    print(f"  Host:    {HOST}")
    print(f"  Puertos: {START_PORT}-{END_PORT}")
    print(f"  Timeout: {TIMEOUT}s")
    print(f"  Threads: {MAX_THREADS}")
    print(f"{'='*60}")
    print()

    try:
        # Importar módulos (pueden no tener rich si falló la instalación)
        from scanner import scan_ports
        from output import print_results

        print(f"[*] Escaneando {HOST} (puertos {START_PORT}-{END_PORT})...")
        print(f"[*] Esto puede tomar unos segundos...")
        print()

        results = scan_ports(
            host=HOST,
            start_port=START_PORT,
            end_port=END_PORT,
            timeout=TIMEOUT,
            max_threads=MAX_THREADS,
        )

        elapsed = time.time() - start_time

        print()
        print_results(results)
        print()
        print(f"  Tiempo total: {elapsed:.2f} segundos")

    except KeyboardInterrupt:
        print("\n[!] Escaneo interrumpido por el usuario.")
        sys.exit(1)
    except Exception as e:
        print(f"\n[!] Error durante el escaneo: {e}")
        sys.exit(1)


# ──────────────────────────────────────────────
# Ejecución
# ──────────────────────────────────────────────

if __name__ == "__main__":
    setup_environment()
    main()
