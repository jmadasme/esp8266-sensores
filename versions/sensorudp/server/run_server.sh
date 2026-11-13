#!/usr/bin/env bash
# SensorUDP — inicia el servidor receptor UDP en primer plano.
# La salida es en vivo por la terminal (sin logs a archivo). Ctrl+C para detener.
#
# Uso:
#   ./run_server.sh                       # escucha en 0.0.0.0:5005
#   ./run_server.sh -p 5006               # otro puerto
#   ./run_server.sh --out datos.jsonl     # además guarda cada datagrama a disco
#   ./run_server.sh -v                    # imprime una línea por datagrama
#   ./run_server.sh -r                    # modo crudo: cada trama JSON completa en vivo
#   ./run_server.sh -t                    # modo tabla: una fila por muestra (columnas)
#   ./run_server.sh -c                    # modo CSV: una línea por muestra (índice + valores)
#   ./run_server.sh -c -f acelerometro    # modo CSV filtrado (solo ese sensor/tipo)
#   ./run_server.sh -i 5                  # resumen cada 5 segundos
#
set -euo pipefail
cd "$(dirname "$0")"
exec python3 -u sensorudp_server.py "$@"
