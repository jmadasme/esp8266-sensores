#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════
#  monitor.sh — Muestra el log por serie de la placa a 115200 baudios
#
#  Uso:  ./scripts/monitor.sh              # auto-detecta el puerto
#        ./scripts/monitor.sh /dev/ttyUSB0 # puerto explícito
#        ./scripts/monitor.sh /dev/ttyUSB0 60   # 60 segundos y sale
#
#  Para salir antes: Ctrl-A y después K  (de screen)
#
#  NOTA: screen muestra basura al inicio porque el ESP manda bytes en
#  blanco al arrancar. Para un log limpio con el pulso de reset aplicado,
#  usar read_serial.py (solo en /tmp, no forma parte del repo).
# ═══════════════════════════════════════════════════════════════════
set -euo pipefail

ARDUINO_CLI="${ARDUINO_CLI:-$HOME/.local/bin/arduino-cli}"
PUERTO="${1:-}"
SEGUNDOS="${2:-0}"

if [ -z "$PUERTO" ]; then
    PUERTO=$("$ARDUINO_CLI" board list 2>/dev/null \
             | grep -Eo '/dev/tty(USB|ACM)[0-9]*' | head -1 || true)
    if [ -z "$PUERTO" ]; then
        echo "✗ No se detectó ninguna placa. Probá: ./scripts/detectar.sh"
        exit 1
    fi
fi

if [ ! -e "$PUERTO" ]; then
    echo "✗ El puerto $PUERTO no existe."
    exit 1
fi

echo "Escuchando $PUERTO a 115200 baudios..."
echo "  Ctrl-A luego K para salir."
echo

if [ "$SEGUNDOS" -gt 0 ] 2>/dev/null; then
    timeout "${SEGUNDOS}s" screen "$PUERTO" 115200
else
    screen "$PUERTO" 115200
fi
