# TempSensor-8266 — Documentación para desarrolladores

Explicación del código fuente de `TempSensor-8266.ino` (v2.2, 581 líneas).

## 1. Compilar y subir

```bash
./scripts/compilar.sh     # solo compilar -> build/, log en build/compile.log
./scripts/subir.sh        # compila y transfiere
./scripts/detectar.sh     # identificar puerto
./scripts/monitor.sh /dev/ttyUSB0 60
```

Toolchain: `arduino-cli` + core `esp8266:esp8266@3.1.2`, FQBN `esp8266:esp8266:nodemcuv2`.
El sketch debe compilarse apuntando a la **carpeta**, no al archivo `.ino`:

```bash
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 .
```

> `arduino-cli upload` no recompila. Compilar y subir siempre en el mismo paso, o se
> transfiere el binario anterior.

## 2. Estructura del sketch

### Configuración (`#define`, líneas 41-102)

| Define | Valor | Rol |
|--------|-------|-----|
| `ONE_WIRE_BUS` | 4 | Pin del bus OneWire (D2) |
| `LED_PIN` | 2 | LED_BUILTIN (D4) |
| `REPORT_INTERVAL` | 60 | Segundos entre reportes de temperatura |
| `SENSOR_POLL_MS` | 100 | Resolución del scheduler del sensor |
| `PING_INTERVAL` | 30 | Segundos entre rondas de ping |
| `MAX_WIFI_RETRIES` | 120 | 120 × 250 ms = 30 s |
| `MAX_MQTT_FAILURES` | 10 | Publicaciones fallidas antes de reiniciar |
| `MQTT_CONNECT_TIMEOUT` | 120000 | Timeout total de conexión MQTT |
| `PING_TARGET_IP` | `0x08080808UL` | 8.8.8.8 en network byte order |
| `PING_MAX_TRIES` | 5 | Probes por ronda |
| `PING_GAP_MS` | 2000 | Pausa entre probes |
| `PING_ROUND_MAX_MS` | 20000 | Tope de la ronda completa |
| `STALE_REPORT_MS` | 300000 | 5 min sin publicar → reinicio |
| `MSG_MAX` | 192 | Tamaño del buffer del callback MQTT |

Los secretos vienen de `secrets.h` (no versionado). Si falta, el sketch no compila.

### Funciones, en orden de declaración

| Función | Línea | Responsabilidad |
|---------|-------|-----------------|
| `blinkLed` | 310 | Invierte el LED |
| `blinkMessageReceived` | 312 | Parpadeo doble al recibir un mensaje |
| `sendOnlineReport` | 445 | Publica `report/online` |
| `sendTemperature` | 220 | Lee el sensor y publica temperatura + estado |
| `requestTemperature` | 213 | Dispara la conversión DS18B20 |
| `startWifi` | 324 | Conexión WiFi con reintentos |
| `startMqtt` | 377 | Conexión y suscripción MQTT |
| `checkMqtt` | 364 | Mantiene viva la conexión MQTT |
| `sendToBroker` | 422 | Publica y actualiza `lastGoodPublish` |
| `turnOn` / `turnOff` | 259 / 266 | Activa y desactiva el sensor |
| `splitTopic` | 275 | Divide el topic recibido por `/` |
| `callback` | 286 | Dispatcher de mensajes MQTT entrantes |
| `randomizeMac` | 450 | Aleatoriza el último byte de la MAC |
| `rebootWithNewMac` | 459 | Reinicio con MAC nueva |
| `onPingRecv` / `onPingSent` | 472 / 487 | Callbacks del SDK de ping |
| `startPingRound` | 491 | Lanza una ronda de ICMP |
| `servicePing` | 513 | Decide OK / reinicio según el resultado |
| `brokerReachable` | 562 | Verificación secundaria por TCP |
| `checkStaleReport` | 574 | Reinicio si no se publica hace 5 min |

## 3. Flujo principal

### `setup()` (línea 134)

1. Serial a 115200,Led apagado.
2. `ESP.wdtEnable(WDTO_8S)` — el watchdog de hardware, activo desde el primer instante.
3. Inicializa el driver OneWire, el DS18B20 con resolución de 12 bits, y busca sensores.
4. `randomizeMac()` + `startWifi()` + `startMqtt()`.
5. Banner de arranque con la configuración vigente.

### `loop()` (línea 184)

```
loop() {
  ESP.wdtFeed();                 // mantiene vivo el WDT
  if (!wifiConnected) { startWifi(); return; }
  checkMqtt();                   // reconexión MQTT si hace falta
  servicePing();                 // ronda de ICMP cada PING_INTERVAL
  checkStaleReport();            // guard de 5 min sin publicar
  scheduler del sensor           // ver abajo
}
```

**El scheduler del sensor** es la pieza clave del diseño:

```
cada SENSOR_POLL_MS (100 ms):
  si millis() - lastReportAt >= REPORT_INTERVAL * 1000:
      sendTemperature();         // publica el valor ya convertido
      requestTemperature();      // y dispara la siguiente conversión
```

### Conversión no bloqueante

La versión anterior bloqueaba ~750 ms esperando la conversión de 12 bits. Ahora se separa:

- `requestTemperature()` dispara la conversión y marca `convPending`.
- `sendTemperature()` consulta con `getTempCByIndex()` y respeta un timeout
  `CONV_TIMEOUT_MS` (2 s). Si vence, publica `sensorStatus = CONVERSION_TIMEOUT`.

> **No sondear el sensor cada 100 ms esperando el resultado.** En cuanto termina la
> conversión, `getTempCByIndex()` devuelve el valor convertido (no 85), así que un sondeo
> frecuente publicaría la misma lectura en bucle. El gating por `REPORT_INTERVAL` es
> obligatorio, no una optimización.

## 4. Las tres capas de resiliencia

### Capa 1 — Watchdog de hardware

`ESP.wdtEnable(WDTO_8S)` es el timeout máximo que ofrece el ESP8266. Cubre cuelgues de CPU
que dejarían el loop principal sin avanzar. Se alimenta explícitamente en `loop()`, en el
bucle de `startWifi()`, en `brokerReachable()` y en los 10 slices de `delay(1000)` de
`startMqtt()` — **cada `delay()` largo debe ir acompañado de un `wdtFeed()`**, o el chip se
reiniciará a mitad de la conexión.

El objetivo de alimentación (`WDT_FEED_INTERVAL`) son 4 s, con margen 2× sobre el límite.

### Capa 2 — Ronda de ping ICMP

Cada `PING_INTERVAL` (30 s), `startPingRound()` hace **una sola llamada** a `ping_start()`
con `pingOpt.count = 5` y `pingOpt.coarse_time = 2000`.

**Por qué una sola llamada.** `ping_start()` no se puede relanzar dentro del mismo boot: el
SDK no libera su pbuf interno, así que las llamadas siguientes no emiten nada. El síntoma
observable era un loop de reinicio: primer probe OK, 5 fallos, reinicio, repetir. La
solución es delegar el encadenado de los 5 probes al propio SDK.

**`struct ping_option` debe ser global.** El SDK conserva el puntero y lo desreferencia
desde su propia tarea. Con la struct en el stack aparece `Exception (28)` con un stack dump
que muestra `08080808` — ni más ni menos, la IP destino.

**Los callbacks corren en contexto de tarea**, no en el loop: `onPingRecv()` solo toca
flags (`pingReplies`, `pingRoundDone`, `pingLast`). Toda la decisión ocurre en
`servicePing()`, invocado desde `loop()`.

`servicePing()` decide así:

```
pingReplies > 0                        → "OK", no se reinicia
pingReplies == 0 y brokerReachable()   → "ICMP filtrado", no se reinicia
pingReplies == 0 y !brokerReachable()  → rebootWithNewMac()
```

La verificación secundaria por TCP es **necesaria**. Sin ella, cualquier red que filtre ICMP
saliente (corporativa, cellular, muchos hotspots) provocaría reinicios indefinitely.

**No usar `pingLast.timeout_count` como criterio.** El campo nunca llega poblado (el callback
solo se dispara con respuesta), lo que hace que cualquier condición que lo incluya dé
siempre `true` y el reinicio nunca ocurra.

### Capa 3 — Guard de "sin reportar"

`checkStaleReport()` compara `millis() - lastGoodPublish` contra `STALE_REPORT_MS` (5 min).
`lastGoodPublish` se actualiza en cada `client.publish()` exitoso y al conectar.

Cubre el fallo que las otras dos capas no ven: el equipo está vivo, conectado al WiFi, sin
excepciones, simplemente no publica.

**Excluido cuando `sensorOn == false`.** Con el sensor apagado no hay reportes por diseño,
y un guard que no lo consideraría provocaría un loop de reinicio inmediato.

## 5. Trampas del SDK del ESP8266

Recopilación de lo que costó encontrar. **Todas están resueltas en el firmware actual.**

### `WiFi.ping()` no existe

El core 3.1.2 no expone ese método en `ESP8266WiFiClass`. El `ping.h` del SDK solo da la API
de C de bajo nivel (`ping_start`, `ping_regist_recv`, `ping_regist_sent`).

### `yield()` desde contexto de Ticker provoca panic

`core_esp8266_main.cpp:191` llama a `panic()` cuando `cont_can_suspend()` es falso, y los
callbacks de `Ticker` corren en contexto de interrupción. Cualquier cadena que llegue a
`yield()` — y `OneWire` llega, vía `delayMicroseconds()` — provoca:

```
panic core_esp8266_main.cpp:191
```

Se eliminó el `Ticker` que invocaba la lectura del sensor. **La regla general: no llamar
nada que use `delay`, `delayMicroseconds` o el bus OneWire desde un `Ticker` o desde un
callback de interrupción.** Toda temporización va en el `loop()` con `millis()`.

### `setTimeout()` en milisegundos, no en segundos

`WiFiClient.setTimeout(3)` es **3 milisegundos**, no 3 segundos. No alcanza para un handshake
TCP, así que la verificación del broker fallaba siempre y disparaba reinicios sin motivo.
Usar `setTimeout(2000)`.

## 6. Seguridad de memoria

### Buffer del callback MQTT

La versión anterior dimensionaba el buffer con la longitud del payload remoto:

```cpp
char message[length + 1];   // VLA dimensionado por dato no confiable
```

El payload viene de la red, así que es un vector de desbordamiento de stack. Ahora:

```cpp
char message[MSG_MAX];                  // MSG_MAX = 192, fijo
memcpy(message, payload, length < MSG_MAX ? length : MSG_MAX - 1);
message[...] = '\0';
```

`sendToBroker()` usa `snprintf`, no `sprintf`.

### Sensor ausente o desconectado

`getTempCByIndex()` devuelve `-127` cuando no hay sensor. El código distingue ese valor y
publica `sensorStatus = DISCONNECTED` en vez de reportar una temperatura falsa.

## 7. Contrato MQTT

### Publicaciones

| Topic | Frecuencia | Payload |
|-------|-----------|---------|
| `report/temperature` | 60 s | Entero °C (`lroundf`) |
| `report/temperatureDetail` | 60 s | JSON: `t_c`, `t_f`, `rssi`, `heap`, `uptime` |
| `report/sensorStatus` | 60 s | `OK` / `DISCONNECTED` / `CONVERSION_TIMEOUT` |
| `report/humidity` | al conectar / al activar | `"N/A"` |
| `report/online` | 60 s | `"true"` |
| `report/powerState` | al cambiar | `"ON"` / `"OFF"` |

`report/temperature` se mantiene entero a propósito: hay consumidores previos que no
soportan un payload con decimales. Quien necesite precisión completa usa
`temperatureDetail`.

### Suscripción

El dispositivo se suscribe a `<CLIENT_ID>/#`. Al conectar se fija
`lastReportAt = millis() - (REPORT_INTERVAL * 1000)` para que el scheduler dispare la
primera conversión dentro de los primeros 100 ms, en lugar de esperar un minuto entero.

## 8. Reinicios

`rebootWithNewMac()` aleatoriza el último byte de la MAC antes de reiniciar, lo que fuerza
al router a re-resolver la tabla ARP y evita quedar trabado detrás de una entrada
obsoleta. Un flag en RTC memory distingue un fallo de arranque de un reinicio limpio.

No hay que confundir tres reinicios con causas distintas:

| Log | Causa | Temporización |
|-----|-------|---------------|
| Panic en `core_esp8266_main.cpp:191` | `yield()` en contexto de interrupción | Al usar el sensor por Ticker |
| `Ping ...: FALLO` + `Broker MQTT responde: no` | Pérdida de red real | Cada 30 s |
| `Ping ...: FALLO` + `Broker MQTT responde: si` | ICMP filtrado | **No reinicia** |
| `STALE` | 5 min sin publicar, sensor `ON` | 5 min |

## 9. Consumo de recursos (v2.2)

```
Variables y constantes RAM (global, static)   29552 / 80192 bytes (36%)
Instruction RAM (IRAM_ATTR, ICACHE_RAM_ATTR)  60411 / 65536 bytes (92%)
Code in flash (default, ICACHE_FLASH_ATTR)   267676 / 1048576 bytes (25%)
```

El 92% de IRAM es el baseline del core ESP8266 cuando se usa WiFi + PubSubClient, no una
consecuencia de este código. Queda margen de RAM estática amplio.

## 10. Depuración por puerto serie

Salida a 115200 baudios. Los primeros bytes tras el reset son basura binaria: el ESP envía
bytes en blanco mientras el core inicializa y el driver CH340 los emite como caracteres.

Para capturar el log de arranque hay que **pulsar el reset por RTS/DTR antes de leer**:

```python
import serial, time
s = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.2)
s.dtr = False; s.rts = True; time.sleep(0.1)
s.rts = False;            time.sleep(0.3)
```

Sin ese pulso solo se lee el backlog acumulado del driver, que mezcla varias ejecuciones y
produce falsos positivos.

Para filtrar la basura:

```bash
tr -cd '\11\12\15\40-\176'
```

## 11. Verificación

| Prueba | Resultado esperado |
|--------|-------------------|
| Operación normal | `Temperature: 18C (...) OK` cada 60 s, `Ping ...: OK`, 0 reinicios |
| ICMP bloqueado pero broker accesible | `Ping ...: FALLO` + `Broker MQTT responde: si` + **0 reinicios** |
| Red caída | Rondas de FALLO, broker caído, reinicio con MAC nueva |
| Sensor desconectado | `sensorStatus = DISCONNECTED`, sin reinicio |
| Código colgado | Reinicio por watchdog a los 8 s |
| Sin publicar 5 min con sensor `ON` | Reinicio por guard de reporte |
