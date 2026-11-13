# Documentacion para Desarrolladores -- SensorPIR-8266

Documentacion tecnica del codigo fuente, arquitectura y guia de extension para el firmware **SensorPIR-8266**.

---

## Tabla de Contenidos

- [Arquitectura del Codigo](#arquitectura-del-codigo)
- [Diagrama de Flujo de la Logica Principal](#diagrama-de-flujo-de-la-logica-principal)
- [Logica del Sensor PIR (Edge Detection)](#logica-del-sensor-pir-edge-detection)
- [API de Funciones](#api-de-funciones)
- [Estructura de Datos](#estructura-de-datos)
- [Constantes de Configuracion](#constantes-de-configuracion)
- [Librerias y Dependencias](#librerias-y-dependencias)
- [Protocolo MQTT: Topics y Payloads](#protocolo-mqtt-topics-y-payloads)
- [Mecanismos de Resiliencia](#mecanismos-de-resiliencia)
- [Guia de Modificacion y Extension](#guia-de-modificacion-y-extension)
- [Como Agregar Nuevos Sensores/Actuadores](#como-agregar-nuevos-sensoresactuadores)
- [Notas de Memoria y Rendimiento](#notas-de-memoria-y-rendimiento)
- [Checklist de Seguridad para Pull Requests](#checklist-de-seguridad-para-pull-requests)

---

## Arquitectura del Codigo

### Estructura General

El firmware sigue el patron arquitectonico de **lp101_secure**, organizado en un unico sketch (`.ino`) con funciones agrupadas por responsabilidad:

```
SensorPIR-8266.ino  (345 lineas)
│
├── [1] Includes y cabeceras
│   ├── Librerias del sistema: <ESP8266WiFi.h>, <PubSubClient.h>, <Ticker.h>, <Arduino.h>
│   ├── secrets.h (credenciales WiFi + MQTT)
│   └── <user_interface.h> (API de bajo nivel ESP8266 para MAC randomization)
│
├── [2] Definiciones de pines y constantes de timing
│   ├── PIR_PIN = 4 (GPIO4 / D2)
│   ├── LED_PIN = 2 (GPIO2 / D4, LED_BUILTIN)
│   ├── PIR_WARMUP_MS = 65000
│   ├── SENSOR_REPORT_MS = 200
│   ├── REPORT_INTERVAL = 30
│   ├── PING_INTERVAL = 30
│   ├── WDT_TIMEOUT = 180
│   ├── MAX_WIFI_RETRIES = 40
│   ├── MAX_MQTT_FAILURES = 10
│   └── MAX_PING_FAILS = 20
│
├── [3] Objetos globales
│   ├── WiFiClient espClient
│   ├── PubSubClient client(espClient)
│   ├── Ticker blinkStatus
│   ├── Ticker blinkHeartbeat (20s blink cuando MQTT OK)
│   ├── Ticker sendUpdate (30s heartbeat MQTT)
│   ├── Ticker pingCheck (30s ping check)
│   └── Ticker wdtTicker (180s watchdog)
│
├── [4] Variables de estado
│   ├── Contadores de fallos: mqttPublishFailCount, wifiConnectRetries, pingFailCount
│   ├── Flags volatiles: shouldPing, wdtTriggered
│   ├── Estado PIR: sensorOn, sensorTriggered, sensorReportSent, pirReady
│   └── Timestamp: warmupStart
│
├── [5] Estructura RTC rtcData_t (magic=0xDEAD + failureFlag)
│
├── [6] Declaraciones forward de funciones
│
├── [7] ISR helpers (IRAM_ATTR): setPingFlag(), wdtCallback()
│
├── [8] SETUP()
│   ├── pinMode config
│   ├── RTC memory check (MAC randomization tras fallo previo)
│   ├── startWifi()    ← WiFi.begin(WIFI_SSID, WIFI_PASSWORD)
│   ├── startMqtt()    ← client.connect + subscribe + report/online
│   ├── Tickers: pingCheck y watchdog
│   └── warmupStart = millis()
│
├── [9] LOOP()
│   ├── wdtTriggered? → ESP.restart()
│   ├── feedWatchdog()
│   ├── shouldPing? → checkInternetConnectivity()
│   ├── WiFi.status()? → startWifi()
│   ├── checkMqtt() → startMqtt() si desconectado
│   ├── checkSensor()
│   └── client.loop()
│
└── [10] Funciones auxiliares
    ├── PIR: checkSensor(), sendReport(), turnOn(), turnOff()
    ├── MQTT: callback(), splitTopic(), startMqtt(), checkMqtt(), sendToBroker(), sendOnlineReport()
    ├── WiFi: startWifi()
    ├── LED: blinkLed(), blinkMessageReceived()
    ├── Resiliencia: feedWatchdog(), checkInternetConnectivity()
    └── MAC: randomizeMac(), rebootWithNewMac()
```

### Patron de Diseno

El firmware usa un **super-loop con Tickers**:
- `loop()` itera lo mas rapido posible sin delays bloqueantes (excepto en reconexiones).
- Las tareas periodicas (heartbeat, ping, watchdog) se manejan con `Ticker` (timers por hardware).
- La lectura del PIR se hace por polling con `digitalRead()` en cada iteracion via `checkSensor()`.
- Las esperas durante reconexion WiFi usan `delay(250)` con `feedWatchdog()` para mantener vivo el watchdog.
- Las esperas durante reconexion MQTT usan `delay(10000)` con `feedWatchdog()`.
- La conexion WiFi es directa: `WiFi.begin(WIFI_SSID, WIFI_PASSWORD)` con credenciales hardcodeadas en `secrets.h`.

---

## Diagrama de Flujo de la Logica Principal

### Flujo de SETUP (lineas 81-112)

```
SETUP()
  │
  ├─ pinMode(PIR_PIN, INPUT)        // GPIO4/D2
  ├─ pinMode(LED_PIN, OUTPUT)       // GPIO2/D4
  ├─ digitalWrite(LED_PIN, HIGH)    // LED off (active low)
  ├─ Serial.begin(115200)
  ├─ randomSeed(ESP.getCycleCount())
  │
  ├─ Leer RTC memory (offset 0)
  │   ├─ ¿rtc.magic == 0xDEAD && rtc.failureFlag == 1?
  │   │   ├─ SI -> randomizeMac()
  │   │   │       ├─ WiFi.disconnect()
  │   │   │       ├─ Obtener MAC actual
  │   │   │       ├─ mac[5] = random(0x01, 0xFE)
  │   │   │       └─ wifi_set_macaddr(STATION_IF, mac)
  │   │   └─ Limpiar failureFlag en RTC
  │   └─ NO -> continuar
  │
  ├─ Inicializar contadores de fallos a 0
  │
  ├─ startWifi()
  │   ├─ blinkStatus.attach(0.1, blinkLed)   // parpadeo rapido 100ms
  │   ├─ WiFi.persistent(false)
  │   ├─ WiFi.mode(WIFI_STA)
  │   ├─ WiFi.begin(WIFI_SSID, WIFI_PASSWORD)
  │   ├─ Bucle: delay(250) + feedWatchdog()
  │   │   └─ ¿wifiConnectRetries >= MAX_WIFI_RETRIES (40)?
  │   │       ├─ SI -> rebootWithNewMac()
  │   │       └─ NO -> reintentar
  │   └─ Al conectar: imprime SSID e IP
  │       LED: blinkStatus.attach(0.5, blinkLed) // 500ms
  │
  ├─ startMqtt()
  │   ├─ client.setServer(MQTT_BROKER, MQTT_PORT)
  │   ├─ client.setCallback(callback)
  │   ├─ Bucle de conexion:
  │   │   ├─ client.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD)
  │   │   ├─ ¿Exito?
  │   │   │   ├─ SI -> mqttPublishFailCount = 0
  │   │   │   │      LED: blinkStatus.attach(20, blinkLed) -> heartbeat lento
  │   │   │   │      blinkHeartbeat.attach(20, blinkLed)
  │   │   │   │      sendUpdate.attach(30, sendOnlineReport)
  │   │   │   └─ NO -> ¿client.state() == 5? → "MQTT auth error"
  │   │   │            feedWatchdog(); delay(10000)
  │   │   └─ ¿Timeout 120s? -> rebootWithNewMac()
  │   ├─ Suscripcion: sprintf(sub, "%s/#", MQTT_CLIENT)
  │   │                client.subscribe(sub)
  │   └─ sendToBroker("report/online", "true")
  │
  ├─ pingCheck.attach(PING_INTERVAL, setPingFlag)   // 30s
  ├─ feedWatchdog()
  ├─ warmupStart = millis()
  └─ Serial: "SensorPIR-8266 booted" + info de config
```

### Flujo de LOOP (lineas 115-123)

```
LOOP()  <- se ejecuta continuamente
  │
  ├─ ¿wdtTriggered?
  │   └─ SI -> "Watchdog. Rebooting..." -> ESP.restart()
  │
  ├─ feedWatchdog()  // wdtTicker.once(WDT_TIMEOUT, wdtCallback) → 180s
  │
  ├─ ¿shouldPing? (ISR setPingFlag cada 30s)
  │   └─ SI -> shouldPing = false -> checkInternetConnectivity()
  │       ├─ WiFiClient tc; tc.setTimeout(5)
  │       ├─ tc.connect(IPAddress(8,8,8,8), 53)
  │       │   ├─ OK -> tc.stop(); pingFailCount = 0
  │       │   └─ FAIL -> pingFailCount++
  │       └─ ¿pingFailCount >= MAX_PING_FAILS (20)?
  │           └─ SI -> "20 ping fails. Rebooting..." -> ESP.restart()
  │
  ├─ ¿WiFi.status() != WL_CONNECTED?
  │   └─ SI -> wifiConnectRetries = 0 -> startWifi()
  │
  ├─ checkMqtt()  // ver detalle abajo
  │
  ├─ checkSensor()  // ver detalle en seccion PIR
  │
  └─ client.loop()  // procesar mensajes MQTT entrantes
```

### Flujo de checkMqtt (lineas 249-258)

```
checkMqtt()
  │
  └─ ¿!client.connected()?
      └─ SI ->
          ├─ "MQTT disconnected, reconnecting..."
          ├─ ¿WiFi.status() != WL_CONNECTED?
          │   └─ SI -> wifiConnectRetries = 0 -> startWifi()
          ├─ blinkStatus.attach(0.5, blinkLed)   // parpadeo medio
          ├─ blinkHeartbeat.detach()
          ├─ sendUpdate.detach()
          └─ startMqtt()  // reconexion completa
```

---

## Logica del Sensor PIR (Edge Detection)

### Principio de funcionamiento

El firmware implementa **deteccion de flancos simple** (edge detection). El sensor HC-SR501 ya incorpora su propio filtrado y estabilizacion de senal en hardware (chip BISS0001), por lo que no se requiere debounce adicional por software.

### Variables de estado

| Variable | Tipo | Proposito |
|----------|------|-----------|
| `sensorOn` | `bool` | Control ON/OFF remoto. `false` = no reporta detecciones. |
| `sensorTriggered` | `bool` | Estado actual del PIR: `true` = movimiento. Usado para detectar flancos. |
| `sensorReportSent` | `unsigned long` | Timestamp del ultimo reporte. Throttling 200ms. |
| `warmupStart` | `unsigned long` | Timestamp inicio warmup (65s). |
| `pirReady` | `bool` | `true` cuando warmup de 65s ha terminado. |

### Algoritmo de checkSensor() (lineas 128-147)

```
checkSensor()
  │
  ├─ ¿pirReady == false?
  │   ├─ ¿millis() - warmupStart >= PIR_WARMUP_MS (65000)?
  │   │   ├─ SI -> pirReady = true, Serial: "PIR ready"
  │   │   └─ NO -> return
  │   └─ return
  │
  ├─ state = digitalRead(PIR_PIN)  // GPIO4/D2
  │
  ├─ CASO 1: state == LOW && sensorTriggered == false
  │   └─ return  // Sin movimiento, sin cambios
  │
  ├─ CASO 2: state == LOW && sensorTriggered == true
  │   ├─ sensorTriggered = false
  │   └─ sendReport(false)  // Flanco HIGH->LOW: cese de movimiento
  │
  ├─ CASO 3: state == HIGH && sensorTriggered == false
  │   ├─ sensorTriggered = true
  │   └─ sendReport(true)   // Flanco LOW->HIGH: movimiento detectado
  │
  └─ CASO 4: state == HIGH && sensorTriggered == true
      └─ return  // Movimiento ya reportado, sin cambios
```

### Algoritmo de sendReport() (lineas 149-160)

```
sendReport(bool value)
  │
  ├─ ¿millis() - sensorReportSent > SENSOR_REPORT_MS (200)?
  │   └─ NO -> return  // Throttling: ignorar si < 200ms del ultimo
  │
  ├─ ¿sensorOn == true?
  │   ├─ SI:
  │   │   ├─ ¿value == true?
  │   │   │   ├─ sendToBroker("report/detectionState", "true")
  │   │   │   └─ Serial: ">>> MOTION DETECTED <<<"
  │   │   └─ ¿value == false?
  │   │       ├─ sendToBroker("report/detectionState", "false")
  │   │       └─ Serial: "--- motion cleared ---"
  │   │   sensorReportSent = millis()
  │   └─ NO -> return  // Sensor desactivado, no reportar
```

---

## API de Funciones

### Funciones de Inicializacion y Loop

#### `setup()`
Inicializa pines, lee RTC memory, conecta WiFi via `WiFi.begin()`, conecta MQTT, configura Tickers, inicia warmup del PIR de 65s.

#### `loop()`
Super-loop principal. Verifica watchdog, conectividad (WiFi + ping), reconecta si es necesario, lee sensor PIR, procesa mensajes MQTT entrantes.

---

### Funciones del Sensor PIR

#### `void checkSensor()`
Logica principal de deteccion del sensor PIR. Gestiona el warmup de 65s, lee `digitalRead(PIR_PIN)` y detecta flancos LOW->HIGH (movimiento) y HIGH->LOW (cese). Llama a `sendReport()` para publicar cambios.

**Parametros**: ninguno.  
**Retorno**: void.  
**Variables afectadas**: `pirReady`, `sensorTriggered`, `warmupStart`.

#### `void sendReport(bool value)`
Publica el estado de deteccion via MQTT con throttling de 200ms.

**Parametros**:
- `value`: `true` para movimiento, `false` para cese.

**Topic**: `<MQTT_CLIENT>/report/detectionState`  
**Payload**: `"true"` o `"false"`  
**Throttling**: 200 ms entre reportes consecutivos.

#### `void turnOn()`
Activa el sensor PIR (`sensorOn = true`) y publica confirmacion.

**Topic**: `<MQTT_CLIENT>/report/powerState`  
**Payload**: `"ON"`  
**Serial**: `"Sensor ON"`

#### `void turnOff()`
Desactiva el sensor PIR (`sensorOn = false`) y publica confirmacion.

**Topic**: `<MQTT_CLIENT>/report/powerState`  
**Payload**: `"OFF"`  
**Serial**: `"Sensor OFF"`

---

### Funciones MQTT

#### `void callback(char* topic, byte* payload, unsigned int length)`
Callback invocado por PubSubClient cuando se recibe un mensaje MQTT.

**Parametros**:
- `topic`: topic MQTT completo
- `payload`: contenido del mensaje (bytes)
- `length`: longitud del payload

**Comportamiento** (lineas 188-205):
1. `blinkMessageReceived()`: 3 parpadeos rapidos del LED.
2. Resetea `mqttPublishFailCount = 0` (recepcion confirma conectividad).
3. Parsea el topic con `splitTopic()`.
4. Construye `message[]` desde payload (bucle caracte por caracter).
5. **Validacion de seguridad**: verifica que `tokens[1] != NULL && tokens[2] != NULL` antes de `strcmp()`.
6. Procesa comando: si `tokens[1] == "directive" && tokens[2] == "powerState"`:
   - `"ON"` -> `turnOn()`
   - `"OFF"` -> `turnOff()`

#### `int splitTopic(char* topic, char* tokens[], int tokensNumber)`
Parsea un topic MQTT separando por `/` usando `strtok()`. Modifica el string de entrada in-place.

**Parametros**:
- `topic`: string con topic (modificado in-place)
- `tokens[]`: array de punteros a cada segmento
- `tokensNumber`: maximo de tokens a extraer (10 en este firmware)

**Retorno**: numero de tokens encontrados.

#### `void startMqtt()`
Establece conexion con el broker MQTT y configura suscripciones.

**Comportamiento** (lineas 260-289):
- `client.setServer(MQTT_BROKER, MQTT_PORT)`
- `client.setCallback(callback)`
- Bucle de conexion con timeout de 120s. Reintenta cada 10s (`delay(10000)` + `feedWatchdog()`).
- Si `client.state() == 5` (connection refused: not authorized): imprime advertencia de credenciales.
- Al conectar:
  - `mqttPublishFailCount = 0`
  - LED: `blinkStatus.attach(20, blinkLed)`, `blinkHeartbeat.attach(20, blinkLed)` (pulso cada 20s)
  - `sendUpdate.attach(REPORT_INTERVAL, sendOnlineReport)` (heartbeat cada 30s)
  - Suscribe `MQTT_CLIENT/#`
  - Publica `report/online = "true"`

#### `void checkMqtt()`
Verifica el estado de la conexion MQTT. Si no esta conectado: verifica WiFi, detiene Tickers de heartbeat, y llama a `startMqtt()`.

#### `void sendToBroker(char* topic, char* message)`
Funcion generica de publicacion MQTT con manejo de fallos.

**Parametros**:
- `topic`: sufijo del topic (se antepone `<MQTT_CLIENT>/`)
- `message`: payload

**Topic completo**: construido con `sprintf(t, "%s/%s", MQTT_CLIENT, topic)` (buffer 128 bytes).

**Manejo de errores**:
- Si `client.connected()` y `client.publish()` falla: `mqttPublishFailCount++`
- Si `!client.connected()`: `mqttPublishFailCount++`
- Si `mqttPublishFailCount >= MAX_MQTT_FAILURES` (10): `rebootWithNewMac()`

#### `void sendOnlineReport()`
Publica heartbeat periodico. Callback del Ticker `sendUpdate` (cada 30s).
**Topic**: `<MQTT_CLIENT>/report/online`, **Payload**: `"true"`.

---

### Funciones WiFi

#### `void startWifi()`
Gestiona la conexion WiFi directa con credenciales de `secrets.h`.

**Comportamiento** (lineas 222-244):
1. LED: `blinkStatus.attach(0.1, blinkLed)` (parpadeo rapido 100ms).
2. `WiFi.persistent(false)` -- no guardar configuracion en flash.
3. `WiFi.mode(WIFI_STA)` -- modo estacion.
4. `WiFi.begin(WIFI_SSID, WIFI_PASSWORD)` -- conexion con credenciales de `secrets.h`.
5. Bucle de espera: `delay(250)` + `feedWatchdog()` + contador de reintentos.
6. Si `wifiConnectRetries >= MAX_WIFI_RETRIES` (40): `rebootWithNewMac()`.
7. Al conectar: imprime SSID e IP. LED: `blinkStatus.attach(0.5, blinkLed)` (parpadeo medio 500ms).

---

### Funciones de LED

#### `void blinkLed()`
Toggle del LED. Callback para Tickers de patrones de parpadeo.

```cpp
void blinkLed() { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); }
```

**Nota**: LED_BUILTIN es active-low: LOW = encendido, HIGH = apagado.

#### `void blinkMessageReceived()`
Secuencia de 3 parpadeos rapidos al recibir mensaje MQTT (lineas 212-217):

```cpp
void blinkMessageReceived() {
  digitalWrite(LED_PIN, LOW);  delay(50);
  digitalWrite(LED_PIN, HIGH); delay(50);
  digitalWrite(LED_PIN, LOW);  delay(50);
  digitalWrite(LED_PIN, HIGH);
}
```

**Duracion total**: 150ms de bloqueo. Tras la secuencia, el Ticker `blinkStatus` o `blinkHeartbeat` restaura el patron normal en la siguiente activacion.

---

### Funciones de Resiliencia

#### `void feedWatchdog()`
Reinicia el Ticker del watchdog por software. Debe llamarse en cada iteracion del `loop()` y durante esperas largas.

```cpp
void feedWatchdog() { wdtTicker.once(WDT_TIMEOUT, wdtCallback); }
```

Programa `wdtCallback` para dentro de 180s. Cada llamada reinicia el contador.

#### `void checkInternetConnectivity()`
Verifica conectividad a internet mediante conexion TCP a `8.8.8.8:53` (DNS de Google).

**Comportamiento**:
- Crea `WiFiClient` con timeout de 5s.
- Intenta `connect(IPAddress(8,8,8,8), 53)`.
- Exito: `tc.stop()`, `pingFailCount = 0`.
- Fallo: `pingFailCount++`.
- Si `pingFailCount >= 20`: `ESP.restart()`.

#### `void randomizeMac()`
Aleatoriza el ultimo byte de la direccion MAC del ESP8266. Usa `wifi_set_macaddr(STATION_IF, mac)` de `user_interface.h`.

```cpp
void randomizeMac() {
  WiFi.disconnect();
  uint8_t mac[6]; WiFi.macAddress(mac);
  uint8_t old = mac[5];
  mac[5] = random(0x01, 0xFE);
  wifi_set_macaddr(STATION_IF, mac);
  Serial.printf("MAC: %02x -> %02x\n", old, mac[5]);
}
```

#### `void rebootWithNewMac()`
Prepara un reinicio con aleatorizacion de MAC en el siguiente arranque.

**Comportamiento**:
1. Escribe en RTC memory: `magic = 0xDEAD`, `failureFlag = 1`.
2. `delay(100)` para completar la escritura.
3. `ESP.restart()`.

**Disparadores**:
- 40 reintentos WiFi fallidos en `startWifi()`.
- 10 fallos consecutivos de publicacion MQTT en `sendToBroker()`.
- Timeout de 2 minutos en `startMqtt()`.

---

### Funciones ISR (Interrupt Service Routines)

#### `void IRAM_ATTR setPingFlag()`
Disparada por `pingCheck` Ticker cada 30s. Pone `shouldPing = true` (volatile).

#### `void IRAM_ATTR wdtCallback()`
Disparada por `wdtTicker` si el watchdog no se alimenta en 180s. Pone `wdtTriggered = true` (volatile).

---

## Estructura de Datos

### `rtcData_t` -- Datos en RTC Memory

```cpp
typedef struct {
  uint32_t magic;       // Magic number: 0xDEAD (valida integridad)
  uint16_t failureFlag; // 1 = reinicio previo fue por fallo
} rtcData_t;
```

**Tamano**: 6 bytes + 2 padding = 8 bytes.  
**Ubicacion**: offset 0 de RTC memory del ESP8266.  
**Proposito**: persistir flag de fallo entre reinicios para MAC randomization.

---

## Constantes de Configuracion

### Pines

| Constante | Valor | GPIO / Pin | Descripcion |
|-----------|-------|-----------|-------------|
| `LED_PIN` | 2 | GPIO2 (D4) | LED_BUILTIN, active-low |
| `PIR_PIN` | 4 | GPIO4 (D2) | Senal OUT del HC-SR501 |

### Timing del PIR

| Constante | Valor (ms) | Descripcion |
|-----------|-----------|-------------|
| `PIR_WARMUP_MS` | 65000 | Tiempo de warm-up del PIR (65s) |
| `SENSOR_REPORT_MS` | 200 | Intervalo minimo entre reportes (throttling) |

### Intervalos (segundos)

| Constante | Valor | Descripcion |
|-----------|-------|-------------|
| `REPORT_INTERVAL` | 30 | Heartbeat MQTT (`report/online`) |
| `PING_INTERVAL` | 30 | Ping check a 8.8.8.8 |

### Limites de Fallos

| Constante | Valor | Descripcion |
|-----------|-------|-------------|
| `MAX_WIFI_RETRIES` | 40 | Reintentos WiFi antes de reboot con nueva MAC |
| `MAX_MQTT_FAILURES` | 10 | Fallos MQTT consecutivos antes de reboot |
| `MAX_PING_FAILS` | 20 | Fallos de ping consecutivos (20 x 30s = 10 min) |
| `WDT_TIMEOUT` | 180 | Timeout watchdog (segundos) |

### Credenciales (secrets.h)

| Constante | Valor | Descripcion |
|-----------|-------|-------------|
| `WIFI_SSID` | `"Nv2"` | SSID de la red WiFi |
| `WIFI_PASSWORD` | `"12345678"` | Contrasena WiFi |
| `MQTT_BROKER` | `"www.smartage.cl"` | Direccion del broker MQTT |
| `MQTT_PORT` | `1886` | Puerto del broker |
| `MQTT_USERNAME` | `"luisHernandez"` | Usuario MQTT |
| `MQTT_PASSWORD` | `"luis123"` | Contrasena MQTT |
| `MQTT_CLIENT` | `"6a7357a00221f201ddcc5f1d"` | Client ID unico |

---

## Protocolo MQTT: Topics y Payloads

### Topics de publicacion (device -> broker)

| Topic relativo | Payloads | Frecuencia | Funcion que publica |
|---------------|----------|------------|-------------------|
| `report/online` | `"true"` | Cada 30s | `sendOnlineReport()` |
| `report/detectionState` | `"true"`, `"false"` | Al detectar/cesar movimiento | `sendReport()` |
| `report/powerState` | `"ON"`, `"OFF"` | Al arrancar o recibir comando | `turnOn()` / `turnOff()` |

### Topics de suscripcion (broker -> device)

| Topic | Payloads | Funcion que procesa |
|-------|----------|-------------------|
| `<MQTT_CLIENT>/#` | cualquiera | `callback()` |
| `<MQTT_CLIENT>/directive/powerState` | `"ON"`, `"OFF"` | `callback()` → `turnOn()` / `turnOff()` |

### Formato del topic completo

`sendToBroker()` construye el topic: `sprintf(t, "%s/%s", MQTT_CLIENT, topic)`.

Ejemplo: `6a7357a00221f201ddcc5f1d/report/detectionState`

El callback recibe el topic completo y lo parsea con `splitTopic()`. Tokens esperados:
- `tokens[0]` = MQTT_CLIENT (device ID)
- `tokens[1]` = namespace (`"report"` o `"directive"`)
- `tokens[2]` = recurso (`"detectionState"`, `"powerState"`, `"online"`)

---

## Librerias y Dependencias

### Librerias del Sistema (built-in, ESP8266 Core)

| Libreria | Include | Proposito |
|----------|---------|-----------|
| **ESP8266WiFi** | `<ESP8266WiFi.h>` | API WiFi modo estacion |
| **WiFiClient** | (incluido en ESP8266WiFi) | Sockets TCP para MQTT y ping |
| **Ticker** | `<Ticker.h>` | Timers basados en hardware timer |
| **Arduino** | `<Arduino.h>` | Core functions (pinMode, digitalRead, millis, etc.) |
| **user_interface** | `<user_interface.h>` | API bajo nivel (wifi_set_macaddr, RTC memory) |

### Libreria Externa

| Libreria | Version | Autor | Instalacion |
|----------|---------|-------|-------------|
| **PubSubClient** | 2.8.0 | Nick O'Leary | `arduino-cli lib install "PubSubClient"` |

### Instalacion completa

```bash
# Core ESP8266
arduino-cli config add board_manager.additional_urls \
  https://arduino.esp8266.com/stable/package_esp8266com_index.json
arduino-cli core update-index
arduino-cli core install esp8266:esp8266

# Unica libreria externa
arduino-cli lib install "PubSubClient"
```

---

## Mecanismos de Resiliencia

### 1. Watchdog por Software

```cpp
void feedWatchdog() {
    wdtTicker.once(WDT_TIMEOUT, wdtCallback);  // 180s
}
```

- Se llama en `loop()` y en bucles de espera (`startWifi`, `startMqtt`, `checkInternetConnectivity`).
- Si `loop()` se bloquea > 180s → `wdtCallback()` pone `wdtTriggered = true` → `ESP.restart()`.

### 2. Ping Check

```
pingCheck.attach(PING_INTERVAL, setPingFlag);  // Cada 30s
   │
   └─ setPingFlag() (ISR) → shouldPing = true
       │
       └─ loop() detecta flag → checkInternetConnectivity()
           ├─ TCP connect 8.8.8.8:53 (timeout 5s)
           ├─ Exito → pingFailCount = 0
           ├─ Fallo → pingFailCount++
           └─ pingFailCount >= 20 → ESP.restart()
```

### 3. Reconexion WiFi

En `loop()`: si `WiFi.status() != WL_CONNECTED` → `startWifi()`. La funcion `startWifi()` usa `WiFi.begin(WIFI_SSID, WIFI_PASSWORD)` con 40 reintentos (cada 250ms). Si falla → `rebootWithNewMac()`.

### 4. MAC Randomization

Flujo completo:

```
Fallo (WiFi 40 reintentos | MQTT 10 fallos | timeout MQTT 2min)
  │
  └─ rebootWithNewMac()
      ├─ RTC.magic = 0xDEAD
      ├─ RTC.failureFlag = 1
      ├─ ESP.rtcUserMemoryWrite(0, ...)
      └─ ESP.restart()
          │
          └─ setup()
              ├─ ESP.rtcUserMemoryRead(0, ...)
              ├─ ¿magic == 0xDEAD && failureFlag == 1?
              │   ├─ SI → randomizeMac()
              │   │       ├─ WiFi.disconnect()
              │   │       ├─ WiFi.macAddress(mac)
              │   │       ├─ mac[5] = random(0x01, 0xFE)
              │   │       └─ wifi_set_macaddr(STATION_IF, mac)
              │   └─ Limpiar failureFlag en RTC
              └─ Continuar con nueva MAC
```

### 5. Reconexion MQTT

`checkMqtt()` en `loop()` verifica `client.connected()`. Si no esta conectado → detiene Tickers → `startMqtt()` (reintentos cada 10s, timeout 2min).

---

## Guia de Modificacion y Extension

### Cambiar credenciales

Editar `secrets.h` y recompilar:

```cpp
#define WIFI_SSID "MiNuevaRed"
#define WIFI_PASSWORD "miPassword"
#define MQTT_BROKER "nuevo-broker.com"
#define MQTT_PORT 1883
#define MQTT_CLIENT "nuevo-id-unico"
```

### Cambiar intervalos de timing

Modificar constantes en el sketch:

```cpp
#define REPORT_INTERVAL  60    // heartbeat cada 60s
#define PING_INTERVAL    60    // ping cada 60s
#define SENSOR_REPORT_MS 500   // throttling 500ms
#define PIR_WARMUP_MS    30000 // warmup 30s
```

### Cambiar tiempo de warmup del PIR

```cpp
#define PIR_WARMUP_MS  30000   // reducir a 30s (requiere que el sensor lo soporte)
```

### Modificar la logica de deteccion (agregar debounce)

```cpp
// En checkSensor(), antponer debounce antes de la logica de flancos:
#define PIR_DEBOUNCE_MS 300

static int lastStableState = LOW;
static unsigned long lastChangeTime = 0;

int rawState = digitalRead(PIR_PIN);

if (rawState != lastStableState) {
    lastChangeTime = millis();
    lastStableState = rawState;
    return;  // esperar a que se estabilice
}

if (millis() - lastChangeTime < PIR_DEBOUNCE_MS) {
    return;  // aun en periodo de debounce
}

// ... logica de flancos existente con lastStableState ...
```

### Agregar un nuevo comando MQTT

En `callback()`, agregar despues del bloque `directive/powerState`:

```cpp
// Comando: directive/reboot
if (tokens[1] != NULL && tokens[2] != NULL &&
    strcmp(tokens[1], "directive") == 0 && strcmp(tokens[2], "reboot") == 0) {
    if (strcmp(message, "now") == 0) {
        Serial.println(F("Reboot command received. Restarting..."));
        ESP.restart();
    }
}

// Comando: directive/reportInterval
if (tokens[1] != NULL && tokens[2] != NULL &&
    strcmp(tokens[1], "directive") == 0 && strcmp(tokens[2], "reportInterval") == 0) {
    int newInterval = atoi(message);
    if (newInterval >= 5 && newInterval <= 3600) {
        sendUpdate.attach(newInterval, sendOnlineReport);
        Serial.printf("Report interval changed to %ds\n", newInterval);
    }
}
```

### Agregar OTA (Over-The-Air updates)

```cpp
#include <ArduinoOTA.h>

void setup() {
    // ... codigo existente ...

    ArduinoOTA.setHostname("SensorPIR-8266");
    ArduinoOTA.onStart([]() { Serial.println("OTA Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("\nOTA Complete"); });
    ArduinoOTA.onProgress([](unsigned int prog, unsigned int total) {
        Serial.printf("Progress: %u%%\r", prog / (total / 100));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
    });
    ArduinoOTA.begin();

    // ... resto del setup ...
}

void loop() {
    ArduinoOTA.handle();  // primera linea del loop
    // ... resto del loop ...
}
```

---

## Como Agregar Nuevos Sensores/Actuadores

### Ejemplo: Sensor de temperatura DHT22

**1. Includes, pines y constantes:**

```cpp
#include <DHT.h>

#define DHT_PIN   5    // D1 (GPIO5)
#define DHT_TYPE  DHT22
#define TEMP_READ_INTERVAL 60000  // 60s

DHT dht(DHT_PIN, DHT_TYPE);
unsigned long lastTempRead = 0;
```

**2. Inicializar en `setup()`:**

```cpp
dht.begin();
```

**3. Agregar lectura periodica en `loop()`:**

```cpp
void loop() {
    // ... codigo existente ...

    unsigned long now = millis();
    if (now - lastTempRead >= TEMP_READ_INTERVAL) {
        lastTempRead = now;
        float temp = dht.readTemperature();
        float hum = dht.readHumidity();
        if (!isnan(temp) && !isnan(hum)) {
            char payload[32];
            sprintf(payload, "%.1f,%.1f", temp, hum);
            sendToBroker("report/temperature_humidity", payload);
        }
    }
}
```

### Ejemplo: Relay/actuador controlado por MQTT

**1. Definir pin:**

```cpp
#define RELAY_PIN  14   // D5 (GPIO14)
```

**2. Configurar en `setup()`:**

```cpp
pinMode(RELAY_PIN, OUTPUT);
digitalWrite(RELAY_PIN, LOW);  // Relay off
```

**3. Funcion de control:**

```cpp
void setRelay(bool state) {
    digitalWrite(RELAY_PIN, state ? HIGH : LOW);
    sendToBroker("report/relayState", state ? "ON" : "OFF");
}
```

**4. Procesar comando en `callback()`:**

```cpp
// Dentro de callback(), despues del manejo de powerState:
if (tokens[1] != NULL && tokens[2] != NULL &&
    strcmp(tokens[1], "directive") == 0 && strcmp(tokens[2], "relay") == 0) {
    if (strcmp(message, "ON") == 0) setRelay(true);
    else if (strcmp(message, "OFF") == 0) setRelay(false);
}
```

---

## Notas de Memoria y Rendimiento

### Uso de Memoria (estimado)

| Region | Usado | Total | % |
|--------|-------|-------|---|
| Flash (sketch) | ~329 KB | ~1 MB (disponible) | ~31% |
| RAM (heap) | ~31 KB | ~80 KB | ~38% |

### Buenas Practicas Aplicadas

1. **Sin `String`**: todas las cadenas son `char[]`, evitando fragmentacion de heap.
2. **Macro `F()`**: cadenas de `Serial.print()` usan `F()` para almacenarlas en PROGMEM (flash).
3. **Sin `malloc`/`new`**: todos los buffers son stack-allocated con tamanos fijos.
4. **`volatile` en ISR**: variables modificadas por ISR (`shouldPing`, `wdtTriggered`) declaradas volatile.
5. **`millis()` overflow-safe**: calculos de tiempo con `now - start >= interval`.
6. **Edge detection minimalista**: sin debounce, sin maquina de estados compleja → bajo uso de RAM y CPU.

### Puntos de Atencion

- **`delay(50)` en `blinkMessageReceived()`**: 150ms totales de bloqueo. Solo ocurre al recibir mensajes MQTT (infrecuente). No afecta operacion normal.
- **`delay(250)` en `startWifi()`**: solo durante conexion/reconexion WiFi. `feedWatchdog()` se llama en cada iteracion.
- **`delay(10000)` en `startMqtt()`**: solo durante reconexion MQTT. `feedWatchdog()` se llama antes del delay.
- **Buffer `message[]` en callback**: VLA `char message[length + 1]` en stack. Para payloads tipicos (< 256 bytes) es seguro. Si se esperan payloads muy grandes, considerar buffer fijo.

---

## Checklist de Seguridad para Pull Requests

Antes de enviar cambios, verificar:

- [ ] Credenciales WiFi y MQTT siguen solo en `secrets.h`
- [ ] No se han agregado variables `String` (usar `char[]`)
- [ ] Cadenas `Serial.print()` usan macro `F()` para PROGMEM
- [ ] Variables modificadas por ISR son `volatile`
- [ ] `feedWatchdog()` se llama en todos los bucles de espera
- [ ] Contadores de fallos tienen limites y provocan recuperacion
- [ ] Buffers con tamanos fijos (no VLAs excepto en callback donde es acotado)
- [ ] `tokens[] != NULL` validado antes de `strcmp()` en callback
- [ ] Pines nuevos no conflictuan con GPIO0, GPIO2, GPIO15 (boot) ni GPIO1/GPIO3 (Serial)
- [ ] Actualizado README.md con nueva tabla de pines si se agregaron
- [ ] Firmware compila: `arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 .`

---

## Compilacion para Desarrollo

```bash
# Compilar (verboso para warnings)
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 --verbose .

# Compilar y transferir
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 . && \
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:nodemcuv2 .

# Monitor Serial
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### Limpiar build artifacts

```bash
rm -rf build/
arduino-cli cache clean
```

---

**Documentacion para desarrolladores -- SensorPIR-8266 v2.0**
Basado en arquitectura lp101_secure con conexion WiFi directa.
Fecha: 2026-08-05
