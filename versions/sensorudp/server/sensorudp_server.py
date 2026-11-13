#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
SensorUDP — Servidor receptor UDP (PC Linux)
============================================

Recibe por UDP los datagramas JSON que envía la app Android SensorUDP y
muestra estadísticas en vivo: paquetes, muestras, tasas por sensor,
pérdidas por número de secuencia, desorden y errores de parseo.

Esquema esperado (por datagrama):
    {
      "seq": 123,                     # número de secuencia (reinicia por sesión)
      "device": "a10s",               # nombre del dispositivo
      "model": "SM-A107M",            # modelo
      "timestamp": 1729000000000,     # epoch ms del lote
      "samples": [
        {"sensor": "KX120-1089 Accelerometer", "type": 1,
         "typeName": "TYPE_ACCELEROMETER", "eventTimestamp": 123456789,
         "values": [0.1, 0.2, 9.8]},
        ...
      ]
    }

Uso:
    python3 sensorudp_server.py                    # escucha en 0.0.0.0:5005
    python3 sensorudp_server.py -p 5006            # otro puerto
    python3 sensorudp_server.py --out datos.jsonl  # guarda datagramas crudos (JSONL)
    python3 sensorudp_server.py -v                 # imprime cada datagrama
    python3 sensorudp_server.py -i 5               # resumen cada 5 s (default 2 s)
    python3 sensorudp_server.py -r                 # modo crudo: imprime cada trama JSON en vivo
    python3 sensorudp_server.py -t                 # modo tabla: una fila por muestra (columnas alineadas)
    python3 sensorudp_server.py -c                 # modo CSV: N,valores... (índice + valores)
    python3 sensorudp_server.py -c -f acelerometro # modo CSV filtrado: solo ese sensor (o tipo)

Solo usa la librería estándar de Python 3. Cortar con Ctrl+C: imprime el
resumen final y sale ordenadamente.

Firewall: si ufw está activo en esta PC, hay que permitir el puerto UDP
(una sola vez, requiere sudo):
    sudo ufw allow 5005/udp
"""

import argparse
import json
import signal
import socket
import sys
import time
import unicodedata
from collections import deque

WINDOW = 5.0    # ventana (s) para calcular las tasas en vivo
MAX_LINES = 12  # máximo de sensores listados en cada resumen


def hhmmss():
    return time.strftime("%H:%M:%S")


def hhmmssms():
    """Hora local con milisegundos (para las filas del modo tabla)."""
    t = time.time()
    ms = int((t - int(t)) * 1000)
    return time.strftime("%H:%M:%S", time.localtime(t)) + f".{ms:03d}"


def port_type(value):
    """I-007: valida que el puerto sea un entero en el rango 1-65535."""
    try:
        port = int(value)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"puerto inválido: {value!r} (se espera un entero entre 1 y 65535)"
        ) from None
    if not (1 <= port <= 65535):
        raise argparse.ArgumentTypeError(
            f"puerto fuera de rango: {port} (rango válido: 1-65535)"
        )
    return port


def _sin_acentos(texto):
    """Quita los acentos para comparar (p. ej. 'acelerómetro' → 'acelerometro')."""
    return "".join(
        c for c in unicodedata.normalize("NFD", texto)
        if unicodedata.category(c) != "Mn"
    )


# Alias en español → texto que aparece en los datos (nombres/tipos en inglés).
FILTER_ALIASES = {
    "acelerometro": "accelerometer",
    "luz": "light",
    "proximidad": "proximity",
    "magnetometro": "magnetic",
    "brujula": "magnetic",
    "presion": "pressure",
    "barometro": "pressure",
    "paso": "step",
    "pasos": "step",
    "orientacion": "orientation",
    "giroscopio": "gyroscope",
}


def normaliza_filtro(texto):
    """Normaliza el texto del filtro: sin acentos/mayúsculas y traduce alias."""
    t = _sin_acentos(texto).strip().lower()
    return FILTER_ALIASES.get(t, t)


class Stats:
    def __init__(self):
        self.t0 = time.monotonic()
        self.pkts = 0
        self.nbytes = 0
        self.nsamples = 0
        self.malformed = 0
        self.lost = 0
        self.ooo = 0
        self.sessions = 1
        self.write_errors = 0
        self.last_seq = None
        self.first_pkt_ts = None
        self.device = "?"
        self.model = "?"
        self.sender = "?"
        self.total_by_type = {}
        self.name_by_type = {}
        self.win_by_type = {}
        self.rows = 0  # contador de filas del modo CSV (una por muestra)
        self.mostradas = 0  # filas impresas en -t/-c (para el resumen con filtro)
        self.filter_name = None  # texto de filtro activo (normalizado) o None

    # -- ingesta -----------------------------------------------------------
    def note_packet(self, raw, addr, out_fh, verbose, raw_out=False, tabla=False,
                    csv_mode=False, filtro=None):
        """Procesa un datagrama. Devuelve el archivo de salida, que pasa a
        None si una escritura falla (I-008: degradación a modo solo-consola).

        Con `tabla=True` imprime cada muestra al instante como una fila en
        columnas: hora con milisegundos, sensor, tipo y valores.

        Con `csv_mode=True` imprime cada muestra como una línea CSV:
        N,valores... (contador de fila + valores con precisión completa).

        Con `filtro` (texto ya normalizado) solo se imprimen las muestras
        cuyo sensor o tipo lo contengan; aplica a los modos tabla y CSV."""
        now = time.monotonic()
        try:
            pkt = json.loads(raw.decode("utf-8", errors="replace"))
            if not isinstance(pkt, dict):
                raise ValueError("datagrama no es un objeto JSON")
            samples = pkt.get("samples") or []
            if not isinstance(samples, list):
                raise ValueError("campo 'samples' inválido")
        except (ValueError, UnicodeDecodeError):
            self.malformed += 1
            if verbose:
                print(f"[{hhmmss()}] malformado: {len(raw)} B de {addr[0]}:{addr[1]}")
            return out_fh

        first = self.pkts == 0
        self.pkts += 1
        self.nbytes += len(raw)
        if first:
            self.first_pkt_ts = now
        if out_fh is not None:
            try:
                out_fh.write(raw.decode("utf-8", errors="replace").strip() + "\n")
                out_fh.flush()
            except (OSError, ValueError) as exc:
                # I-008: no abortar por un error de E/S; seguir en consola.
                self.write_errors += 1
                print(f"[{hhmmss()}] AVISO: error al escribir en el archivo ({exc}). "
                      f"Se continúa en modo solo-consola.", file=sys.stderr)
                try:
                    out_fh.close()
                except (OSError, ValueError):
                    pass
                out_fh = None

        # identificación del emisor
        dev = str(pkt.get("device", "?"))
        mdl = str(pkt.get("model", "?"))
        if first:
            self.device, self.model = dev, mdl
            self.sender = f"{addr[0]}:{addr[1]}"
            print(f"[{hhmmss()}] Primer datagrama de {self.sender} — device={dev} model={mdl}")
        elif dev != self.device or mdl != self.model:
            self.device, self.model = dev, mdl
            print(f"[{hhmmss()}] Cambio de dispositivo: {dev} ({mdl}) desde {addr[0]}:{addr[1]}")

        # número de secuencia: pérdidas / desorden / nueva sesión
        seq = pkt.get("seq")
        if isinstance(seq, int):
            if self.last_seq is None:
                self.last_seq = seq
            elif seq > self.last_seq:
                gap = seq - self.last_seq - 1
                if gap > 0:
                    self.lost += gap
                self.last_seq = seq
            elif seq < self.last_seq:
                if seq <= 3 and (self.last_seq - seq) > 50:  # heurística: reinicio de sesión
                    self.sessions += 1
                    self.last_seq = seq
                    if verbose:
                        print(f"[{hhmmss()}] nueva sesión (seq reinicia a {seq})")
                else:
                    self.ooo += 1

        # muestras
        for s in samples:
            if not isinstance(s, dict):
                continue
            tname = str(s.get("typeName") or s.get("type") or "?")
            filtro_ok = (
                filtro is None
                or filtro in tname.lower()
                or filtro in str(s.get("sensor") or "").lower()
            )
            if tabla and filtro_ok:
                vals = s.get("values")
                if isinstance(vals, list):
                    nums = " ".join(
                        f"{v:9.3f}" if isinstance(v, (int, float)) and not isinstance(v, bool)
                        else f"{str(v):>9}"
                        for v in vals
                    )
                else:
                    nums = "" if vals is None else str(vals)
                sensor_name = str(s.get("sensor") or "?")
                line = f"{hhmmssms()}  {sensor_name[:24]:<24}  {tname[:20]:<20} {nums}"
                print(line.rstrip(), flush=True)
                self.mostradas += 1
            if csv_mode and filtro_ok:
                # Una línea por muestra: N,valores... (sin timestamp ni seq)
                self.rows += 1
                vals = s.get("values")
                if isinstance(vals, list):
                    # Precisión completa (str de Python conserva el valor original).
                    nums = ",".join(str(v) for v in vals)
                elif vals is None:
                    nums = ""
                else:
                    nums = str(vals)
                print(f"{self.rows},{nums}", flush=True)
                self.mostradas += 1
            self.nsamples += 1
            self.total_by_type[tname] = self.total_by_type.get(tname, 0) + 1
            if tname not in self.name_by_type:
                self.name_by_type[tname] = str(s.get("sensor") or "")
            dq = self.win_by_type.setdefault(tname, deque())
            dq.append(now)
            while dq and (now - dq[0]) > WINDOW:
                dq.popleft()

        if raw_out:
            print(raw.decode("utf-8", errors="replace").strip(), flush=True)
        if verbose:
            print(f"[{hhmmss()}] seq={seq} n={len(samples)} {len(raw)}B desde {addr[0]}:{addr[1]}")

        return out_fh

    # -- presentación ------------------------------------------------------
    def _span(self, now):
        return max(1.0, min(WINDOW, now - self.t0))

    def print_tick(self):
        now = time.monotonic()
        el = max(0.001, now - self.t0)
        span = self._span(now)
        print(f"[{hhmmss()}] pkts={self.pkts} ({self.pkts / el:.1f}/s) "
              f"muestras={self.nsamples} (~{self.nsamples / el:.0f}/s) "
              f"| perdidas={self.lost} desorden={self.ooo} err={self.malformed} "
              f"| sesiones={self.sessions}")
        rates = []
        for tname, dq in self.win_by_type.items():
            if dq:
                rates.append((len(dq) / span, tname))
        rates.sort(reverse=True)
        for rate, tname in rates[:MAX_LINES]:
            name = self.name_by_type.get(tname, "")
            extra = f"  ({name})" if name and name != tname else ""
            print(f"    {tname:<28} {rate:7.1f} Hz   total={self.total_by_type.get(tname, 0)}{extra}")
        sys.stdout.flush()

    def print_final(self):
        el = max(0.001, time.monotonic() - self.t0)
        print()
        print("=== Resumen final ===")
        print(f"Duración            : {el:.1f} s")
        if self.first_pkt_ts is not None:
            print(f"Primer datagrama    : hace {time.monotonic() - self.first_pkt_ts:.1f} s (desde {self.sender})")
        print(f"Dispositivo         : device={self.device} model={self.model}")
        print(f"Paquetes            : {self.pkts} ({self.pkts / el:.1f}/s) — {self.nbytes / 1024:.1f} KiB "
              f"({self.nbytes / el / 1024:.1f} KiB/s)")
        print(f"Muestras            : {self.nsamples} (~{self.nsamples / el:.0f}/s)")
        if self.filter_name:
            print(f"Filtro              : '{self.filter_name}' (muestras mostradas: {self.mostradas})")
        print(f"Pérdidas / desorden : {self.lost} / {self.ooo} (por número de secuencia)")
        print(f"Malformados         : {self.malformed}")
        if self.write_errors:
            print(f"Errores de escritura: {self.write_errors} (se degradó a modo solo-consola)")
        print(f"Sesiones            : {self.sessions}")
        if self.total_by_type:
            print("Muestras por sensor :")
            for tname, cnt in sorted(self.total_by_type.items(), key=lambda kv: -kv[1]):
                name = self.name_by_type.get(tname, "")
                extra = f"  ({name})" if name and name != tname else ""
                print(f"    {tname:<28} total={cnt}{extra}")


def main():
    ap = argparse.ArgumentParser(description="Servidor UDP receptor para la app SensorUDP.")
    ap.add_argument("-b", "--bind", default="0.0.0.0", help="IP local donde escuchar (default 0.0.0.0)")
    ap.add_argument("-p", "--port", type=port_type, default=5005,
                    help="puerto UDP, rango 1-65535 (default 5005)")
    ap.add_argument("--out", default=None, metavar="ARCHIVO.jsonl",
                    help="guarda cada datagrama crudo (una línea JSON por paquete)")
    ap.add_argument("-v", "--verbose", action="store_true", help="imprime cada datagrama recibido")
    ap.add_argument("-r", "--raw", action="store_true",
                    help="modo crudo: imprime la trama JSON completa de cada datagrama en vivo "
                         "(desactiva los resúmenes periódicos; ideal para redirigir a un archivo)")
    ap.add_argument("-t", "--tabla", action="store_true",
                    help="modo tabla: imprime cada muestra como una fila en columnas "
                         "(hora, sensor, tipo, valores; desactiva los resúmenes periódicos)")
    ap.add_argument("-c", "--csv", action="store_true",
                    help="modo CSV: una línea por muestra con el contador de fila "
                         "y los valores (precisión completa) "
                         "(desactiva los resúmenes periódicos)")
    ap.add_argument("-f", "--filtro", default=None, metavar="TEXTO",
                    help="muestra solo las muestras cuyo sensor o tipo contenga este texto "
                         "(p. ej. acelerometro, kx120, light; aplica a -t y -c)")
    ap.add_argument("-i", "--interval", type=float, default=2.0,
                    help="segundos entre resúmenes periódicos (default 2)")
    args = ap.parse_args()
    if args.filtro:
        args.filtro = normaliza_filtro(args.filtro)

    # I-009: atender SIGTERM con la misma salida ordenada que Ctrl+C.
    if hasattr(signal, "SIGTERM"):
        def _on_sigterm(_signum, _frame):
            raise KeyboardInterrupt
        signal.signal(signal.SIGTERM, _on_sigterm)

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((args.bind, args.port))
    except OSError as exc:
        print(f"ERROR: no se pudo abrir UDP {args.bind}:{args.port} — {exc}", file=sys.stderr)
        print("¿Puerto ocupado? Pruebe otro con -p.", file=sys.stderr)
        return 1

    out_fh = None
    if args.out:
        try:
            out_fh = open(args.out, "a", encoding="utf-8")
        except OSError as exc:
            print(f"ERROR: no se pudo abrir {args.out} — {exc}", file=sys.stderr)
            return 1

    print(f"SensorUDP — servidor receptor escuchando en UDP {args.bind}:{args.port}")
    mode_note = ("resúmenes periódicos desactivados" if (args.raw or args.tabla or args.csv)
                 else f"resumen cada {args.interval:g} s")
    print(f"{'Guardando en ' + args.out if out_fh else 'Modo consola (sin guardar)'}"
          f" · {mode_note} · Ctrl+C para salir")
    if args.raw:
        print("Modo crudo: cada trama JSON se imprime tal cual llega (resúmenes periódicos desactivados).")
    if args.tabla:
        print("Modo tabla: una fila por muestra en vivo (resúmenes periódicos desactivados).")
        print()
        print(f"{'HORA':<12}  {'SENSOR':<24}  {'TIPO':<20} VALORES")
    elif args.csv:
        print("Modo CSV: una línea por muestra (resúmenes periódicos desactivados).")
        print("Formato: N,valores... (p. ej. 1,0.08,0.01,9.94)")
    else:
        print(f"(Si no llegan paquetes desde el teléfono: revise la IP destino en la app y el firewall: "
              f"sudo ufw allow {args.port}/udp)")
    if args.filtro and not args.raw:
        print(f"Filtro activo: solo se imprimen muestras que contengan '{args.filtro}'.")
    print()

    stats = Stats()
    stats.filter_name = args.filtro
    sock.settimeout(0.5)
    next_tick = time.monotonic() + args.interval
    try:
        while True:
            try:
                raw, addr = sock.recvfrom(65535)
                # I-008: el archivo puede pasar a None si falla una escritura.
                out_fh = stats.note_packet(raw, addr, out_fh, args.verbose, args.raw, args.tabla,
                                           args.csv, args.filtro)
            except TimeoutError:
                pass
            now = time.monotonic()
            if not (args.raw or args.tabla or args.csv) and now >= next_tick:
                stats.print_tick()
                next_tick = now + args.interval
    except KeyboardInterrupt:
        pass
    finally:
        # I-009: cerrar recursos primero, garantizando ambos cierres con
        # `finally` anidado aunque falle uno de los `close()`...
        try:
            sock.close()
        finally:
            if out_fh is not None:
                out_fh.close()
        # ...y asegurar que el resumen final nunca impida una salida limpia.
        try:
            stats.print_final()
        except Exception as exc:
            print(f"[aviso] no se pudo imprimir el resumen final: {exc}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
