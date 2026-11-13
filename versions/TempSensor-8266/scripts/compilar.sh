#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════
#  compilar.sh — Cross-compila el sketch para el NodeMCU ESP8266
#
#  Uso:  ./scripts/compilar.sh
#  Envía: TempSensor-8266.ino  →  firmware .bin
#
#  NOTA IMPORTANTE: arduino-cli upload NO recompila. Siempre hay que
#  correr este script antes de subir, o se sube el binario viejo.
# ═══════════════════════════════════════════════════════════════════
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROYECTO_DIR="$(dirname "$SCRIPT_DIR")"

ARDUINO_CLI="${ARDUINO_CLI:-$HOME/.local/bin/arduino-cli}"
FQBN="${FQBN:-esp8266:esp8266:nodemcuv2}"
SKETCH_NAME="${SKETCH_NAME:-TempSensor-8266}"
BUILD_DIR="$PROYECTO_DIR/build"

if [ ! -x "$ARDUINO_CLI" ]; then
    echo "ERROR: no se encontró arduino-cli en $ARDUINO_CLI"
    echo "       Instalalo o seteá ARDUINO_CLI=/ruta/a/arduino-cli"
    exit 2
fi

if [ ! -f "$PROYECTO_DIR/$SKETCH_NAME.ino" ]; then
    echo "ERROR: no existe $PROYECTO_DIR/$SKETCH_NAME.ino"
    exit 2
fi

if [ ! -f "$PROYECTO_DIR/secrets.h" ]; then
    echo "ADVERTENCIA: falta secrets.h — el firmware no va a compilar sin credenciales."
    echo "             Copiá secrets.example.h a secrets.h y completá los valores."
fi

echo "════════════════════════════════════════════════════"
echo " COMPILANDO — $SKETCH_NAME"
echo " Sketch : $PROYECTO_DIR/$SKETCH_NAME.ino"
echo " Placa  : $FQBN"
echo "════════════════════════════════════════════════════"
echo

mkdir -p "$BUILD_DIR"
LOG="$BUILD_DIR/compile.log"

# Compila una sola vez, guardando la salida para poder mostrar el resumen
# de memoria. set +e porque queremos manejar el exit code nosotros.
set +e
"$ARDUINO_CLI" compile \
    --fqbn "$FQBN" \
    --warnings all \
    --export-binaries \
    --build-path "$BUILD_DIR" \
    "$PROYECTO_DIR" 2>&1 | tee "$LOG"
RC=${PIPESTATUS[0]}
set -e

echo
if [ $RC -ne 0 ]; then
    echo "✗ COMPILACIÓN FALLIDA (exit $RC)"
    echo "  Revisá los errores de arriba. Nada se subió a la placa."
    exit $RC
fi

echo "════════════════════════════════════════════════════"
echo " ✓ COMPILACIÓN EXITOSA"
echo "════════════════════════════════════════════════════"
grep -E 'used [0-9]+ / [0-9]+ bytes' "$LOG" || true
echo
echo "Para subirlo a la placa:"
echo "    ./scripts/detectar.sh          # ver el puerto"
echo "    ./scripts/subir.sh             # compila y sube"
