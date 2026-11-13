# Datasheet -- SensorPIR-8266

Documentacion de usuario del dispositivo IoT **SensorPIR-8266** basado en NodeMCU ESP8266 con sensor PIR HC-SR501, conexion WiFi directa, y protocolo MQTT para comunicacion bidireccional.

---

## 1. Descripcion del Producto

**SensorPIR-8266** es un sensor de movimiento inalambrico que detecta presencia humana mediante infrarrojos pasivos (PIR) y transmite alertas en tiempo real a traves del protocolo MQTT. Disenado para integracion en sistemas domoticos, monitoreo de seguridad y automatizacion industrial ligera.

### Caracteristicas principales

- Deteccion de movimiento por infrarrojos pasivos (PIR) con logica de flancos (edge detection)
- Alcance de deteccion: 3 a 7 metros (ajustable)
- Angulo de cobertura: ~110 grados (lente de Fresnel integrada)
- Conectividad WiFi 802.11 b/g/n 2.4 GHz
- Conexion WiFi directa con credenciales en `secrets.h` (sin dependencia de WiFiManager)
- Protocolo MQTT v3.1.1 para integracion IoT
- Comunicacion bidireccional: reportes de estado + comandos de control (ON/OFF)
- Heartbeat periodico cada 30 segundos
- Auto-recuperacion ante fallos de red
- Watchdog por software para maxima disponibilidad (180s timeout)
- LED indicador de estado del sistema (no de movimiento)
- Alimentacion por USB estandar (5V)
- Intervalo minimo entre reportes: 200 ms (anti-saturacion)
- MAC randomization ante fallos persistentes (evita bloqueos DHCP/AP)

---

## 2. Especificaciones Tecnicas

### 2.1 Placa de Control

| Parametro | Valor |
|-----------|-------|
| Modelo | NodeMCU ESP8266 (ESP-12E / LoLin V3) |
| Microcontrolador | Tensilica L106 32-bit RISC |
| Frecuencia de reloj | 80 MHz |
| Memoria Flash | 4 MB |
| RAM disponible (heap) | ~80 KB |
| WiFi | 802.11 b/g/n (2.4 GHz), WPA/WPA2 |
| Conexion WiFi | Directa via `WiFi.begin(SSID, PASSWORD)` |
| Voltaje de entrada | 5V DC (via micro-USB) |
| Voltaje logico GPIO | 3.3V |
| Corriente maxima GPIO | 12 mA por pin |
| Consumo en operacion | ~70 mA (WiFi activo) |
| Dimensiones | 49 x 25.5 x 13 mm |
| Temperatura operativa | -40degC a +125degC (microcontrolador) |
| Conector de programacion | micro-USB (CH340G USB-Serial) |

### 2.2 Sensor PIR HC-SR501

| Parametro | Valor |
|-----------|-------|
| Modelo | HC-SR501 |
| Tipo de sensor | Piroelectrico infrarrojo pasivo (PIR) |
| Chip controlador | BISS0001 |
| Voltaje de operacion | 4.5V -- 20V DC |
| Alimentacion en este proyecto | 5V (desde VU del NodeMCU) |
| Consumo en reposo | < 50 uA |
| Salida | Digital TTL (HIGH = 3.3V, LOW = 0V) |
| Tiempo de warm-up | ~60 segundos |
| Angulo de deteccion | < 110deg (cono) |
| Alcance de deteccion | 3 -- 7 m (ajustable) |
| Temperatura operativa | -20degC a +60degC |
| Dimensiones | 32 x 24 x 18 mm |
| Modo de operacion | H (repetible / retriggerable) |

### 2.3 Comunicacion

| Parametro | Valor |
|-----------|-------|
| Protocolo | MQTT v3.1.1 |
| Calidad de servicio (QoS) | 0 (fire-and-forget) |
| Retain | false |
| Broker predeterminado | `www.smartage.cl` |
| Puerto MQTT | 1886 (no estandar, no TLS) |
| Timeout de conexion MQTT | 120 segundos |
| Intervalo de heartbeat | 30 segundos |
| Device ID | `6a7357a00221f201ddcc5f1d` |
| Logica PIR | Edge detection (flancos LOW<->HIGH) |
| Intervalo minimo entre reportes | 200 ms (`SENSOR_REPORT_MS`) |
| Tasa de baudios Serial | 115200 bps |
| WiFi SSID | `Nv2` |
| Usuario MQTT | `luisHernandez` |

### 2.4 Condiciones Ambientales

| Parametro | Valor |
|-----------|-------|
| Temperatura operativa | 0degC a +40degC (recomendada para el PIR) |
| Humedad relativa | 20% -- 85% (sin condensacion) |
| Uso | Interior (el PIR no es apto para exteriores sin proteccion) |
| Altitud maxima | 2000 m |

---

## 3. Mapa de Pines y Conexiones

### 3.1 Tabla de Conexion

| Pin NodeMCU | GPIO | Modo | Pin PIR HC-SR501 | Color sugerido cable |
|-------------|------|------|-----------------|---------------------|
| VU (5V) | -- | PWR (5V) | VCC | Rojo |
| GND | -- | GND | GND | Negro |
| D2 | GPIO4 | INPUT | OUT | Amarillo o blanco |

### 3.2 Significado de los Pines del PIR

| Pin PIR | Funcion |
|---------|---------|
| VCC | Alimentacion positiva (4.5V -- 20V DC) |
| OUT | Salida digital (HIGH = movimiento, LOW = reposo) |
| GND | Tierra / negativo |

### 3.3 Configuracion de Jumpers y Potenciometros

El HC-SR501 tiene un jumper amarillo de 3 pines y dos potenciometros:

**Jumper de modo (trigger):**

```
Posicion H (jumper en pines 1-2): Modo REPETIBLE -- La salida se mantiene HIGH
                                   mientras hay movimiento continuo.
                                   <- USAR ESTA POSICION

Posicion L (jumper en pines 2-3): Modo NO REPETIBLE -- La salida pulsa HIGH
                                   una sola vez por deteccion.
```

**Potenciometros:**

| Control | Ajuste | Efecto |
|---------|--------|--------|
| Delay time (izquierdo) | Minimo (antihorario) | Output vuelve a LOW inmediatamente al cesar movimiento |
| Sensibilidad (derecho) | Maximo (horario) | Maximo alcance (~7m) |

### 3.4 Pines Reservados (NO USAR)

| Pin NodeMCU | GPIO | Razon |
|-------------|------|-------|
| D3 | GPIO0 | Debe estar HIGH en boot. Tiene pull-up interno. |
| D8 | GPIO15 | Debe estar LOW en boot. Tiene pull-down interno. |
| TX | GPIO1 | Reservado para Serial TX (115200 baud) |
| RX | GPIO3 | Reservado para Serial RX (115200 baud) |
| D0 | GPIO16 | Solo para wake-up de deep sleep. No usado. |

---

## 4. Instalacion y Configuracion

### 4.1 Pasos de Instalacion

1. **Conexion fisica**: Conectar los 3 cables entre el PIR y la NodeMCU segun la tabla de la seccion 3.1.
2. **Configurar jumper**: Verificar que el jumper del PIR esta en posicion **H**.
3. **Ajustar potenciometros**: Delay time al minimo (antihorario), sensibilidad al maximo (horario).
4. **Configurar credenciales**: Editar `secrets.h` con las credenciales WiFi y MQTT (ver seccion 4.2).
5. **Compilar y cargar firmware**: Ver [README.md](../README.md#compilacion-y-carga).
6. **Verificar funcionamiento**: Abrir Monitor Serial a 115200 baud y esperar ~65 segundos.

### 4.2 Configuracion de Credenciales (secrets.h)

Todas las credenciales se definen en el archivo `secrets.h`. Editar antes de compilar:

```cpp
// ─── WiFi ──────────────────────────────────────────────────
#define WIFI_SSID "Nv2"                  // SSID de la red WiFi
#define WIFI_PASSWORD "12345678"         // Contrasena de la red WiFi

// ─── MQTT ──────────────────────────────────────────────────
#define MQTT_BROKER "www.smartage.cl"    // Direccion del broker MQTT
#define MQTT_PORT 1886                    // Puerto MQTT
#define MQTT_USERNAME "luisHernandez"     // Usuario MQTT
#define MQTT_PASSWORD "luis123"           // Contrasena MQTT
#define MQTT_CLIENT "6a7357a00221f201ddcc5f1d"  // ID unico del dispositivo
```

**IMPORTANTE**:
- El `MQTT_CLIENT` debe ser **unico** para cada dispositivo conectado al mismo broker.
- Si se despliegan multiples SensorPIR-8266, asignar un `MQTT_CLIENT` diferente a cada uno.
- Para cambiar la red WiFi, editar `WIFI_SSID` y `WIFI_PASSWORD` y recompilar/cargar.

### 4.3 Verificacion Inicial

Tras cargar el firmware y alimentar, abrir el Monitor Serial a 115200 baud:

**Secuencia esperada de mensajes:**

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

Si algun paso falla, consultar la [Guia de Solucion de Problemas](#7-guia-de-solucion-de-problemas).

---

## 5. Operacion

### 5.1 Ciclo de Vida del Dispositivo

```
   ┌────────────────────────────────────────────────────────────┐
   │                                                            │
   │  ┌─────────┐    ┌──────────┐    ┌──────────┐    ┌───────┐ │
   │  │  BOOT   │───>│  WiFi    │───>│  MQTT    │───>│  PIR  │ │
   │  │         │    │  Connect │    │  Connect │    │ Warmup│ │
   │  │         │    │(directo) │    │          │    │ 65s   │ │
   │  └─────────┘    └──────────┘    └──────────┘    └───┬───┘ │
   │                                                      │     │
   │                                                      v     │
   │  ┌──────────────────────────────────────────────────────┐  │
   │  │              MONITORING (operacion normal)           │  │
   │  │                                                      │  │
   │  │   ┌──────┐  flanco LOW->HIGH  ┌────────┐           │  │
   │  │   │ IDLE │ ─────────────────> │ MOTION │           │  │
   │  │   └──────┘ <───────────────── └────────┘           │  │
   │  │            flanco HIGH->LOW                          │  │
   │  │                                                      │  │
   │  │   report/detectionState: true  (al detectar)        │  │
   │  │   report/detectionState: false (al cesar)           │  │
   │  │   Cada 30s: heartbeat report/online=true            │  │
   │  │   Cada 30s: ping check a 8.8.8.8                   │  │
   │  │                                                      │  │
   │  │   Comandos aceptados:                               │  │
   │  │   directive/powerState ON  -> activar sensor        │  │
   │  │   directive/powerState OFF -> desactivar sensor     │  │
   │  └──────────────────────────────────────────────────────┘  │
   │                                                            │
   └────────────────────────────────────────────────────────────┘
```

### 5.2 Estados del LED (D4 / GPIO2)

| Patron LED (D4) | Estado del Sistema |
|-----------------|-------------------|
| ON/OFF rapido (100ms) | Conectando a WiFi |
| ON/OFF medio (500ms) | WiFi OK, conectando/reconectando MQTT |
| ON/OFF lento (pulso 20ms cada 20s) | Sistema operativo normal (WiFi + MQTT OK) |
| 3 destellos rapidos (50ms) | Mensaje MQTT recibido |
| Apagado (HIGH) | Entre pulsos de heartbeat |

### 5.3 Mensajes MQTT Publicados

| Evento | Topic | Payload | Cuando se envia |
|--------|-------|---------|-----------------|
| Heartbeat | `<ID>/report/online` | `true` | Cada 30 segundos |
| Movimiento detectado | `<ID>/report/detectionState` | `true` | Al detectar flanco LOW->HIGH del PIR |
| Movimiento finalizado | `<ID>/report/detectionState` | `false` | Al detectar flanco HIGH->LOW del PIR |
| Sensor activado | `<ID>/report/powerState` | `ON` | Al arrancar o recibir comando ON |
| Sensor desactivado | `<ID>/report/powerState` | `OFF` | Al recibir comando OFF |

Donde `<ID>` = `6a7357a00221f201ddcc5f1d`.

### 5.4 Comandos MQTT Aceptados

| Comando | Topic | Payload | Efecto |
|---------|-------|---------|--------|
| Activar sensor | `<ID>/directive/powerState` | `ON` | El sensor comienza a detectar y reportar movimiento |
| Desactivar sensor | `<ID>/directive/powerState` | `OFF` | El sensor deja de reportar detecciones |

### 5.5 Logica de Deteccion del PIR

El firmware utiliza **deteccion de flancos simple** (edge detection):

- **Sin debounce por software**: la deteccion se basa en la transicion directa LOW<->HIGH del pin GPIO4. El sensor HC-SR501 ya incluye su propio circuito de estabilizacion y filtrado de senal (chip BISS0001).
- **Intervalo minimo entre reportes**: 200 ms (`SENSOR_REPORT_MS`). Si el sensor cambia de estado mas rapido que esto, los cambios intermedios se ignoran. Esto evita saturar el broker MQTT con reportes repetidos.
- **Sin re-publicacion durante movimiento continuo**: una vez reportado `detectionState: true`, no se vuelve a publicar hasta que el sensor retorne a LOW y luego vuelva a HIGH. Esto evita inundar el broker mientras hay movimiento sostenido.
- **Control ON/OFF**: si el sensor esta desactivado (`sensorOn == false`), no se publica ningun reporte de deteccion, independientemente de lo que indique el PIR.

### 5.6 Interpretacion de Errores en Serial

| Mensaje en Serial | Significado | Accion del sistema |
|-------------------|-------------|-------------------|
| `WiFi timeout. Rebooting...` | No se pudo conectar tras 40 reintentos | Reinicio con nueva MAC |
| `MQTT auth error. Check credentials/ClientId` (estado 5) | Credenciales MQTT invalidas o dispositivo ya conectado | Reintento automatico cada 10s |
| `MQTT timeout. Rebooting...` | No se pudo conectar al broker en 2 minutos | Reinicio con nueva MAC |
| `Too many MQTT failures. Rebooting...` | 10 publicaciones MQTT fallidas consecutivas | Reinicio con nueva MAC |
| `20 ping fails. Rebooting...` | Sin conectividad a internet por 10 minutos | Reinicio de placa |
| `Watchdog. Rebooting...` | Loop bloqueado por 180 segundos | Reinicio de placa |
| `Failure reboot detected. Randomizing MAC...` | Reinicio previo fue por fallo | MAC aleatorizada |

---

## 6. Integracion con Sistemas

### 6.1 Home Assistant

```yaml
# configuration.yaml
mqtt:
  binary_sensor:
    - name: "Sensor Movimiento Sala"
      state_topic: "6a7357a00221f201ddcc5f1d/report/detectionState"
      payload_on: "true"
      payload_off: "false"
      device_class: motion
      availability:
        - topic: "6a7357a00221f201ddcc5f1d/report/online"
          payload_available: "true"

  switch:
    - name: "PIR Sensor Power"
      state_topic: "6a7357a00221f201ddcc5f1d/report/powerState"
      command_topic: "6a7357a00221f201ddcc5f1d/directive/powerState"
      state_on: "ON"
      state_off: "OFF"
      payload_on: "ON"
      payload_off: "OFF"
      optimistic: false
```

### 6.2 Node-RED

```javascript
// Nodo MQTT In
// Topic: 6a7357a00221f201ddcc5f1d/#
// Broker: www.smartage.cl:1886
// Usuario: luisHernandez

// Funcion para procesar movimiento:
if (msg.topic.endsWith("/report/detectionState")) {
    if (msg.payload.toString() === "true") {
        node.warn("Movimiento detectado!");
    } else {
        node.warn("Movimiento finalizado");
    }
}

// Para enviar comando de apagado:
// Nodo MQTT Out
// Topic: 6a7357a00221f201ddcc5f1d/directive/powerState
// Payload: OFF
```

### 6.3 OpenHAB

```
// Archivo .items
Switch SensorMovimiento "Movimiento [%s]"
    {mqtt="<[broker:6a7357a00221f201ddcc5f1d/report/detectionState:state:true]"}
```

### 6.4 Script Python con Paho-MQTT

```python
import paho.mqtt.client as mqtt
import time

BROKER = "www.smartage.cl"
PORT = 1886
DEVICE_ID = "6a7357a00221f201ddcc5f1d"
last_heartbeat = 0

def on_connect(client, userdata, flags, rc):
    client.subscribe(f"{DEVICE_ID}/#")
    print(f"Conectado. Suscrito a {DEVICE_ID}/#")

def on_message(client, userdata, msg):
    global last_heartbeat
    topic = msg.topic
    payload = msg.payload.decode()

    if "report/online" in topic:
        last_heartbeat = time.time()
    elif "report/detectionState" in topic:
        if payload == "true":
            print(f"ALERTA: Movimiento detectado a las {time.ctime()}")
        else:
            print(f"Movimiento finalizado a las {time.ctime()}")
    elif "report/powerState" in topic:
        print(f"Estado del sensor: {payload}")

def desactivar_sensor(client):
    client.publish(f"{DEVICE_ID}/directive/powerState", "OFF")
    print("Comando OFF enviado")

def activar_sensor(client):
    client.publish(f"{DEVICE_ID}/directive/powerState", "ON")
    print("Comando ON enviado")

client = mqtt.Client()
client.username_pw_set("luisHernandez", "luis123")
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, PORT, 60)

# Ejemplo: desactivar y reactivar
import threading
threading.Timer(10, desactivar_sensor, args=[client]).start()
threading.Timer(20, activar_sensor, args=[client]).start()

client.loop_forever()
```

### 6.5 mosquitto_pub / mosquitto_sub

```bash
# Monitorear todos los mensajes del dispositivo
mosquitto_sub -h www.smartage.cl -p 1886 \
  -u luisHernandez -P luis123 \
  -t "6a7357a00221f201ddcc5f1d/#" -v

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

---

## 7. Guia de Solucion de Problemas

### Problema: El dispositivo no enciende

| Causa posible | Solucion |
|---------------|----------|
| Cable USB defectuoso | Probar con otro cable micro-USB |
| Puerto USB sin suficiente corriente | Conectar a puerto USB de PC (no hub sin alimentacion) |
| Placa danada | Verificar que el LED de power se enciende al conectar USB |

### Problema: No se conecta al WiFi

| Causa posible | Solucion |
|---------------|----------|
| SSID o contrasena incorrectos en `secrets.h` | Verificar `WIFI_SSID` y `WIFI_PASSWORD` en `secrets.h` y recompilar |
| Senal WiFi debil | Acercar la NodeMCU al router |
| DHCP no asigna IP | Verificar que el router tiene direcciones disponibles |
| Canal WiFi saturado | Cambiar canal en el router |
| AP no disponible | Verificar que el AP esta encendido y el SSID es visible |

### Problema: No se conecta al broker MQTT

| Causa posible | Solucion |
|---------------|----------|
| Broker caido | Verificar con MQTT Explorer que el broker esta activo en `www.smartage.cl:1886` |
| IP/Puerto incorrectos | Verificar `MQTT_BROKER` y `MQTT_PORT` en `secrets.h` |
| Firewall bloqueando puerto | Abrir puerto 1886 en el firewall del servidor |
| Credenciales incorrectas | Verificar `MQTT_USERNAME` y `MQTT_PASSWORD` en `secrets.h` |
| Device ID en uso | Otro dispositivo ya esta conectado con el mismo ID. Cambiar `MQTT_CLIENT` en `secrets.h` |

### Problema: No detecta movimiento

| Causa posible | Solucion |
|---------------|----------|
| Aun en warm-up (65s) | Esperar a que el Serial muestre "PIR ready" |
| Sensor desactivado (powerState OFF) | Enviar comando ON: `directive/powerState` = `ON` |
| Jumper en modo L | Cambiar jumper a posicion H |
| Sensibilidad muy baja | Ajustar potenciometro de sensibilidad (derecha, sentido horario = mas sensible) |
| PIR mal alimentado | Verificar que VCC recibe 5V desde VU (no 3.3V) |
| Cable OUT suelto | Verificar conexion del cable de senal entre PIR OUT y D2 |
| Sensor defectuoso | Probar el sensor con un LED + resistencia en OUT para verificar funcionamiento |

### Problema: Falsos positivos de movimiento

| Causa posible | Solucion |
|---------------|----------|
| Sensibilidad muy alta | Reducir sensibilidad (potenciometro hacia la izquierda) |
| Cambios bruscos de temperatura | Alejar el sensor de ventanas, aires acondicionados y calefactores |
| Interferencia electromagnetica | Alejar el sensor de motores, balastros y fuentes de EMI |
| Reflexiones IR | Cambiar orientacion; el PIR no debe apuntar a superficies reflectantes |

### Problema: No se detecta el cese de movimiento

| Causa posible | Solucion |
|---------------|----------|
| Delay time del PIR muy alto | Reducir delay time al minimo (potenciometro izquierdo, antihorario) |
| Jumper en modo L | Cambiar a modo H |
| PIR "ve" algo continuamente | Reorientar el sensor; verificar que no apunte a ventiladores, cortinas oscilantes, etc. |

### Problema: El sensor no responde a comandos ON/OFF

| Causa posible | Solucion |
|---------------|----------|
| Topic incorrecto | El comando debe enviarse a `<DEVICE_ID>/directive/powerState` (ej: `6a7357a00221f201ddcc5f1d/directive/powerState`) |
| Payload incorrecto | Debe ser exactamente `ON` o `OFF` (mayusculas) |
| Dispositivo desconectado de MQTT | Verificar que el LED muestra patron de heartbeat lento (conectado) |
| Suscripcion no activa | La suscripcion `6a7357a00221f201ddcc5f1d/#` se realiza al conectar MQTT |

### Problema: Reinicios frecuentes

| Causa posible | Solucion |
|---------------|----------|
| Fuente de alimentacion inestable | Cambiar cable USB; usar fuente de 5V de calidad |
| WiFi inestable | Verificar calidad de senal WiFi; acercar al router |
| Broker MQTT inestable | Verificar logs del broker; revisar conectividad de red |
| Ping check fallando | Verificar conectividad a internet desde la red local (20 fallos → reinicio) |

---

## 8. Mantenimiento

### Limpieza

- **Lente de Fresnel**: Limpiar periodicamente con un pano suave y seco. El polvo en la lente reduce la sensibilidad y puede causar falsos positivos.
- **Placa NodeMCU**: Mantener libre de polvo. No usar liquidos de limpieza.

### Actualizacion de Firmware

Para actualizar el firmware:

```bash
# Actualizar secrets.h si es necesario, luego:
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 .
```

El nuevo firmware se cargara reemplazando al anterior. La placa se reiniciara automaticamente.

**Nota**: Las credenciales en `secrets.h` forman parte del firmware compilado. Al cambiar de red WiFi o broker, se debe editar `secrets.h` y recompilar/cargar.

### Reemplazo del Sensor PIR

Si es necesario reemplazar el HC-SR501:

1. Desconectar alimentacion.
2. Desconectar los 3 cables del sensor antiguo.
3. Conectar el nuevo sensor respetando la misma asignacion de pines (VCC->VU, GND->GND, OUT->D2).
4. Verificar que el jumper del nuevo sensor esta en posicion H.
5. Ajustar potenciometros: delay al minimo, sensibilidad al maximo.
6. Conectar alimentacion y esperar el warm-up de 65 segundos.

### Cambio de red WiFi

Para cambiar la red WiFi a la que se conecta el dispositivo:

1. Editar `WIFI_SSID` y `WIFI_PASSWORD` en `secrets.h`.
2. Recompilar y cargar el firmware.
3. El dispositivo se conectara a la nueva red en el siguiente arranque.

---

## 9. Tabla de Compatibilidad con Otras Placas

| Placa | Compatible | Notas |
|-------|-----------|-------|
| **NodeMCU ESP8266 (ESP-12E)** | Si (nativa) | Placa objetivo del proyecto. FQBN: `esp8266:esp8266:nodemcuv2`. |
| **Wemos D1 Mini** | Si | Pin compatible. D2 = GPIO4, D4 = GPIO2. Usar FQBN `esp8266:esp8266:d1_mini`. |
| **Wemos D1 Mini Pro** | Si | Igual que D1 Mini. Antena externa mejora alcance WiFi. |
| **NodeMCU ESP8266 (LoLin V3)** | Si | Misma placa, distinto fabricante. Totalmente compatible. |
| **ESP-01 / ESP-01S** | No (sin modificaciones) | Solo tiene 2 GPIOs expuestos (GPIO0 y GPIO2), memoria flash limitada (512KB/1MB), requiere programador externo. |
| **ESP32 (cualquier variante)** | Requiere portabilidad | El codigo usa funciones especificas de ESP8266 (`ESP.rtcUserMemory*`, `wifi_set_macaddr`, `user_interface.h`). Necesitaria adaptacion. |
| **Arduino Uno + WiFi Shield** | No | Requeriria reescritura completa. |
| **Arduino Nano 33 IoT** | No (sin modificaciones) | Arquitectura ARM diferente. WiFi y MQTT usan otras librerias. |

### Notas de portabilidad a Wemos D1 Mini

```bash
arduino-cli compile --fqbn esp8266:esp8266:d1_mini .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:d1_mini .
```

La asignacion de pines es identica: D2 (GPIO4) para el PIR, D4 (GPIO2) para el LED.

---

## 10. Especificaciones Mecanicas (montaje sugerido)

```
Posicion recomendada de montaje:
  - Altura: 2.0 -- 2.5 metros del suelo
  - Angulo: ligeramente inclinado hacia abajo (~15deg)
  - Orientacion: lente de Fresnel hacia la zona a vigilar
  - Evitar: apuntar directamente a ventanas, fuentes de calor o corrientes de aire
```

---

## 11. Librerias Requeridas

### Compilacion

| Libreria | Autor | Version | Instalacion |
|----------|-------|---------|-------------|
| **ESP8266WiFi** | ESP8266 Core | built-in (3.1.2+) | Incluida con el core ESP8266 |
| **PubSubClient** | Nick O'Leary | 2.8.0 | `arduino-cli lib install "PubSubClient"` |
| **Ticker** | ESP8266 Core | built-in | Incluida con el core ESP8266 |
| **Arduino.h** | ESP8266 Core | built-in | Incluida con el core ESP8266 |
| **user_interface.h** | ESP8266 SDK | built-in | Incluida con el core ESP8266 |

### Instalacion completa de dependencias

```bash
# Core ESP8266
arduino-cli config add board_manager.additional_urls \
  https://arduino.esp8266.com/stable/package_esp8266com_index.json
arduino-cli core update-index
arduino-cli core install esp8266:esp8266

# Libreria externa (unica requerida)
arduino-cli lib install "PubSubClient"
```

---

**Documentacion de usuario -- SensorPIR-8266 v2.0**
Basado en arquitectura lp101_secure con conexion WiFi directa.
Fecha: 2026-08-05
