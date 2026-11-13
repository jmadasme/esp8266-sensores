# AirSensor-8266 — Sensor de Calidad de Aire MQ135 con NodeMCU ESP8266

Sensor de calidad de aire basado en el **MQ135**. Detecta amoníaco (NH3), óxidos de nitrógeno (NOx), alcohol, benceno, humo y CO2. Publica lecturas analógicas y alertas vía MQTT al broker `www.smartage.cl:1886`. Conexión WiFi directa a red `Nv2`. Incluye **resiliencia lp101_secure**: watchdog, ping check y MAC randomization.

---

## Componentes

| Componente | Cantidad | Nota |
|------------|----------|------|
| NodeMCU ESP8266 | 1 | ESP-12E, 4MB Flash |
| Sensor MQ135 | 1 | Salida analógica AO |
| Cable micro USB | 1 | Alimentación + carga |

---

## Conexión física

```
MQ135                    NodeMCU 8266
┌──────────┐            ┌──────────────┐
│ VCC   ●──┼────────────┼──● VU (5V)   │  ← 5V del USB (obligatorio para calentador)
│ GND   ●──┼────────────┼──● GND       │
│ AO    ●──┼────────────┼──● A0 (ADC0) │  ← señal analógica
│ DO    ●──┼── (no usar)│              │
└──────────┘            └──────────────┘
```

> **IMPORTANTE**: El MQ135 requiere **5V** para el calentador interno. Usa **VU** (5V del USB). El pin 3.3V no es suficiente. El calentador tarda ~2 minutos en estabilizarse tras encender.

| MQ135 | NodeMCU | Nota |
|-------|---------|------|
| VCC | VU (5V) | Alimentación del calentador |
| GND | GND | Tierra común |
| AO | A0 (ADC0) | Lectura analógica (0-1023) |
| DO | — | No se usa |

---

## Principio de funcionamiento

El MQ135 varía su resistencia interna según la concentración de gases. A mayor concentración de gases contaminantes, **menor** es el valor leído en A0:

| Valor ADC | Significado |
|-----------|-------------|
| < 130 | Gas detectado (alta concentración) |
| 130 - 700 | Zona de transición |
| > 700 | Aire limpio (baja concentración) |

El firmware analiza el **tiempo** que el sensor permanece en estado de detección para clasificar:
- **Adecuado** (600-900ms): calidad de aire normal, fluctuación breve
- **Alerta** (800-1500ms): presencia sostenida de gases

---

## MQTT Topics

### Publicaciones (dispositivo → servidor)

| Topic | Payload | Frecuencia | Descripción |
|-------|---------|------------|-------------|
| `6a74bebb5c21770c7132ca76/report/value` | `"512"` (0-1023) | Cada 5s | Valor crudo del ADC |
| `6a74bebb5c21770c7132ca76/report/airState` | `"adecuado"` / `"alerta"` | Al detectar | Clasificación de calidad |
| `6a74bebb5c21770c7132ca76/report/powerState` | `"ON"` / `"OFF"` | Al cambiar | Estado del sensor |
| `6a74bebb5c21770c7132ca76/report/online` | `"true"` | Al conectar | Heartbeat |

### Comandos (servidor → dispositivo)

| Topic | Payload | Acción |
|-------|---------|--------|
| `6a74bebb5c21770c7132ca76/directive/powerState` | `"ON"` / `"OFF"` | Activa/desactiva |

---

## Credenciales (secrets.h)

```cpp
#define MQTT_BROKER "www.smartage.cl"
#define MQTT_PORT 1886
#define MQTT_USERNAME "luisHernandez"
#define MQTT_PASSWORD "luis123"
#define MQTT_CLIENT "6a74bebb5c21770c7132ca76"
#define WIFI_SSID "Nv2"
#define WIFI_PASSWORD "12345678"
```

---

## Compilación y carga

```bash
arduino-cli core install esp8266:esp8266
arduino-cli lib install PubSubClient
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 AirSensor-8266.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 AirSensor-8266.ino
```

---

## Arquitectura de resiliencia (lp101_secure)

| Mecanismo | Valor | Comportamiento |
|-----------|-------|----------------|
| **Watchdog software** | 180s | Si el loop principal no responde en 180s → `ESP.restart()` |
| **Ping check** | Cada 30s a 8.8.8.8:53 | 20 fallos consecutivos → reinicio total |
| **WiFi retry** | 40 intentos | Si no conecta en 40 ciclos de 250ms → reinicio con nueva MAC |
| **MQTT retry** | 10 fallos | 10 publicaciones fallidas consecutivas → reinicio con nueva MAC |
| **MAC randomization** | — | Aleatoriza el último byte de la MAC antes de reiniciar por fallos |
| **RTC memory** | Flag de fallo | Persiste entre reinicios para saber si debe aleatorizar MAC |

---

## Especificaciones técnicas

| Parámetro | Valor |
|-----------|-------|
| Placa | NodeMCU ESP8266 (ESP-12E) |
| Sensor | MQ135 |
| Gases detectados | NH3, NOx, alcohol, benceno, humo, CO2 |
| Tipo señal | Analógica (0-1023 en ADC 10-bit) |
| Voltaje operación | 5V DC |
| Calentador | 5V, ~800mW, 33Ω |
| Tiempo estabilización | ~2 minutos |
| Flash usado | 253 KB / 1 MB (24%) |
| RAM usada | 29 KB / 80 KB (36%) |
| Frecuencia reporte | 5 segundos |

---

## Estructura del proyecto

```
AirSensor-8266/
├── AirSensor-8266.ino    # Sketch principal
├── secrets.h              # Credenciales WiFi y MQTT
├── .gitignore
└── README.md
```

---

## Ejemplos de uso

```bash
# Monitorear valor crudo
mosquitto_sub -h www.smartage.cl -p 1886 -u luisHernandez -P luis123 \
  -t "6a74bebb5c21770c7132ca76/report/value"

# Monitorear alertas
mosquitto_sub -h www.smartage.cl -p 1886 -u luisHernandez -P luis123 \
  -t "6a74bebb5c21770c7132ca76/report/airState"
```

---

## Notas

- El calentador del MQ135 tarda ~2 minutos en alcanzar temperatura de operación. Las primeras lecturas no son fiables.
- En aire limpio, el valor ADC típico es >700.
- Para calibrar, exponer a aire limpio durante 5 minutos y anotar el valor base.
- La MAC del dispositivo: `d8:bf:c0:06:26:3e`

**Repositorio privado.** Uso interno.
