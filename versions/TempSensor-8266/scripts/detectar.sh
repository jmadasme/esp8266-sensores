#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════
#  detectar.sh — Escanea los puertos USB y dice si la placa está conectada
#
#  Uso:  ./scripts/detectar.sh
#  Devuelve 0 si encuentra una placa, 1 si no.
# ═══════════════════════════════════════════════════════════════════
set -u

ARDUINO_CLI="${ARDUINO_CLI:-$HOME/.local/bin/arduino-cli}"

echo "════════════════════════════════════════════════════"
echo " DETECCIÓN DE PLACA — NodeMCU ESP8266"
echo "════════════════════════════════════════════════════"

# ── 1. Puertos serie presentes en el sistema ──────────────────────
echo
echo "[1] Puertos serie del sistema:"
PUERTOS=$(ls -1 /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | grep -v '\.') || true
if [ -z "$PUERTOS" ]; then
    echo "    (ninguno)"
else
    for p in $PUERTOS; do
        PERMISOS=$(stat -c '%A' "$p" 2>/dev/null)
        echo "    $p   $PERMISOS"
    done
fi

# ── 2. Dispositivos USB plugged ────────────────────────────────────
echo
echo "[2] Dispositivos USB conectados:"
if command -v lsusb >/dev/null 2>&1; then
    USB=$(lsusb 2>/dev/null | grep -iE 'ch340|cp210|esp|1a86|10c4|wch|serial|uart')
    if [ -z "$USB" ]; then
        echo "    (ningún adaptador USB-serie conocido)"
    else
        echo "$USB" | while read -r line; do echo "    $line"; done
    fi
else
    echo "    (lsusb no instalado, se omite)"
fi

# ── 3. Detección real vía arduino-cli ──────────────────────────────
echo
echo "[3] Detección con arduino-cli:"
if [ ! -x "$ARDUINO_CLI" ]; then
    echo "    ERROR: no se encontró arduino-cli en $ARDUINO_CLI"
    echo "    Instalalo o seteá ARDUINO_CLI=/ruta/a/arduino-cli"
    exit 2
fi

BOARD_OUT=$("$ARDUINO_CLI" board list 2>&1 || true)
PLACAS=$(echo "$BOARD_OUT" | grep -Eo '/dev/tty(USB|ACM)[0-9]*' | sort -u)

if [ -z "$PLACAS" ]; then
    echo "    ✗ No se detectó ninguna placa."
    echo
    echo "  Diagnóstico:"
    echo "    · Revisá el cable USB — muchos son solo de datos, no de carga."
    echo "    · Probá otro puerto o un hub con más corriente."
    echo "    · En Linux, si el puerto aparece y desaparece, suele ser brltty"
    echo "     ovitchupándalo con 'sudo systemctl stop brltty'."
    echo "    · Necesitás ser miembro de 'dialout':  sudo usermod -aG dialout \$USER"
    exit 1
fi

for p in $PLACAS; do
    echo "    ✓ $p"
done

echo
echo "════════════════════════════════════════════════════"
echo " PLACA DETECTADA en: $(echo "$PLACAS" | head -1)"
echo "════════════════════════════════════════════════════"
echo "Para usarla explícitamente:"
echo "    ./scripts/compilar.sh"
echo "    ./scripts/subir.sh $(echo "$PLACAS" | head -1)"
echo
exit 0
