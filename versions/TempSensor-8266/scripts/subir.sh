#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════
#  subir.sh — Compila y transfiere el firmware a la placa
#
#  Uso:  ./scripts/subir.sh              # auto-detecta el puerto
#        ./scripts/subir.sh /dev/ttyUSB0 # puerto explícito
#
#  IMPORTANTE: compila SIEMPRE antes de subir. arduino-cli upload no
#  recompila — si sólo cambiás el .ino y subís, se sube el binario viejo.
# ═══════════════════════════════════════════════════════════════════
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROYECTO_DIR="$(dirname "$SCRIPT_DIR")"

ARDUINO_CLI="${ARDUINO_CLI:-$HOME/.local/bin/arduino-cli}"
FQBN="${FQBN:-esp8266:esp8266:nodemcuv2}"

# ── Determinar puerto ──────────────────────────────────────────────
PUERTO="${1:-}"

if [ -z "$PUERTO" ]; then
    echo "Buscando la placa en los puertos USB..."
    # Nunca hardcodear /dev/ttyUSB0: puede ser ttyACM0 (CDC nativo)
    PUERTO=$("$ARDUINO_CLI" board list 2>/dev/null \
             | grep -Eo '/dev/tty(USB|ACM)[0-9]*' | head -1 || true)

    if [ -z "$PUERTO" ]; then
        echo
        echo "✗ No se detectó ninguna placa conectada."
        echo
        echo "  Probá:"
        echo "    · Cambiá el cable USB (muchos son sólo de carga, sin datos)"
        echo "    · Probá otro puerto USB o un hub con más corriente"
        echo "    · Verificá permisos:  sudo usermod -aG dialout \$USER  (luego re-logueate)"
        echo "    · En Linux, si el puerto aparece y desaparece, es brltty:"
        echo "        sudo systemctl stop brltty && sudo systemctl disable brltty"
        echo
        echo "  Diagnóstico completo:  ./scripts/detectar.sh"
        exit 1
    fi
    echo "✓ Placa detectada en $PUERTO"
else
    if [ ! -e "$PUERTO" ]; then
        echo "✗ El puerto $PUERTO no existe."
        echo "  Puertos disponibles:"
        ls -1 /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | sed 's/^/    /' || echo "    (ninguno)"
        exit 1
    fi
    echo "✓ Usando puerto $PUERTO"
fi

# ── Compilar primero ───────────────────────────────────────────────
echo
echo "════════════════════════════════════════════════════"
echo " 1/2  COMPILANDO"
echo "════════════════════════════════════════════════════"
"$SCRIPT_DIR/compilar.sh"

# ── Subir ──────────────────────────────────────────────────────────
echo
echo "════════════════════════════════════════════════════"
echo " 2/2  TRANSFERIENDO A LA PLACA ($PUERTO)"
echo "════════════════════════════════════════════════════"
echo
echo "Si se queda trabado en 'connecting...':"
echo "  · Mantené el botón FLASH presionado y volvé a correr el script."
echo "  · O usá un cable más corto o un hub USB si la placa se reinicia."
echo

set +e
"$ARDUINO_CLI" upload -p "$PUERTO" --fqbn "$FQBN" "$PROYECTO_DIR"
RC=$?
set -e

echo
if [ $RC -ne 0 ]; then
    echo "✗ TRANSFERENCIA FALLIDA (exit $RC)"
    exit $RC
fi

echo "════════════════════════════════════════════════════"
echo " ✓ FIRMWARE TRANSFERIDO"
echo "════════════════════════════════════════════════════"
echo
echo "Para ver el log por serie a 115200:"
echo "    ./scripts/monitor.sh $PUERTO"
