# Ldr-8266 — Sensor de Luz LDR con NodeMCU ESP8266

Sensor de luminosidad basado en **fotorresistencia LDR** con divisor de voltaje. Publica el valor analógico crudo y cambios de estado (claro/oscuro) vía MQTT al broker `www.smartage.cl:1886` cada 5 segundos. WiFi directo a `Nv2`. Incluye **resiliencia lp101_secure** completa: watchdog 180s, ping check a 8.8.8.8, y MAC randomization.

---

## Componentes

| Componente | Cantidad | Nota |
|------------|----------|------|
| NodeMCU ESP8266 | 1 | ESP-12E, 4MB Flash |
| LDR (fotorresistencia) | 1 | 5mm estándar |
| Resistencia 8-10kΩ | 1 | Pull-down en divisor de voltaje |
| Cable micro USB | 1 | Alimentación + carga |

---

## Conexión física — Divisor de voltaje

```
   3.3V ──── LDR ────┬──── A0 (ADC0) NodeMCU
                     │
                  8-10kΩ
                     │
    GND ─────────────┘
```

| Componente | NodeMCU | Nota |
|------------|---------|------|
| LDR (extremo 1) | 3.3V | Alimentación |
| LDR (extremo 2) | A0 | Punto medio del divisor |
| Resistencia (extremo 1) | A0 | Mismo nodo que LDR |
| Resistencia (extremo 2) | GND | Pull-down |

**Funcionamiento del divisor:**
- **Luz intensa** → LDR baja resistencia (~1kΩ) → A0 lee voltaje alto (~3V → ADC >700)
- **Oscuridad** → LDR alta resistencia (~100kΩ+) → A0 lee voltaje bajo (~0.3V → ADC <200)
- **Penumbra** → valores intermedios (200-700)

---

## MQTT Topics

### Publicaciones (dispositivo → servidor)

| Topic | Payload | Frecuencia | Descripción |
|-------|---------|------------|-------------|
| `6a74e8165c21770c7132ca79/report/value` | `"512"` (0-1023) | Cada 5s | Valor crudo ADC |
| `6a74e8165c21770c7132ca79/report/lightState` | `"oscuro"` / `"claro"` | Al detectar | Cambio de estado |
| `6a74e8165c21770c7132ca79/report/powerState` | `"ON"` / `"OFF"` | Al cambiar | Estado del sensor |
| `6a74e8165c21770c7132ca79/report/online` | `"true"` | Al conectar | Heartbeat |

### Comandos (servidor → dispositivo)

| Topic | Payload | Acción |
|-------|---------|--------|
| `6a74e8165c21770c7132ca79/directive/powerState` | `"ON"` / `"OFF"` | Activa/desactiva |

---

## Arquitectura de resiliencia (lp101_secure)

| Mecanismo | Valor | Comportamiento |
|-----------|-------|----------------|
| **Watchdog software** | 180s | Si loop se bloquea >180s → `ESP.restart()` |
| **Ping check** | Cada 30s a 8.8.8.8:53 | 20 fallos consecutivos → reinicio total |
| **WiFi retry** | 40 intentos | Si no conecta en 40×250ms → reinicio con nueva MAC |
| **MQTT retry** | 10 fallos | 10 publicaciones fallidas consecutivas → reinicio con nueva MAC |
| **MAC randomization** | Último byte | Aleatoriza `mac[5]` entre 0x01-0xFE antes de reiniciar |
| **RTC memory** | Flag de fallo | Persiste entre reinicios |

---

## Detección de estados de luz

El firmware usa la misma lógica de detección que el AirSensor MQ135, adaptada para luz:

| Umbral | Significado |
|--------|-------------|
| ADC < 200 | Oscuridad detectada |
| ADC > 700 | Luz clara detectada |
| 600-900ms en oscuridad ×2 | Estado: **oscuro** |
| 800-1500ms en luz ×3 | Estado: **claro** |

---

## Credenciales (secrets.h)

```cpp
#define MQTT_BROKER    "www.smartage.cl"
#define MQTT_PORT      1886
#define MQTT_USERNAME  "luisHernandez"
#define MQTT_PASSWORD  "luis123"
#define MQTT_CLIENT    "6a74e8165c21770c7132ca79"
#define WIFI_SSID      "Nv2"
#define WIFI_PASSWORD  "12345678"
```

---

## Compilación y carga

```bash
arduino-cli core install esp8266:esp8266
arduino-cli lib install PubSubClient

arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 Ldr-8266.ino
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 Ldr-8266.ino
```

---

## Especificaciones técnicas

| Parámetro | Valor |
|-----------|-------|
| Placa | NodeMCU ESP8266 (ESP-12E) |
| MCU | Tensilica L106 32-bit, 80 MHz |
| Sensor | LDR (fotorresistencia) + divisor de voltaje |
| Resistencia pull-down | 8-10kΩ |
| Tipo señal | Analógica, 10-bit ADC (0-1023) |
| Flash usado | 254 KB / 1 MB (24%) |
| RAM usada | 29 KB / 80 KB (36%) |
| Frecuencia reporte | 5 segundos |
| MAC dispositivo | 84:f3:eb:7c:79:1a |

---

## Ejemplos de uso

```bash
# Monitorear nivel de luz
mosquitto_sub -h www.smartage.cl -p 1886 -u luisHernandez -P luis123 \
  -t "6a74e8165c21770c7132ca79/report/value"

# Ver cambios de estado
mosquitto_sub -h www.smartage.cl -p 1886 -u luisHernandez -P luis123 \
  -t "6a74e8165c21770c7132ca79/report/lightState"
```

---

## Estructura del proyecto

```
Ldr-8266/
├── Ldr-8266.ino    # Sketch principal
├── secrets.h        # Credenciales
├── .gitignore
└── README.md
```

---

## Notas

- El valor ADC con luz ambiente normal ronda 400-600.
- Cubrir completamente el LDR debería bajar la lectura a <100.
- La resistencia de 10kΩ es el valor recomendado. 8kΩ también funciona.
- La MAC del dispositivo es `84:f3:eb:7c:79:1a`.

**Repositorio privado.** Uso interno.
