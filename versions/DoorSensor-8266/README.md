# DoorSensor-8266 — Sensor Magnético de Puerta MC-38 con NodeMCU ESP8266

Sensor de apertura/cierre de puerta utilizando un reed switch magnético **MC-38**. Publica el estado vía MQTT al broker `www.smartage.cl:1886` cada 500ms, reportando cambios de estado en tiempo real. Conexión WiFi directa a red `Nv2`.

---

## Componentes

| Componente | Cantidad | Nota |
|------------|----------|------|
| NodeMCU ESP8266 | 1 | ESP-12E, 4MB Flash |
| Sensor magnético MC-38 | 1 | Reed switch normalmente abierto (NO) |
| Cable micro USB | 1 | Alimentación + carga de firmware |

> **Sin resistencia externa necesaria.** Se usa el pull-up interno del ESP8266.

---

## Conexión física — Solo 2 cables

```
MC-38 Reed Switch        NodeMCU 8266
┌──────────────┐        ┌──────────────┐
│  ●───────────┼────────┼──● D2 (GPIO4) │  ← INPUT_PULLUP interno
│  ●───────────┼────────┼──● GND        │
└──────────────┘        └──────────────┘
```

| MC-38 | NodeMCU | Nota |
|-------|---------|------|
| Cable 1 | D2 (GPIO4) | Pull-up interno activado |
| Cable 2 | GND | Tierra común |

**Funcionamiento:**
- **Imán cerca** → reed se cierra → D2 a GND → `LOW` → `"true"` (cerrado)
- **Imán lejos** → reed se abre → pull-up → `HIGH` → `"false"` (abierto)

---

## MQTT Topics

### Publicaciones (dispositivo → servidor)

| Topic | Payload | Frecuencia | Descripción |
|-------|---------|------------|-------------|
| `6a73734f5c21770c7132ca70/report/lockedState` | `"true"` / `"false"` | 500ms (cada cambio) | Estado de la puerta |
| `6a73734f5c21770c7132ca70/report/powerState` | `"ON"` / `"OFF"` | Al cambiar | Estado del sensor |
| `6a73734f5c21770c7132ca70/report/online` | `"true"` | Al conectar | Heartbeat |

### Comandos (servidor → dispositivo)

| Topic | Payload | Acción |
|-------|---------|--------|
| `6a73734f5c21770c7132ca70/directive/powerState` | `"ON"` / `"OFF"` | Activa/desactiva reportes |
| `6a73734f5c21770c7132ca70/directive/lockedState` | `"true"` / `"false"` | Fuerza estado manual |

---

## Credenciales (secrets.h)

```cpp
#define MQTT_BROKER "www.smartage.cl"
#define MQTT_PORT 1886
#define MQTT_USERNAME "luisHernandez"
#define MQTT_PASSWORD "luis123"
#define MQTT_CLIENT "6a73734f5c21770c7132ca70"
#define WIFI_SSID "Nv2"
#define WIFI_PASSWORD "12345678"
```

---

## Compilación y carga

```bash
# Instalar dependencias
arduino-cli core install esp8266:esp8266
arduino-cli lib install PubSubClient

# Compilar
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 DoorSensor-8266.ino

# Subir
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 DoorSensor-8266.ino
```

---

## Estructura del proyecto

```
DoorSensor-8266/
├── DoorSensor-8266.ino    # Sketch principal (~80 líneas)
├── secrets.h              # Credenciales WiFi y MQTT
├── .gitignore
└── README.md
```

---

## Especificaciones técnicas

| Parámetro | Valor |
|-----------|-------|
| Placa | NodeMCU ESP8266 (ESP-12E) |
| MCU | Tensilica L106 32-bit, 80 MHz |
| Flash usado | 252 KB / 1 MB (24%) |
| RAM usada | 29 KB / 80 KB (36%) |
| WiFi | Directo a Nv2 |
| Sensor | MC-38 reed switch (NO) |
| Frecuencia lectura | 500 ms |
| MQTT | PubSubClient 2.8 |
| Debounce | No necesario (reed switch) |

---

## Ejemplos de uso

### Monitorear estado
```bash
mosquitto_sub -h www.smartage.cl -p 1886 -u luisHernandez -P luis123 \
  -t "6a73734f5c21770c7132ca70/report/lockedState"
```

### Forzar apertura
```bash
mosquitto_pub -h www.smartage.cl -p 1886 -u luisHernandez -P luis123 \
  -t "6a73734f5c21770c7132ca70/directive/lockedState" -m "false"
```

### Python
```python
import paho.mqtt.client as mqtt

def on_message(client, userdata, msg):
    state = "CERRADO" if msg.payload == b"true" else "ABIERTO"
    print(f"Puerta: {state}")

client = mqtt.Client()
client.username_pw_set("luisHernandez", "luis123")
client.on_message = on_message
client.connect("www.smartage.cl", 1886)
client.subscribe("6a73734f5c21770c7132ca70/report/lockedState")
client.loop_forever()
```

---

## Notas

- El reed switch MC-38 es **normalmente abierto (NO)**. Con imán cerca se cierra.
- No requiere resistencia externa — el ESP8266 tiene pull-up interno en GPIO4.
- Si se invierte el comportamiento (imán cerca = abierto), intercambiar los dos cables del reed.
- La MAC del dispositivo visible en la red WiFi es `e0:98:06:23:ba:e9`.

---

**Repositorio privado.** Uso interno.
