# Servidor UDP receptor (PC)

Recibe los datagramas JSON que envía la app **SensorUDP** desde el teléfono y
muestra estadísticas en vivo (paquetes/s, muestras/s por sensor, pérdidas por
número de secuencia, desorden, malformados). Solo requiere Python 3 (stdlib).

## Inicio rápido

```bash
# En la PC (misma red Wi-Fi que el teléfono):
./run_server.sh                                # escucha en 0.0.0.0:5005, salida en vivo
                                               # (equivale a: python3 -u sensorudp_server.py)

# Opcional: guardar todo a disco (una línea JSON por datagrama, para post-proceso)
./run_server.sh --out datos.jsonl

# Otras opciones (el script acepta las mismas que el .py)
./run_server.sh -p 5006                   # otro puerto
./run_server.sh -v                        # imprime una línea por datagrama
./run_server.sh -r                        # modo crudo: cada trama JSON completa en vivo
./run_server.sh -t                        # modo tabla: una fila por muestra (columnas alineadas)
./run_server.sh -c                        # modo CSV: una línea por muestra (índice + valores)
./run_server.sh -c -f acelerometro        # modo CSV filtrado: solo las muestras de ese sensor/tipo
./run_server.sh -i 5                      # resumen cada 5 s (acepta decimales, p. ej. 0.5)
```

`-r` (modo crudo): imprime cada datagrama tal cual llega, **al instante**, y desactiva
los resúmenes periódicos — ideal para ver el flujo en vivo o redirigir a un archivo
(`./run_server.sh -r > captura.jsonl`).

`-t` (modo tabla): imprime **cada muestra como una fila** con columnas alineadas —
hora con milisegundos, sensor, tipo y valores — también al instante y sin resúmenes
periódicos. Es la vista más cómoda para leer el flujo como una planilla:

```
HORA          SENSOR                    TIPO                 VALORES
14:32:05.812  KX120-1089                ACCELEROMETER          0.067     0.000    10.008
14:32:05.834  KX120-1089                ACCELEROMETER          0.067     0.000     9.998
14:32:05.856  MN26005UKDN               LIGHT                187.000
```

`-c` (modo CSV): una **línea por muestra**, ideal para planillas o para parsear con
cualquier herramienta. Campos: contador de fila y los valores con **precisión
completa** (tal como llegan del sensor). Sin resúmenes periódicos; se puede redirigir
a un archivo (`./run_server.sh -c > datos.csv`):

```
1672,0.08595,0.009000001,9.94095
1673,0.076050006,0.009000001,9.94095
1674,0.076050006,0.01905,9.94995
```

`-f TEXTO` (filtro opcional, combinable con `-t` y `-c`): muestra solo las muestras
cuyo **sensor o tipo** contenga ese texto (sin distinguir mayúsculas). Acepta español
(`acelerometro`, `luz`, `proximidad`, `magnetometro`, `pasos`, …) o el texto tal como
viene en los datos (`accelerometer`, `kx120`, `light`). El índice sigue contando 1, 2, 3…
solo las filas mostradas:

```
./run_server.sh -c -f acelerometro
1,0.076050006,0.009000001,10.017
2,0.06705,0.009000001,9.997951
```

En la app: escribir la **IP LAN de la PC** (por ejemplo `192.168.1.33`) y el
puerto `5005`, y pulsar **Iniciar**.

## Firewall (importante)

En las pruebas, `ufw` de esta PC bloqueaba el UDP entrante. Permitir el puerto
una sola vez:

```bash
sudo ufw allow 5005/udp
```

Para saber la IP LAN de la PC:

```bash
hostname -I | awk '{print $1}'
```

## Formato de los datagramas

```json
{"seq":123,"device":"a10s","model":"SM-A107M","timestamp":1729000000000,
 "samples":[{"sensor":"KX120-1089 Accelerometer","type":1,
             "typeName":"TYPE_ACCELEROMETER","eventTimestamp":123456789,
             "values":[0.1,0.2,9.8]}]}
```

- `seq` es el número de secuencia de la sesión (reinicia cuando se reinicia la
  transmisión) — se usa para contar pérdidas.
- Cada datagrama agrupa las muestras de ~20 ms (batching de la app).
- Con `--out`, cada datagrama se guarda tal cual (una línea JSON), ideal para
  `jq`, pandas o reenvíos.
