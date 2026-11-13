# TempSensor-8266 — Sensor de Temperatura DS18B20 con NodeMCU ESP8266

Sensor de temperatura digital de alta precisión basado en el **DS18B20** (protocolo OneWire). Publica lecturas de temperatura vía MQTT al broker `www.smartage.cl:1886` cada 60 segundos, con tres mecanismos de supervisor que impiden que el equipo quede colgado o deje de reportar.

> **El DS18B20 solo mide temperatura.** No existe sensor de humedad en este proyecto.
> El topic `report/humidity` se publica con el valor explícito `"N/A"` para mantener el contrato
> con los consumidores que esperan ese topic. Si necesitas humedad real, requiere otro sensor
> (DHT22/AM2302, BME280, SHT31) — no es un cambio de firmware, es otro componente de hardware.

---

## Componentes necesarios

| Componente | Cantidad | Nota |
|------------|----------|------|
| NodeMCU ESP8266 | 1 | ESP-12E, 4MB Flash |
| DS18B20 | 1 | Sensor de temperatura digital OneWire |
| Resistencia 4.7kΩ | 1 | Pull-up entre DATA y VDD |
| Cable micro USB | 1 | Alimentación y carga de firmware |
| Protoboard + cables | — | Para montaje |

---

## Conexión física

```
                     ┌──────────────────────┐
                     │     NodeMCU 8266      │
                     │                      │
     DS18B20         │  3.3V ●──────────────┼──┐
    ┌────────┐       │                      │  │
    │ VDD ●──┼───────┼──────────────────────┼──┤
    │ DATA ●─┼──┐    │  D2 (GPIO4) ●────────┼──┤ 4.7kΩ
    │ GND ●──┼──┼────┼──● GND               │  │
    └────────┘  │    │                      │  │
                └────┼── 4.7kΩ pull-up ─────┼──┘
                     └──────────────────────┘
```

**Tabla de conexión resumida:**

| DS18B20 (pata) | Color típico | NodeMCU | Nota |
|----------------|-------------|---------|------|
| VDD | Rojo | 3.3V | También funciona con VU (5V) |
| DATA | Amarillo | D2 (GPIO4) | Señal digital OneWire |
| GND | Negro | GND | Tierra común |
| Resistencia 4.7kΩ | — | Entre DATA y VDD | Obligatorio: pull-up |

---

## Tabla de pines completa del NodeMCU

| Pin NodeMCU | GPIO | Uso | Nota |
|-------------|------|-----|------|
| D2 | GPIO4 | DS18B20 DATA | OneWire |
| D4 | GPIO2 | LED_BUILTIN | Indicador estado sistema (activo bajo) |
| VU | — | 5V (USB) | Alternativa para VDD del sensor |
| 3.3V | — | 3.3V | VDD del sensor (recomendado) |
| GND | — | Tierra | GND del sensor |
| D0 | GPIO16 | Reservado | Deep sleep wake (no usar) |
| D3 | GPIO0 | Reservado | Pin de boot — no usar |
| D8 | GPIO15 | Reservado | Pin de boot — no usar |
| TX | GPIO1 | Serial TX | No conectar |
| RX | GPIO3 | Serial RX | No conectar |

---

## MQTT Topics

Todos los topics cuelgan del ClientId del dispositivo, `<CLIENT_ID>`. La tabla usa
`<CLIENT_ID>` como marcador; el valor real vive en `secrets.h` (no versionado).

### Publicaciones (dispositivo → servidor)

| Topic | Payload | Frecuencia | Descripción |
|-------|---------|------------|-------------|
| `<CLIENT_ID>/report/temperature` | `"18"` | 60s | Temperatura en °C, **entero**. Formato legacy, compatible con consumidores previos |
| `<CLIENT_ID>/report/temperatureDetail` | JSON | 60s | Detalle: `t_c`, `t_f`, `rssi`, `heap`, `uptime` |
| `<CLIENT_ID>/report/sensorStatus` | `"OK"` / `"DISCONNECTED"` / `"CONVERSION_TIMEOUT"` | 60s | Salud de la lectura del DS18B20 |
| `<CLIENT_ID>/report/humidity` | `"N/A"` | Al conectar y al activar | El DS18B20 no mide humedad |
| `<CLIENT_ID>/report/online` | `"true"` | 60s | Heartbeat de conectividad |
| `<CLIENT_ID>/report/powerState` | `"ON"` / `"OFF"` | Al cambiar | Estado del sensor |

### Comandos (servidor → dispositivo)

| Topic | Payload | Acción |
|-------|---------|--------|
| `<CLIENT_ID>/directive/powerState` | `"ON"` | Activa reportes de temperatura y publica el estado |
| `<CLIENT_ID>/directive/powerState` | `"OFF"` | Desactiva reportes de temperatura |

El dispositivo se suscribe a `<CLIENT_ID>/#`, así que cualquier topic nuevo bajo el ClientId
llega al callback de mensajes.

---

## Instalación y carga

### 1. Dependencias

```bash
# Core ESP8266
arduino-cli core install esp8266:esp8266@3.1.2

# Librerías
arduino-cli lib install PubSubClient OneWire DallasTemperature
```

### 2. Configurar credenciales

`secrets.h` **no está versionado**. Partir de la plantilla:

```bash
cp secrets.example.h secrets.h
$EDITOR secrets.h
```

```cpp
#define MQTT_BROKER   "www.smartage.cl"
#define MQTT_PORT     1886
#define MQTT_USERNAME "TU_USUARIO"
#define MQTT_PASSWORD "TU_PASSWORD"
#define MQTT_CLIENT   "TU_CLIENT_ID"
#define WIFI_SSID     "TU_RED_WIFI"
#define WIFI_PASSWORD "TU_PASSWORD_WIFI"
```

> **Este broker exige las tres cosas**: usuario, contraseña **y ClientId registrado**.
> Si el ClientId no está dado de alta en el panel SmartAge, la respuesta es
> `CONNACK 5 — not authorised` aunque usuario y contraseña sean correctos. Lo mismo
> ocurre con credenciales inválidas, así que un rechazo de autenticación no
> distingue entre las dos causas.

### 3. Compilar y cargar

```bash
./scripts/detectar.sh      # identifica el puerto serie de la placa
./scripts/subir.sh         # compila y sube (usa el puerto detectado)
```

Equivalente manual — **nótese que se compila apuntando a la carpeta del sketch, no al `.ino`**:

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 .
arduino-cli upload  -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 .
```

> `arduino-cli upload` **no recompila**. Si se sube sin compilar antes, se transfiere el
> binario viejo y el cambio no se ve en la placa.

### 4. Verificar funcionamiento

```bash
./scripts/monitor.sh /dev/ttyUSB0 60
```

Salida esperada (los primeros bytes son basura del driver USB, es normal):

```
DS18B20 sensors found: 1
Connected: Nv2   IP: 192.168.43.47   RSSI: -42 dBm   Canal: 6
connected
Subscribed to <client>/#
TempSensor-8266 v2.2 booted
  Sensor: DS18B20 on D2/GPIO4 (temperatura ONLY, sin humedad)
  Report: report/temperature every 60s
  WDT: 8s | Ping ICMP 8.8.8.8 cada 30s, 5 intentos, reinicio si no hay reporte en 300s
Temperature: 18C (17.81C) OK
Ping 8.8.8.8: OK (1 respuesta(s))
Temperature: 18C (17.81C) OK
```

---

## Ejemplos de uso

### Monitoreo por línea de comandos (mosquitto)

```bash
# Ver temperatura (formato entero legacy)
mosquitto_sub -h www.smartage.cl -p 1886 -u TU_USUARIO -P TU_PASSWORD \
  -t "<CLIENT_ID>/report/temperature" -v

# Ver todo, incluido el detalle JSON
mosquitto_sub -h www.smartage.cl -p 1886 -u TU_USUARIO -P TU_PASSWORD \
  -t "<CLIENT_ID>/#" -v

# Activar / desactivar sensor
mosquitto_pub -h www.smartage.cl -p 1886 -u TU_USUARIO -P TU_PASSWORD \
  -t "<CLIENT_ID>/directive/powerState" -m "ON"

mosquitto_pub -h www.smartage.cl -p 1886 -u TU_USUARIO -P TU_PASSWORD \
  -t "<CLIENT_ID>/directive/powerState" -m "OFF"
```

### Script Python con paho-mqtt

```python
import json
from datetime import datetime
import paho.mqtt.client as mqtt

BROKER = "www.smartage.cl"
PORT = 1886
USER = "TU_USUARIO"
PASSWORD = "[REDACTED]"
CLIENT_ID = "<CLIENT_ID>"

TOPIC_LEGACY = f"{CLIENT_ID}/report/temperature"          # entero °C
TOPIC_DETAIL = f"{CLIENT_ID}/report/temperatureDetail"    # JSON
TOPIC_STATUS = f"{CLIENT_ID}/report/sensorStatus"


def on_connect(client, userdata, flags, rc):
    print(f"Conectado al broker (rc={rc})")
    client.subscribe(f"{CLIENT_ID}/#")


def on_message(client, userdata, msg):
    ahora = datetime.now().strftime("%H:%M:%S")
    if msg.topic == TOPIC_LEGACY:
        print(f"[{ahora}] Temperatura: {msg.payload.decode()}°C")
    elif msg.topic == TOPIC_DETAIL:
        d = json.loads(msg.payload.decode())
        print(f"[{ahora}] {d['t_c']}°C ({d['t_f']}°F) "
              f"rssi={d['rssi']} heap={d['heap']} up={d['uptime']}s")
    elif msg.topic == TOPIC_STATUS:
        print(f"[{ahora}] Estado sensor: {msg.payload.decode()}")


client = mqtt.Client()
client.username_pw_set(USER, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT)
client.loop_forever()
```

### Home Assistant

```yaml
sensor:
  - platform: mqtt
    name: "Temperatura DS18B20"
    state_topic: "<CLIENT_ID>/report/temperature"
    unit_of_measurement: "°C"
    device_class: temperature

    # Estado de salud del sensor
    availability_topic: "<CLIENT_ID>/report/online"
    payload_available: "true"
    payload_not_available: "false"
```

---

## Arquitectura de resiliencia

El objetivo de diseño es explícito: **el equipo no debe quedarse colgado ni dejar de
reportar**. Hay tres mecanismos independientes, en capas, porque cada uno cubre un fallo
distinto.

| # | Mecanismo | Ventana | Comportamiento |
|---|-----------|---------|----------------|
| 1 | **Watchdog de hardware** | `ESP.wdtEnable(WDTO_8S)` | Es el máximo del ESP8266. Si el loop principal se cuelga, el chip se reinicia solo. Se alimenta explícitamente en `loop()`, en el bucle de conexión WiFi, en `brokerReachable()` y en los `delay()` largos de `startMqtt()` |
| 2 | **Ping ICMP a 8.8.8.8** | Cada 30s, 5 probes | Ronda de ICMP real (no TCP:53) con 5 intentos separados por 2s. Si responde al menos 1 → OK |
| 2b | **Respaldo: broker** | Solo si falla el ICMP | Si el ICMP no responde, se comprueba que el broker MQTT acepte conexión TCP. Si el broker responde pero el ICMP no, se asume que la red local filtra ICMP hacia internet y **no se reinicia** |
| 2c | **Reinicio** | Al confirmar el fallo | Si ICMP **y** broker fallan → reinicio con nueva MAC. La nueva MAC evade la tabla ARP del router |
| 3 | **Guard de "sin reportar"** | 300s | Si el sensor está `ON` y pasan 5 minutos sin una publicación MQTT exitosa → reinicio. Cubre el caso "el equipo está vivo, conectado al WiFi, pero sin publicar" |

**WiFi:** hasta `MAX_WIFI_RETRIES` (120) intentos antes de reiniciar con nueva MAC — el valor
anterior (40 ≈ 10s) era insuficiente después de un flasheo, cuando el DHCP puede tardar.

**Por qué 3 capas:** el watchdog de hardware cubre cuelgues de CPU, pero no un stack MQTT
sano que simplemente no publica. El ping cubre "hay internet", pero una red puede filtrar
ICMP legítimamente. El guard de reporte es la red de seguridad que no depende de ninguna de
las dos anteriores.

### Detalle de implementación del ping

El SDK del ESP8266 expone `ping_start()` desde `ping.h` (no existe `WiFi.ping()`). Dos
trampas importantes, ambas resueltas en el firmware:

- `struct ping_option` **debe vivir en memoria global**, no en el stack: el SDK conserva el
  puntero y lo usa desde su propia tarea.
- `ping_start()` **no se puede relanzar dentro del mismo boot** (el SDK no libera su pbuf
  interno). Por eso cada ronda hace **una sola llamada** con `count = 5` y deja que el SDK
  encadena los 5 probes, en vez de llamar 5 veces.

---

## Estructura del proyecto

```
TempSensor-8266/
├── TempSensor-8266.ino       # Sketch principal (581 líneas, v2.2)
├── secrets.example.h         # Plantilla de credenciales (versionada)
├── secrets.h                 # Credenciales reales — NO versionar
├── scripts/
│   ├── detectar.sh            # Detecta el puerto serie de la placa
│   ├── compilar.sh            # Compila a build/ y muestra uso de memoria
│   ├── subir.sh               # Compila y transfiere el firmware
│   └── monitor.sh             # Consola serie 115200
├── .gitignore
├── README.md
└── docs/
    ├── datasheet.md           # Documentación de usuario
    └── developer.md           # Documentación para desarrolladores
```

---

## Especificaciones técnicas

| Parámetro | Valor |
|-----------|-------|
| **Placa** | NodeMCU ESP8266 (ESP-12E) |
| **MCU** | Tensilica L106 32-bit RISC, 80 MHz |
| **Flash** | 4 MB (267 KB usado — 25%) |
| **RAM** | 80 KB (29.5 KB usados — 36%) |
| **IRAM** | 64 KB (60.4 KB usados — 92%, baseline de ESP8266 + WiFi + PubSubClient) |
| **WiFi** | 802.11 b/g/n 2.4 GHz |
| **Sensor** | DS18B20 (Maxim Integrated) |
| **Protocolo sensor** | OneWire (1-Wire) |
| **Rango temperatura** | -55 °C a +125 °C |
| **Precisión** | ±0.5 °C (-10 °C a +85 °C) |
| **Resolución** | 12 bits (0.0625 °C); se reporta redondeado a entero |
| **Intervalo reporte** | 60 segundos |
| **Consumo** | ~70 mA (WiFi activo, modo normal) |

---

## Advertencias y troubleshooting

| Problema | Causa probable | Solución |
|----------|---------------|----------|
| `DS18B20 sensors found: 0` | Sensor no conectado o sin resistencia | Verificar conexión y resistencia 4.7kΩ entre DATA y VDD |
| `sensorStatus = CONVERSION_TIMEOUT` | La conversión OneWire no terminó a tiempo | Verificar calidad del cableado; el cable largo sobre el pull-up degrada la señal |
| `MQTT auth error` en bucle | ClientId no registrado **o** usuario/contraseña incorrectos | Verificar las tres cosas contra el panel SmartAge. Probar desde la línea de comandos con `mosquitto_pub` para aislar si el problema es el firmware |
| `panicked at core_esp8266_main.cpp:191` | `yield()` invocado desde contexto de interrupción (Ticker) | No llamar funciones que usen `delay`/`OneWire` desde un `Ticker`. El scheduler del sensor corre en `loop()` con `millis()` |
| Sin reportes y el LED no parpadea | Reinicio en bucle por fallo de conectividad | Revisar el log serie: si `Ping ...: FALLO` y `Broker MQTT responde: no` seguidos de reinicio, es pérdida de red real |
| `connecting to <ssid>` en bucle | Credenciales WiFi incorrectas o AP apagado | `arduino-cli` no detecta esto; revisar `WIFI_SSID`/`WIFI_PASSWORD` y que el AP esté encendido |
| El cambio no aparece en la placa | Se subió sin recompilar | `arduino-cli upload` no compila. Ejecutar `scripts/compilar.sh` primero |

---

## Licencia

Proyecto privado. Uso interno.
