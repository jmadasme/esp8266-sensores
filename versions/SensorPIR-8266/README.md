# SensorPIR-8266 -- Sensor de Movimiento IoT con ESP8266

Firmware para **NodeMCU ESP8266** que detecta movimiento mediante un **sensor PIR HC-SR501** y publica alertas en tiempo real via **MQTT** al broker `www.smartage.cl:1886`. Basado en la arquitectura resiliente **lp101_secure** con watchdog por software, ping check, reconexion automatica WiFi/MQTT y aleatorizacion de MAC ante fallos.

La conexion WiFi es **directa** mediante `WiFi.begin()` con credenciales definidas en `secrets.h`.

---

## Tabla de Contenidos

- [Descripcion General](#descripcion-general)
- [Placa y Especificaciones](#placa-y-especificaciones)
- [Sensor PIR HC-SR501 -- Principio de Funcionamiento](#sensor-pir-hc-sr501--principio-de-funcionamiento)
- [Tabla de Pines (Pinout)](#tabla-de-pines-pinout)
- [Diagrama de Conexion](#diagrama-de-conexion)
- [Instrucciones de Uso](#instrucciones-de-uso)
- [Compilacion y Carga](#compilacion-y-carga)
- [Mensajes MQTT](#mensajes-mqtt)
- [Comandos MQTT](#comandos-mqtt)
- [Indicador LED de Estado](#indicador-led-de-estado)
- [Resiliencia y Recuperacion ante Fallos](#resiliencia-y-recuperacion-ante-fallos)
- [Ejemplos de Uso](#ejemplos-de-uso)
- [Calibracion del Sensor PIR](#calibracion-del-sensor-pir)
- [Advertencias y Precauciones](#advertencias-y-precauciones)
- [Estructura del Proyecto](#estructura-del-proyecto)
- [Licencia](#licencia)

---

## Descripcion General

**SensorPIR-8266** es un dispositivo IoT autonomo que monitoriza movimiento en un espacio fisico. Al detectar movimiento, publica inmediatamente una alerta MQTT; al cesar el movimiento, publica un mensaje de estado `false`. Ademas, envia un heartbeat periodico cada 30 segundos para que el sistema central sepa que sigue operativo.

El firmware hereda la arquitectura de **lp101_secure**, garantizando alta disponibilidad mediante:

- Conexion WiFi directa con `WiFi.begin()` (credenciales en `secrets.h`)
- Watchdog por software (180s timeout)
- Ping check a 8.8.8.8 cada 30s (20 fallos consecutivos provocan reinicio)
- Reconexion WiFi y MQTT automatica con reintentos
- Aleatorizacion de MAC ante fallos persistentes (evita bloqueos por DHCP / AP)
- Control remoto de encendido/apagado del sensor via comando MQTT (`directive/powerState`)

---

## Placa y Especificaciones

| Parametro | Valor |
|-----------|-------|
| **Placa** | NodeMCU ESP8266 (ESP-12E / LoLin V3) |
| **Microcontrolador** | Tensilica L106 32-bit RISC |
| **Frecuencia** | 80 MHz |
| **Flash** | 4 MB |
| **RAM disponible** | ~80 KB (heap para usuario) |
| **Voltaje logico** | 3.3V (pines GPIO) |
| **Alimentacion** | 5V via USB (cable micro-USB estandar) |
| **Conectividad** | WiFi 802.11 b/g/n 2.4 GHz integrada |
| **Conexion WiFi** | Directa via `WiFi.begin()` |
| **FQBN** | `esp8266:esp8266:nodemcuv2` |
| **Sensor** | PIR HC-SR501 (infrarrojo pasivo) |
| **Protocolo** | MQTT v3.1.1 |
| **Broker** | `www.smartage.cl:1886` |
| **Device ID** | `6a7357a00221f201ddcc5f1d` |
| **WiFi SSID** | `Nv2` |
| **Usuario MQTT** | `luisHernandez` |

---

## Sensor PIR HC-SR501 -- Principio de Funcionamiento

El **HC-SR501** es un sensor **infrarrojo pasivo (PIR)** que detecta cambios en la radiacion infrarroja dentro de su campo de vision. Su funcionamiento se basa en:

1. **Lente de Fresnel**: concentra la radiacion IR del entorno sobre el sensor piroelectrico, dividiendo el area de cobertura en zonas (calientes y frias). Tiene un angulo de deteccion de aproximadamente 110deg y un alcance de 3 a 7 metros.

2. **Sensor piroelectrico**: contiene dos elementos sensibles conectados en oposicion. Cuando una persona (fuente de calor en movimiento) cruza de una zona a otra, la diferencia de radiacion entre los dos elementos genera un pulso de voltaje.

3. **Chip controlador BISS0001**: amplifica y procesa la senal del sensor piroelectrico. En **modo H (repetible)**, la salida se mantiene en HIGH mientras el movimiento continue. Este modo es esencial para que el firmware pueda detectar correctamente el cese de movimiento.

4. **Salida digital TTL** (pin OUT):
   - **HIGH (3.3V)**: movimiento detectado
   - **LOW (0V)**: sin movimiento

### Requisito critico: Jumper en modo H

El jumper amarillo del HC-SR501 debe estar en la posicion **H** (repetible/retriggerable). En modo L (no repetible), la salida solo emite un pulso unico y no permite al firmware detectar correctamente el cese de movimiento.

```
         ┌─────────────────────┐
         │    HC-SR501          │
         │                      │
         │  ┌───┐    ┌───────┐  │
         │  │H L│    │  o  o │  │  <- potenciometros
         │  └─┬─┘    │Delay  │  │     (delay time, sensibilidad)
         │    │      │Sensib │  │
         │    └──────└───┬───┘  │
         │  Jumper en H  │      │
         └───────────────┴──────┘
```

### Logica de deteccion en firmware

El firmware utiliza **deteccion de flancos simple** (edge detection) con un intervalo minimo entre reportes de **200 ms** (`SENSOR_REPORT_MS`):

- **Flanco LOW->HIGH**: se detecta movimiento y se publica `report/detectionState = true`.
- **Flanco HIGH->LOW**: se detecta cese de movimiento y se publica `report/detectionState = false`.
- **Intervalo minimo**: no se envia un nuevo reporte si han pasado menos de 200 ms desde el anterior, evitando saturar el broker con reportes repetidos.

### Tiempo de calentamiento (warm-up)

El HC-SR501 requiere **aproximadamente 60 segundos** tras el encendido para estabilizar su circuito interno. El firmware espera **65 segundos** antes de comenzar a procesar lecturas, ignorando cualquier fluctuacion durante este periodo.

---

## Tabla de Pines (Pinout)

| Pin NodeMCU | GPIO | Modo | Componente Conectado | Notas |
|-------------|------|------|---------------------|-------|
| **D2** | GPIO4 | `INPUT` | **PIR HC-SR501 -- pin OUT** | Senal TTL 3.3V: HIGH = movimiento, LOW = sin movimiento. No requiere resistencia pull-down externa (el sensor entrega 0V en reposo). |
| **D4** | GPIO2 | `OUTPUT` | **LED_BUILTIN** (LED azul en placa) | Indicador de estado del sistema. Logica **invertida**: LOW = encendido, HIGH = apagado. NUNCA indica movimiento (solo estado WiFi/MQTT). Debe permanecer HIGH durante el boot. |
| **VU / Vin** | -- | PWR (5V) | **PIR HC-SR501 -- pin VCC** | 5V desde el puerto USB. El HC-SR501 opera entre 4.5V y 20V, asi que 5V es el voltaje ideal. **No conectar a pin de 3.3V** (el sensor requiere minimo 4.5V). |
| **GND** | -- | GND | **PIR HC-SR501 -- pin GND** | Tierra comun. Cualquier pin GND de la NodeMCU sirve. |
| TX (GPIO1) | GPIO1 | UART TX | USB-Serial integrado | Reservado para comunicacion Serial (115200 baud). **No conectar nada en este pin.** |
| RX (GPIO3) | GPIO3 | UART RX | USB-Serial integrado | Reservado para comunicacion Serial (115200 baud). **No conectar nada en este pin.** |

### Pines reservados que NO deben usarse

| Pin | GPIO | Motivo |
|-----|------|--------|
| D3 | GPIO0 | Debe estar HIGH durante el boot para modo normal. El NodeMCU tiene pull-up interno. |
| D8 | GPIO15 | Debe estar LOW durante el boot. El NodeMCU tiene pull-down interno. |
| D0 | GPIO16 | Solo usable para wake-up desde deep sleep. No utilizado en este proyecto. |

---

## Diagrama de Conexion

### Esquema de conexion

```
                  NodeMCU ESP8266                    HC-SR501 PIR Sensor
               ┌──────────────────┐              ┌─────────────────────┐
               │                  │              │                     │
               │  VU (5V)  ───────┼──────────────┼── VCC (5V)          │
               │                  │              │                     │
               │  GND      ───────┼──────────────┼── GND               │
               │                  │              │                     │
               │  D2 (GPIO4) ─────┼──────────────┼── OUT (senal)       │
               │                  │              │                     │
               │  D4 (GPIO2)       │  (LED azul integrado en placa)    │
               │                  │              │                     │
               │  micro-USB ───────┼── Alimentacion 5V y Serial       │
               │                  │              │                     │
               └──────────────────┘              └─────────────────────┘
```

### Vista fisica (protoboard)

```
   ┌────────────────────────────────────────────────────────┐
   │                                                        │
   │              NodeMCU ESP8266                           │
   │   ┌──────────────────────────────┐                     │
   │   │  D0  D1  D2  D3  D4  3V3 GND│                     │
   │   │  ██  ██  ██  ██  ██  ██  ██ │                     │
   │   │        ┌┤   │                │                     │
   │   │        ││   └────────────────┼───┐                 │
   │   │        ││                    │   │                 │
   │   │        ││   D5  D6  D7  D8 RX TX│   │             │
   │   │        ││   ██  ██  ██  ██  ██ ██│   │             │
   │   │   VU───┘│                    │   │                 │
   │   │   GND───┘                    │   │                 │
   │   └──────────────────────────────┘   │                 │
   │                                      │                 │
   │         ┌────────────────────┐       │                 │
   │         │   HC-SR501 PIR     │       │                 │
   │         │                    │       │                 │
   │         │  VCC o─────────────┼───────┘  (5V)          │
   │         │  OUT o─────────────┼─── D2 (GPIO4)          │
   │         │  GND o─────────────┼─── GND                 │
   │         │                    │                         │
   │         │   ╔═══╗            │                         │
   │         │   ║H L║ <- modo H  │                         │
   │         │   ╚═╤═╝            │                         │
   │         │    jumper          │                         │
   │         └────────────────────┘                         │
   │                                                        │
   │   Cable USB --- PC / cargador 5V                       │
   └────────────────────────────────────────────────────────┘
```

### Configuracion de los potenciometros del PIR

| Potenciometro | Ajuste recomendado | Razon |
|---------------|-------------------|-------|
| **Delay time** (izquierdo) | **Minimo** (giro antihorario completo) | Para que el cese de movimiento se detecte con precision inmediata |
| **Sensibilidad** (derecho) | **Maximo** (giro horario completo) | Maximo alcance de deteccion (~7m) |

### Jumper de trigger

| Posicion | Modo | Uso en este proyecto |
|----------|------|---------------------|
| **H** | Repetible (retriggerable) | **Usar esta.** La salida se mantiene HIGH mientras haya movimiento continuo |
| L | No repetible | No usar. Produce un solo pulso y no permite detectar correctamente el cese |

### Lista de materiales

| Cantidad | Componente | Descripcion |
|----------|------------|-------------|
| 1 | NodeMCU ESP8266 (ESP-12E) | Placa de desarrollo con WiFi |
| 1 | Sensor PIR HC-SR501 | Sensor de movimiento infrarrojo pasivo |
| 3 | Cables jumper hembra-hembra | Para conectar VCC, GND y OUT del PIR a la NodeMCU |
| 1 | Cable micro-USB | Para alimentacion y programacion |

**No se requieren resistencias ni componentes adicionales.** La NodeMCU alimenta directamente al PIR desde el pin VU (5V del USB).

---

## Instrucciones de Uso

### Primer arranque

1. **Conectar el hardware** segun el diagrama de conexion mostrado arriba. Verificar:
   - PIR VCC → NodeMCU VU (5V)
   - PIR OUT → NodeMCU D2 (GPIO4)
   - PIR GND → NodeMCU GND
   - Jumper del PIR en posicion **H**
   - Potenciometro de delay al minimo (antihorario)
   - Potenciometro de sensibilidad al maximo (horario)

2. **Configurar credenciales** en `secrets.h` antes de compilar:
   - WiFi: SSID `Nv2`, password `12345678`
   - MQTT: broker `www.smartage.cl:1886`, usuario `luisHernandez` / `luis123`
   - Device ID: `6a7357a00221f201ddcc5f1d`

3. **Compilar y cargar el firmware** (ver seccion [Compilacion y Carga](#compilacion-y-carga)).

4. **Abrir el Monitor Serial** a 115200 baud para ver los mensajes de diagnostico.

5. **Esperar ~65 segundos** mientras el PIR se inicializa. En el Serial veras:
   ```
   SensorPIR-8266 booted
     PIR: D2/GPIO4 | MQTT: report/detectionState, report/powerState
     Warmup: 65s...
   Connecting to Nv2
   ................
   Connected: Nv2
   IP: 192.168.1.100
   Connecting to MQTT...
   connected
   PIR ready
   ```

6. **Mover la mano frente al sensor** -- deberias ver `>>> MOTION DETECTED <<<` y recibir el mensaje MQTT `report/detectionState: true`.

7. **Dejar de mover** -- veras `--- motion cleared ---` y el mensaje `report/detectionState: false`.

### Operacion normal

Una vez inicializado, el dispositivo funciona de forma autonoma:

- El LED azul (D4) parpadea cada 20 segundos indicando que WiFi, MQTT y sistema estan OK.
- Cuando se detecta movimiento, publica inmediatamente por MQTT. El LED **NO** parpadea con el movimiento (es solo indicador de sistema).
- Cada 30 segundos envia un heartbeat `report/online` con valor `true`.
- Si se pierde WiFi o MQTT, reconecta automaticamente.
- Si hay fallos repetidos, reinicia con una MAC diferente para evitar bloqueos de red.

### Control remoto del sensor

El sensor puede encenderse o apagarse remotamente mediante comandos MQTT:

- Enviar `ON` al topic `6a7357a00221f201ddcc5f1d/directive/powerState` para **activar** el sensor.
- Enviar `OFF` al topic `6a7357a00221f201ddcc5f1d/directive/powerState` para **desactivar** el sensor (deja de reportar detecciones).

Cuando el sensor se desactiva, el firmware responde publicando `report/powerState: OFF`. Al reactivarse, publica `report/powerState: ON`.

---

## Compilacion y Carga

### Requisitos previos

- [arduino-cli](https://arduino.github.io/arduino-cli/) instalado
- NodeMCU ESP8266 conectada por USB
- Puerto serie identificado (ej: `/dev/ttyUSB0` en Linux, `COM3` en Windows)
- Credenciales configuradas en `secrets.h`

### Instalacion de arduino-cli

```bash
# Instalar arduino-cli si no esta instalado
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Mover al PATH si es necesario
sudo mv bin/arduino-cli /usr/local/bin/
```

### Instalacion del core ESP8266

```bash
# Anadir el indice de placas ESP8266
arduino-cli config add board_manager.additional_urls https://arduino.esp8266.com/stable/package_esp8266com_index.json

# Actualizar el indice
arduino-cli core update-index

# Instalar el core ESP8266
arduino-cli core install esp8266:esp8266
```

### Instalacion de librerias

```bash
# PubSubClient (cliente MQTT) - unica libreria externa requerida
arduino-cli lib install "PubSubClient"
```

Todas las demas librerias (`ESP8266WiFi`, `Ticker`, `Arduino.h`, `user_interface.h`) son built-in del core ESP8266.

### Compilar

```bash
# Desde el directorio del proyecto
cd /home/jma/Documents/Proyects/OpenCode/AG.CreadorContenido/proyectos/SensorPIR-8266

arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 .
```

### Transferir a la placa

```bash
# Ajusta el puerto segun tu sistema
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 .
```

En Windows: `-p COM3` (o el puerto que corresponda).

### Monitor Serial

```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### Comando todo-en-uno

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 . && \
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 . && \
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

---

## Mensajes MQTT

### Topics de publicacion (device -> broker)

| Topic | Payload | Frecuencia | Descripcion |
|-------|---------|------------|-------------|
| `<DEVICE_ID>/report/online` | `true` | Cada 30s | Heartbeat: confirma que el dispositivo esta vivo y conectado |
| `<DEVICE_ID>/report/detectionState` | `true` | Al detectar movimiento | Alerta de movimiento detectado (flanco LOW->HIGH del PIR) |
| `<DEVICE_ID>/report/detectionState` | `false` | Al cesar movimiento | Movimiento finalizado (flanco HIGH->LOW del PIR) |
| `<DEVICE_ID>/report/powerState` | `ON` | Al activarse el sensor | Sensor habilitado para detectar movimiento |
| `<DEVICE_ID>/report/powerState` | `OFF` | Al desactivarse el sensor | Sensor deshabilitado (no reporta detecciones) |

Donde `<DEVICE_ID>` es `6a7357a00221f201ddcc5f1d` (configurable en `secrets.h` via `MQTT_CLIENT`).

### Intervalo minimo entre reportes

Para evitar saturar el broker, el firmware impone un intervalo minimo de **200 ms** entre reportes consecutivos de deteccion (`SENSOR_REPORT_MS`). Si el PIR oscila mas rapido que este intervalo, los reportes intermedios se descartan.

---

## Comandos MQTT

### Topics de suscripcion (broker -> device)

| Topic | Payload | Efecto |
|-------|---------|--------|
| `<DEVICE_ID>/directive/powerState` | `ON` | Activa el sensor PIR. Comienza a reportar detecciones. Confirma publicando `report/powerState: ON`. |
| `<DEVICE_ID>/directive/powerState` | `OFF` | Desactiva el sensor PIR. Deja de reportar detecciones. Confirma publicando `report/powerState: OFF`. |
| `<DEVICE_ID>/#` | cualquier | El dispositivo se suscribe a todos los topics bajo su ID para futuras extensiones. Los mensajes recibidos se procesan segun el tipo de comando. |

### Ejemplos de comandos con mosquitto_pub

```bash
# Activar sensor
mosquitto_pub -h www.smartage.cl -p 1886 \
  -u luisHernandez -P luis123 \
  -t "6a7357a00221f201ddcc5f1d/directive/powerState" \
  -m "ON"

# Desactivar sensor
mosquitto_pub -h www.smartage.cl -p 1886 \
  -u luisHernandez -P luis123 \
  -t "6a7357a00221f201ddcc5f1d/directive/powerState" \
  -m "OFF"
```

Respuesta esperada en el Monitor Serial:
```
Sensor ON
```
o
```
Sensor OFF
```

---

## Indicador LED de Estado

El LED_BUILTIN (GPIO2, D4) es el LED azul integrado en la placa NodeMCU. **No indica movimiento**; solo refleja el estado de conectividad del sistema:

| Comportamiento del LED | Significado |
|------------------------|-------------|
| Parpadeo rapido (100ms ON/OFF) | Conectando a WiFi |
| Parpadeo medio (500ms ON/OFF) | WiFi conectado, reconectando MQTT |
| Parpadeo lento (pulso cada 20s) | Sistema OK: WiFi + MQTT conectados |
| Apagado (HIGH, logica invertida) | Estado normal entre pulsos de heartbeat |
| 3 destellos rapidos (50ms) | Mensaje MQTT recibido |

---

## Resiliencia y Recuperacion ante Fallos

El firmware implementa multiples capas de tolerancia a fallos, heredadas de la arquitectura **lp101_secure**:

### 1. Watchdog por Software

Un Ticker interno se reinicia en cada iteracion del `loop()`. Si el loop se bloquea por **180 segundos**, el watchdog fuerza `ESP.restart()`.

### 2. Ping Check

Cada **30 segundos**, el dispositivo intenta una conexion TCP a `8.8.8.8:53` (DNS de Google). Si falla **20 veces consecutivas** (equivalente a 10 minutos sin internet), se reinicia la placa.

### 3. Reconexion WiFi

Si `WiFi.status()` indica desconexion en cualquier momento, se reinicia el proceso de conexion con `WiFi.begin()` usando las credenciales de `secrets.h`. Tras **40 reintentos fallidos**, se aleatoriza la MAC y se reinicia.

### 4. Reconexion MQTT

Si el cliente MQTT se desconecta, se reintenta la conexion cada **10 segundos** con un timeout total de **2 minutos**. Si no se logra conectar en ese tiempo, se reinicia con nueva MAC.

### 5. Aleatorizacion de MAC (MAC Randomization)

Ante fallos persistentes (40 reintentos WiFi o 10 fallos consecutivos de publicacion MQTT), el firmware:

1. Guarda un flag en la memoria RTC del ESP8266 (`magic = 0xDEAD`, `failureFlag = 1`).
2. Reinicia la placa via `ESP.restart()`.
3. En el siguiente boot, detecta el flag y modifica el ultimo byte de la direccion MAC.
4. Conecta con la nueva MAC, evitando potenciales bloqueos del AP o del servidor DHCP.

---

## Ejemplos de Uso

### Ejemplo 1: Monitoreo con mosquitto_sub

```bash
# Suscribirse a todos los topics del dispositivo
mosquitto_sub -h www.smartage.cl -p 1886 \
  -u luisHernandez -P luis123 \
  -t "6a7357a00221f201ddcc5f1d/#" -v
```

**Salida esperada al detectar movimiento:**

```
6a7357a00221f201ddcc5f1d/report/online true
6a7357a00221f201ddcc5f1d/report/powerState ON
6a7357a00221f201ddcc5f1d/report/detectionState true
6a7357a00221f201ddcc5f1d/report/detectionState false
```

### Ejemplo 2: Salida esperada en Monitor Serial (115200 baud)

```
SensorPIR-8266 booted
  PIR: D2/GPIO4 | MQTT: report/detectionState, report/powerState
  Warmup: 65s...

Connecting to Nv2
................
Connected: Nv2
IP: 192.168.1.100
Connecting to MQTT...
connected
PIR ready
>>> MOTION DETECTED <<<
--- motion cleared ---
```

### Ejemplo 3: Integracion con Home Assistant

```yaml
# configuration.yaml
mqtt:
  binary_sensor:
    - name: "Sensor de Movimiento Sala"
      state_topic: "6a7357a00221f201ddcc5f1d/report/detectionState"
      payload_on: "true"
      payload_off: "false"
      device_class: motion
      availability_topic: "6a7357a00221f201ddcc5f1d/report/online"
      payload_available: "true"

  switch:
    - name: "Sensor PIR Power"
      state_topic: "6a7357a00221f201ddcc5f1d/report/powerState"
      command_topic: "6a7357a00221f201ddcc5f1d/directive/powerState"
      state_on: "ON"
      state_off: "OFF"
      payload_on: "ON"
      payload_off: "OFF"
      optimistic: false
```

### Ejemplo 4: Script Python para monitoreo

```python
import paho.mqtt.client as mqtt
import time

BROKER = "www.smartage.cl"
PORT = 1886
DEVICE_ID = "6a7357a00221f201ddcc5f1d"

def on_connect(client, userdata, flags, rc):
    client.subscribe(f"{DEVICE_ID}/#")
    print(f"Conectado. Suscrito a {DEVICE_ID}/#")

def on_message(client, userdata, msg):
    topic = msg.topic
    payload = msg.payload.decode()

    if "report/detectionState" in topic:
        if payload == "true":
            print(f">>> MOVIMIENTO DETECTADO <<<  [{time.ctime()}]")
        else:
            print(f"--- Movimiento finalizado ---  [{time.ctime()}]")
    elif "report/powerState" in topic:
        print(f"Estado del sensor: {payload}")
    elif "report/online" in topic:
        print(".", end="", flush=True)  # heartbeat silencioso

client = mqtt.Client()
client.username_pw_set("luisHernandez", "luis123")
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT, 60)
client.loop_forever()
```

### Ejemplo 5: Desactivar el sensor por la noche (cron job)

```bash
# En crontab:
# Desactivar sensor a las 23:00
0 23 * * * mosquitto_pub -h www.smartage.cl -p 1886 -u luisHernandez -P luis123 -t "6a7357a00221f201ddcc5f1d/directive/powerState" -m "OFF"

# Activar sensor a las 07:00
0 7 * * * mosquitto_pub -h www.smartage.cl -p 1886 -u luisHernandez -P luis123 -t "6a7357a00221f201ddcc5f1d/directive/powerState" -m "ON"
```

### Ejemplo 6: Monitoreo con MQTT Explorer

1. Abrir [MQTT Explorer](http://mqtt-explorer.com/) u otra herramienta MQTT.
2. Conectar al broker `www.smartage.cl:1886` con usuario `luisHernandez` / `luis123`.
3. Suscribirse al topic `6a7357a00221f201ddcc5f1d/#`.
4. Mover la mano frente al sensor.

---

## Calibracion del Sensor PIR

El HC-SR501 tiene **dos potenciometros** ajustables mediante destornillador:

### 1. Delay Time (Tiempo de retardo) -- potenciometro izquierdo

Controla cuanto tiempo permanece HIGH la salida tras detectar movimiento. Rango: ~5 segundos a ~300 segundos.

**IMPORTANTE para este proyecto**: El delay time del PIR debe estar **al minimo** (giro antihorario completo) para que el cese de movimiento se detecte con precision. Si el delay time es muy alto (>5s), el firmware reportara el cese con un retraso igual a ese valor.

### 2. Sensitivity (Sensibilidad) -- potenciometro derecho

Controla la distancia maxima de deteccion. Rango: ~3 metros a ~7 metros.

**Recomendacion para este proyecto**: al maximo (giro horario completo) para maxima cobertura.

### Procedimiento de calibracion

1. Conectar el dispositivo y esperar los 65 segundos de warm-up.
2. Con el Monitor Serial abierto, caminar frente al sensor.
3. Ajustar **Sensibilidad** segun necesidad: reducir si hay falsos positivos, aumentar si no detecta a la distancia deseada.
4. Verificar que tras dejar de moverse, el mensaje `detectionState: false` aparece rapidamente (confirmando que el delay time esta al minimo).
5. Si `detectionState: false` tarda mas de 5 segundos, reducir el delay time.

---

## Advertencias y Precauciones

### Electricas

| Advertencia | Detalle |
|-------------|---------|
| **NO alimentar el PIR con 3.3V** | El HC-SR501 requiere minimo 4.5V. Alimentarlo con 3.3V causa funcionamiento erratico o fallo total. Usar siempre VU (5V). |
| **NO conectar salida PIR directamente a 5V** | Aunque el PIR se alimenta con 5V, su salida OUT es de 3.3V y es segura para los GPIO del ESP8266. |
| **NO superar 500mA en VU** | El pin VU entrega el voltaje del USB (5V). Un puerto USB estandar entrega 500mA. El PIR consume < 50uA en reposo. |
| **Verificar polaridad** | Una conexion invertida VCC/GND puede danar permanentemente el sensor PIR. |

### Pines criticos del ESP8266

| Pin GPIO | Precaución |
|----------|-----------|
| GPIO0 (D3) | Debe estar HIGH durante el boot. El NodeMCU tiene pull-up interno, pero **no conectes nada que fuerce LOW**. |
| GPIO2 (D4) | Debe estar HIGH durante el boot. El LED_BUILTIN lo mantiene HIGH por defecto. |
| GPIO15 (D8) | Debe estar LOW durante el boot. El NodeMCU tiene pull-down interno. |
| GPIO1 (TX) y GPIO3 (RX) | Reservados para comunicacion Serial. No conectar sensores ni actuadores. |

### Operativas

| Advertencia | Detalle |
|-------------|---------|
| **Tiempo de warm-up** | El PIR necesita ~60s para estabilizarse. El firmware espera 65s. Durante este periodo no se detecta movimiento. |
| **Jumper del PIR en modo H** | Debe estar en modo **H (repetible)** para que el cese de movimiento se detecte correctamente. En modo L la salida solo pulsa una vez. |
| **Delay time del PIR** | El potenciometro de delay time debe estar al minimo para deteccion precisa del cese de movimiento. |
| **Device ID unico** | Si se despliegan multiples dispositivos, cada uno debe tener un `MQTT_CLIENT` diferente para evitar colisiones en el broker. |
| **Monitor Serial a 115200** | El baud rate debe ser exactamente 115200. Otros valores mostraran caracteres basura. |
| **Alimentacion estable** | Un cable USB de mala calidad o un puerto USB con ruido pueden causar reinicios aleatorios del ESP8266. |
| **Alcance WiFi** | El ESP8266 tiene antena integrada en PCB. En entornos con mucha interferencia o paredes gruesas, el alcance se reduce significativamente. |
| **Temperatura ambiente** | El PIR puede generar falsos positivos con cambios bruscos de temperatura ambiental (aire acondicionado, calefactores, corrientes de aire). |
| **Mascotas** | El PIR puede detectar mascotas dependiendo de su tamano y la sensibilidad configurada. |

### Credenciales

```cpp
// secrets.h contiene TODAS las credenciales:
// - WiFi: WIFI_SSID, WIFI_PASSWORD
// - MQTT: MQTT_BROKER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD, MQTT_CLIENT
// No versionar con valores reales en repositorios publicos.
```

---

## Estructura del Proyecto

```
SensorPIR-8266/
├── SensorPIR-8266.ino   # Sketch principal: setup, loop, PIR, WiFi, MQTT, watchdog
├── secrets.h             # Credenciales WiFi y MQTT
├── .gitignore            # Exclusiones para build artifacts y archivos sensibles
├── PLAN.md               # Plan de implementacion (referencia interna)
├── AUDIT_REPORT.md       # Informe de auditoria de codigo (referencia interna)
├── README.md             # Este documento
└── docs/
    ├── datasheet.md      # Documentacion de usuario detallada
    └── developer.md      # Documentacion para desarrolladores
```

---

## Licencia

Este proyecto se distribuye como software libre. Consultar los terminos especificos con el autor del proyecto.

---

**Documentacion generada el 2026-08-05. Version del firmware: basada en arquitectura lp101_secure.**
