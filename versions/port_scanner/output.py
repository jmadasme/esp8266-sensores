"""
Formateo de salida en consola.

Genera una tabla con los resultados del escaneo usando la librería rich
si está disponible, o formato básico como fallback.
"""

from typing import Any

try:
    from rich.console import Console
    from rich.table import Table
    from rich.style import Style

    _RICH_AVAILABLE = True
except ImportError:
    _RICH_AVAILABLE = False


def print_results(results: list[dict[str, Any]]) -> None:
    """Imprime los resultados del escaneo en una tabla formateada.

    Args:
        results: Lista de diccionarios con los resultados de cada puerto.
                 Cada diccionario debe contener: port, open, service, version.
    """
    if not results:
        print("No se encontraron puertos abiertos.")
        return

    if _RICH_AVAILABLE:
        _print_rich(results)
    else:
        _print_plain(results)


def _print_rich(results: list[dict[str, Any]]) -> None:
    """Imprime los resultados usando rich (tabla con colores)."""
    console = Console()

    table = Table(title="Resultados del Escaneo de Puertos")
    table.add_column("Puerto", style="cyan", justify="right")
    table.add_column("Estado", style="bold", justify="center")
    table.add_column("Servicio", style="green")
    table.add_column("Versión", style="yellow")

    for r in results:
        port = str(r["port"])
        open_status = "Abierto"
        service = r.get("service", "desconocido")
        version = r.get("version", "") or ""

        table.add_row(port, open_status, service, version)

    console.print(table)

    # Resumen
    console.print(f"\n[bold]Resumen:[/bold] {len(results)} puerto(s) abierto(s) encontrado(s).")


def _print_plain(results: list[dict[str, Any]]) -> None:
    """Imprime los resultados en formato de texto plano alineado."""
    headers = ["Puerto", "Estado", "Servicio", "Versión"]
    rows: list[list[str]] = []

    for r in results:
        rows.append([
            str(r["port"]),
            "Abierto",
            r.get("service", "desconocido"),
            r.get("version", "") or "",
        ])

    # Calcular anchos de columna
    col_widths = [len(h) for h in headers]
    for row in rows:
        for i, cell in enumerate(row):
            col_widths[i] = max(col_widths[i], len(cell))

    # Línea separadora
    sep = "-+-".join("-" * w for w in col_widths)

    # Imprimir encabezados
    header_line = " | ".join(h.ljust(w) for h, w in zip(headers, col_widths))
    print(header_line)
    print(sep)

    # Imprimir filas
    for row in rows:
        line = " | ".join(cell.ljust(w) for cell, w in zip(row, col_widths))
        print(line)

    print(f"\nResumen: {len(results)} puerto(s) abierto(s) encontrado(s).")
