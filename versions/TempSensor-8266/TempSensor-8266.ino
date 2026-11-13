// TempSensor-8266 — v2
// DS18B20 temperature sensor + MQTT reporting (NodeMCU ESP8266)
//
// v2 respecto de v1:
//   - Lectura no bloqueante de la conversión DS18B20 (v1 bloqueaba 750 ms)
//   - Temperatura con 2 decimales en report/temperatureDetail (v1 truncaba a entero)
//   - Validación de errores del sensor publicada a MQTT (v1 solo lo logueaba)
//   - report/humidity publicado explícitamente como no soportado
//   - Conectividad por ICMP (WiFi.ping) en vez de TCP:53 que nunca conecta
//   - Payload MQTT con buffer fijo en vez de VLA (posible desbordamiento de stack)
//   - Suscripción MQTT verificada; reporte inmediato al conectar
//   - LED sin conflicto entre tickers de estado y heartbeat
//   - La lectura del DS18B20 sale del contexto Ticker: los callbacks de Ticker
//     corren en contexto SYS y delayMicroseconds() -> yield() -> panic() en
//     core_esp8266_main.cpp:191. Ahora la conversión se dispara y se consulta
//     desde loop(), con un scheduler propio en millis().
//   - El reporte periódico lo hace loop() (sendTemperature ahora sí se llama,
//     en la v2 quedaba declarado pero nunca invocado -> nunca se publicaba)
//   - Watchdog REAL del SDK (ESP.wdtEnable/wdtFeed) en vez del Ticker software:
//     el Ticker no dispara si el panic loPara todo, el WDT del hardware sí
//   - Watchdog de redundancia: si no se publica nada en STALE_REPORT_MS se
//     reinicia. Cubre el fallo de WDT + MQTT: el WDT nunca se cuelga, pero el
//     broker puede caerse y el equipo quedaria mudo reintentando sin reportar.
//
// IMPORTANTE: el DS18B20 NO mide humedad. Solo temperatura.

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Ticker.h>
#include <Arduino.h>
#include "secrets.h"

extern "C" {
  #include <user_interface.h>
  #include <ping.h>
}

// ─── Pines ──────────────────────────────────────────────────────────
#define ONE_WIRE_BUS  4    // D2 (GPIO4) — DS18B20 data pin
#define LED_PIN       2    // D4 (GPIO2) — LED_BUILTIN

// ─── Timing ────────────────────────────────────────────────────────
#define REPORT_INTERVAL      30   // reporte de temperatura (segundos)
#define SENSOR_POLL_MS      100   // resolución del scheduler del sensor
#define PING_INTERVAL        30   // segundos entre rondas de ping
#define MAX_WIFI_RETRIES    120   // 120 x 250 ms = 30 s (DHCP tras flasheo es lento)
#define MAX_MQTT_FAILURES    10
#define MQTT_CONNECT_TIMEOUT 120000UL

// ── Watchdog ──────────────────────────────────────────────────────────
// WDT real del SDK. WDTO_8S es el maximo del enum; hay que alimentar antes de
// 8 s, asi que ninguna espera bloqueante puede superar ese tiempo.
#define WDT_FEED_INTERVAL     4   // s, objetivo de alimentacion (margen 2x)

// ── Conectividad: ICMP real a 8.8.8.8 ────────────────────────────────
#define PING_TARGET_IP  0x08080808UL   // 8.8.8.8 en network byte order
#define PING_MAX_TRIES      5          // probes por ronda antes de reiniciar
#define PING_TIMEOUT_MS  3000UL        // timeout de cada probe
#define PING_GAP_MS      2000UL        // pausa entre probes de una ronda
#define PING_ROUND_MAX_MS 20000UL      // tope de la ronda entera

// Si no se publica nada en este plazo, se reinicia (evita equipos mudos)
#define STALE_REPORT_MS 300000UL        // 5 minutos = 5 reportes perdidos

// ─── Globals ───────────────────────────────────────────────────────
WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Ticker blinkStatus;
Ticker blinkHeartbeat;

int mqttPublishFailCount = 0;
int wifiConnectRetries = 0;
bool sensorOn = true;

// Estado de la conversión no bloqueante del DS18B20
bool convPending = false;
uint32_t convStartedAt = 0;

// Scheduler del sensor en millis() (nada de Ticker: ver nota de cabecera)
uint32_t lastReportAt = 0;
uint32_t lastPollAt = 0;
uint32_t lastGoodPublish = 0;   // base del watchdog de "sin reportar"

// Estado de la ronda de ping ICMP. Los callbacks del SDK corren en su propia
// tarea, asi que solo tocan flags; toda la logica va en loop().
// OJO: ping_start NO se puede relanzar sobre el mismo estado (tras el primer
// probe los siguientes nunca se envian: el SDK no libera su pbuf interno).
// Por eso toda la ronda va en UNA sola llamada con count = PING_MAX_TRIES.
volatile bool    pingRoundActive = false;
volatile bool    pingRoundDone = false;
volatile uint32_t pingReplies = 0;
ping_resp        pingLast;          // ultimo resumen entregado por el SDK
struct ping_option pingOpt;          // global: el SDK conserva el puntero
uint32_t pingNextTryAt = 0;
uint32_t pingRoundStart = 0;

#define MSG_MAX 192

// ─── RTC data ──────────────────────────────────────────────────────
typedef struct {
  uint32_t magic;
  uint16_t failureFlag;
} rtcData_t;

// ─── Declarations ──────────────────────────────────────────────────
void blinkLed();
void blinkMessageReceived();
void sendOnlineReport();
void sendTemperature();
void requestTemperature();
void startWifi();
void startMqtt();
void checkMqtt();
void sendToBroker(const char* topic, const char* message);
void turnOn();
void turnOff();
int splitTopic(char* topic, char* tokens[], int tokensNumber);
void callback(char* topic, byte* payload, unsigned int length);
void randomizeMac();
void rebootWithNewMac();
void servicePing();
void onPingRecv(void* arg, void* pdata);
void onPingSent(void* arg, void* pdata);
void startPingRound();
bool brokerReachable();
void checkStaleReport();

// ═══════════════════════════════════════════════════════════════════
void setup() {
  // Watchdog REAL del SDK, habilitado antes de cualquier espera larga.
  // Si el firmware se cuelga en cualquier punto, el hardware reinicia solo.
  ESP.wdtEnable(WDTO_8S);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.begin(115200);
  delay(50);
  ESP.wdtFeed();

  // Init DS18B20
  sensors.begin();
  sensors.setResolution(12);
  Serial.print(F("DS18B20 sensors found: "));
  Serial.println(sensors.getDeviceCount());
  if (sensors.getDeviceCount() == 0) {
    Serial.println(F("!! Ningun sensor 1-Wire detectado en D2. "
                     "Revisa cableado y resistencia pull-up 4.7k."));
  }
  ESP.wdtFeed();

  randomSeed(ESP.getCycleCount());

  rtcData_t rtc;
  ESP.rtcUserMemoryRead(0, (uint32_t*)&rtc, sizeof(rtc));
  if (rtc.magic == 0xDEAD && rtc.failureFlag == 1) {
    Serial.println(F("Failure reboot detected. Randomizing MAC..."));
    randomizeMac();
    rtc.failureFlag = 0;
    ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtc, sizeof(rtc));
  }

  wifiConnectRetries = 0;
  mqttPublishFailCount = 0;
  pingRoundActive = false;
  pingNextTryAt = 0;

  startWifi();
  startMqtt();

  lastGoodPublish = millis();
  Serial.println(F("TempSensor-8266 v2.2 booted"));
  Serial.println(F("  Sensor: DS18B20 on D2/GPIO4 (temperatura ONLY, sin humedad)"));
  Serial.printf("  Report: report/temperature every %ds\n", REPORT_INTERVAL);
  Serial.printf("  WDT: 8s | Ping ICMP 8.8.8.8 cada %ds, %d intentos, "
                "reinicio si no hay reporte en %lus\n",
                PING_INTERVAL, PING_MAX_TRIES, STALE_REPORT_MS / 1000);
}

void loop() {
  ESP.wdtFeed();

  if (WiFi.status() != WL_CONNECTED) { wifiConnectRetries = 0; startWifi(); }
  checkMqtt();
  client.loop();
  servicePing();
  checkStaleReport();

  // ── Scheduler del sensor (contexto loop, seguro para OneWire) ──
  // Solo al cumplirse el intervalo: getTempCByIndex() devuelve el valor de la
  // conversion anterior (no 85) en cuanto termina, asi que sondear cada 100 ms
  // publicaria en bucle. El sondeo solo sirve para no bloquear.
  uint32_t now = millis();
  if (sensorOn && (uint32_t)(now - lastPollAt) >= SENSOR_POLL_MS) {
    lastPollAt = now;
    if ((uint32_t)(now - lastReportAt) >= (REPORT_INTERVAL * 1000UL)) {
      lastReportAt = now;
      sendTemperature();      // publica la conversion pendiente (no-op si no hay)
      requestTemperature();   // dispara la siguiente
    }
  }
}

// ═══════════════════════════════════════════════════════════════════
//  TEMPERATURE SENSOR
// ═══════════════════════════════════════════════════════════════════

// Dispara la conversión sin bloquear (12 bits ≈ 750 ms)
void requestTemperature() {
  sensors.requestTemperatures();
  convPending = true;
  convStartedAt = millis();
}

// Chequea si la conversión terminó; publica cuando sí
void sendTemperature() {
  if (!sensorOn || !convPending) return;

  // Criterio de "terminó": que haya pasado el tiempo de conversión que declara
  // el sensor para su resolución (750 ms a 12 bits). Antes se usaba el timeout
  // CONV_TIMEOUT_MS (2 s), lo cual es incorrecto con un intervalo de reporte
  // grande: a los 30 s de pedir la conversión, el tiempo transcurrido siempre
  // supera los 2 s y disparaba timeout espurio desde el segundo ciclo.
  // Un timeout solo tiene sentido si se sondea seguido; acá se sondea cada
  // REPORT_INTERVAL, así que la conversión está completa o el sensor está roto
  // (lo detecta DEVICE_DISCONNECTED_C más abajo).
  if ((uint32_t)(millis() - convStartedAt) < (uint32_t)sensors.millisToWaitForConversion()) {
    return;   // todavía no terminó: no tocar el scratchpad
  }

  convPending = false;
  float t = sensors.getTempCByIndex(0);

  if (t == DEVICE_DISCONNECTED_C) {
    Serial.println(F("DS18B20 disconnected"));
    sendToBroker("report/sensorStatus", "DISCONNECTED");
    requestTemperature();               // reintentar
    return;
  }

  // report/temperature  -> entero (compatible con el consumidor previo)
  char msgInt[MSG_MAX];
  snprintf(msgInt, sizeof(msgInt), "%d", (int)lroundf(t));
  sendToBroker("report/temperature", msgInt);

  // report/temperatureDetail -> payload con precisión completa
  char msgJson[MSG_MAX];
  snprintf(msgJson, sizeof(msgJson),
           "{\"t_c\":%.2f,\"t_f\":%.2f,\"rssi\":%d,\"heap\":%u,\"uptime\":%lu}",
           t, (t * 1.8f) + 32.0f, WiFi.RSSI(), ESP.getFreeHeap(), millis() / 1000UL);
  sendToBroker("report/temperatureDetail", msgJson);

  sendToBroker("report/sensorStatus", "OK");

  // raw = cuentas de 1/16 °C del scratchpad. Sirve para distinguir "el sensor no
  // se mueve ni un LSB" (problema físico o de cableado) de "se mueve menos de lo
  // que muestra el entero". Con 12 bits, 1 LSB = 0.0625 °C.
  Serial.printf("Temperature: %d°C (%.2f°C) raw=%d OK\n",
                (int)lroundf(t), t, (int)(t * 16.0f));
}

// ─── Power control ──────────────────────────────────────────────────
void turnOn() {
  sensorOn = true;
  sendToBroker("report/powerState", "ON");
  sendToBroker("report/humidity", "N/A");
  Serial.println(F("Sensor ON"));
}

void turnOff() {
  sensorOn = false;
  sendToBroker("report/powerState", "OFF");
  Serial.println(F("Sensor OFF"));
}

// ═══════════════════════════════════════════════════════════════════
//  MQTT CALLBACK
// ═══════════════════════════════════════════════════════════════════
int splitTopic(char* topic, char* tokens[], int tokensNumber) {
  const char s[2] = "/";
  int pos = 0;
  tokens[0] = strtok(topic, s);
  while (pos < tokensNumber - 1 && tokens[pos] != NULL) {
    pos++;
    tokens[pos] = strtok(NULL, s);
  }
  return pos;
}

void callback(char* topic, byte* payload, unsigned int length) {
  blinkMessageReceived();
  mqttPublishFailCount = 0;

  // Buffer fijo: v1 usaba un VLA dimensionado por el payload remoto
  char message[MSG_MAX];
  unsigned int n = (length < MSG_MAX - 1) ? length : MSG_MAX - 1;
  memcpy(message, payload, n);
  message[n] = '\0';

  int tokensNumber = 10;
  char* tokens[tokensNumber];
  splitTopic(topic, tokens, tokensNumber);

  if (tokens[1] != NULL && tokens[2] != NULL &&
      strcmp(tokens[1], "directive") == 0 && strcmp(tokens[2], "powerState") == 0) {
    if (strcmp(message, "ON") == 0) turnOn();
    else if (strcmp(message, "OFF") == 0) turnOff();
  }
}

// ═══════════════════════════════════════════════════════════════════
//  LED
// ═══════════════════════════════════════════════════════════════════
void blinkLed() { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); }

void blinkMessageReceived() {
  blinkHeartbeat.detach();
  digitalWrite(LED_PIN, LOW);  delay(50);
  digitalWrite(LED_PIN, HIGH); delay(50);
  digitalWrite(LED_PIN, LOW);  delay(50);
  digitalWrite(LED_PIN, HIGH);
  blinkHeartbeat.attach(20, blinkLed);
}

// ═══════════════════════════════════════════════════════════════════
//  WIFI
// ═══════════════════════════════════════════════════════════════════
void startWifi() {
  delay(10); Serial.println();
  blinkHeartbeat.detach();
  blinkStatus.attach(0.1, blinkLed);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to "); Serial.println(WIFI_SSID);
  wifiConnectRetries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250); Serial.print('.'); ESP.wdtFeed();
    wifiConnectRetries++;
    if (wifiConnectRetries >= MAX_WIFI_RETRIES) {
      // Imprime el estado real: con solo "timeout" no se sabe si es AP
      // invisible, contrasena incorrecta o roaming entre dos APs del mismo SSID
      wl_status_t st = WiFi.status();
      Serial.printf("\nWiFi timeout. status=%d", (int)st);
      switch (st) {
        case WL_NO_SSID_AVAIL:   Serial.print(F(" WL_NO_SSID_AVAIL (red no visible)")); break;
        case WL_CONNECT_FAILED:  Serial.print(F(" WL_CONNECT_FAILED (contrasena incorrecta)")); break;
        case WL_CONNECTION_LOST: Serial.print(F(" WL_CONNECTION_LOST (se cae el enlace)")); break;
        case WL_DISCONNECTED:    Serial.print(F(" WL_DISCONNECTED")); break;
        default:                 Serial.print(F(" (sin asociar)"));
      }
      Serial.println(F("\nRebooting..."));
      rebootWithNewMac();
    }
  }
  Serial.println();
  Serial.print("Connected: "); Serial.println(WiFi.SSID());
  Serial.print("IP: "); Serial.println(WiFi.localIP());
  Serial.printf("RSSI: %d dBm   Canal: %d\n", WiFi.RSSI(), WiFi.channel());
  digitalWrite(LED_PIN, LOW);
  blinkStatus.attach(0.5, blinkLed);
  wifiConnectRetries = 0;
}

// ═══════════════════════════════════════════════════════════════════
//  MQTT
// ═══════════════════════════════════════════════════════════════════
void checkMqtt() {
  if (!client.connected()) {
    Serial.println(F("MQTT disconnected, reconnecting..."));
    if (WiFi.status() != WL_CONNECTED) { wifiConnectRetries = 0; startWifi(); }
    blinkStatus.attach(0.5, blinkLed);
    blinkHeartbeat.detach();
    lastReportAt = millis();
    lastPollAt = millis();
    lastGoodPublish = millis();
    startMqtt();
  }
}

void startMqtt() {
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setBufferSize(256);          // payload JSON completo
  client.setCallback(callback);
  unsigned long start = millis();
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqttPublishFailCount = 0;
      digitalWrite(LED_PIN, HIGH);
      blinkStatus.detach();
      blinkHeartbeat.attach(20, blinkLed);
      lastReportAt = millis();
      lastPollAt = millis();
    } else {
      if (client.state() == 5) {
        Serial.println(F("MQTT auth error. Check credentials/ClientId"));
      } else {
        Serial.print("MQTT error: "); Serial.println(client.state());
      }
      if (millis() - start > MQTT_CONNECT_TIMEOUT) {
        Serial.println(F("MQTT timeout. Rebooting..."));
        rebootWithNewMac();
      }
      // delay() enzamizados: el WDT real corta a los 8 s, un delay(10000) lo mataria
      for (uint8_t i = 0; i < 10; i++) { delay(1000); ESP.wdtFeed(); }
    }
  }
  char sub[100]; snprintf(sub, sizeof(sub), "%s/#", MQTT_CLIENT);
  if (client.subscribe(sub)) {
    Serial.println(F("Subscribed to <client>/#"));
  } else {
    Serial.println(F("!! MQTT subscribe failed"));
  }

  lastGoodPublish = millis();
  sendToBroker("report/online", "true");
  sendToBroker("report/humidity", "N/A");       // el DS18B20 no mide humedad
  // Reporte inmediato: se marca el intervalo como ya cumplido para que el
  // scheduler de loop() dispare la conversion dentro de los primeros 100 ms
  lastReportAt = millis() - (REPORT_INTERVAL * 1000UL);
  requestTemperature();
}

void sendToBroker(const char* topic, const char* message) {
  if (client.connected()) {
    char t[128];
    snprintf(t, sizeof(t), "%s/%s", MQTT_CLIENT, topic);
    if (client.publish(t, message)) {
      mqttPublishFailCount = 0;
      lastGoodPublish = millis();      // el watchdog de "sin reportar" se reinicia
    } else {
      mqttPublishFailCount++;
      if (mqttPublishFailCount >= MAX_MQTT_FAILURES) {
        Serial.println(F("Too many MQTT failures. Rebooting..."));
        rebootWithNewMac();
      }
    }
  } else {
    mqttPublishFailCount++;
    if (mqttPublishFailCount >= MAX_MQTT_FAILURES) {
      Serial.println(F("MQTT offline. Rebooting..."));
      rebootWithNewMac();
    }
  }
}

void sendOnlineReport() { sendToBroker("report/online", "true"); }

// ═══════════════════════════════════════════════════════════════════
//  RESILIENCE
// ═══════════════════════════════════════════════════════════════════
void randomizeMac() {
  WiFi.disconnect();
  uint8_t mac[6]; WiFi.macAddress(mac);
  uint8_t old = mac[5];
  mac[5] = random(0x01, 0xFE);
  wifi_set_macaddr(STATION_IF, mac);
  Serial.printf("MAC: %02x -> %02x\n", old, mac[5]);
}

void rebootWithNewMac() {
  rtcData_t rtc; rtc.magic = 0xDEAD; rtc.failureFlag = 1;
  ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtc, sizeof(rtc));
  delay(100); ESP.restart();
}

// ═══════════════════════════════════════════════════════════════════
//  CONECTIVIDAD — ICMP real a 8.8.8.8
//  5 probes por ronda; si ninguno responde se reinicia el equipo.
// ═══════════════════════════════════════════════════════════════════

// Callbacks del SDK de ping: corren en la tarea de ping, no en loop().
// Solo levantan flags; la decision se toma en servicePing().
void onPingRecv(void* arg, void* pdata) {
  (void)arg;
  ping_resp* r = (ping_resp*)pdata;
  if (r == NULL) return;

  pingLast = *r;                       // resumen: total_count / timeout_count
  if (r->ping_err == 0) {
    pingReplies++;
  }
  // El SDK entrega el resumen cuando termina la ronda
  if (r->total_count >= PING_MAX_TRIES || r->timeout_count >= PING_MAX_TRIES) {
    pingRoundDone = true;
  }
}

void onPingSent(void* arg, void* pdata) {
  (void)arg; (void)pdata;
}

void startPingRound() {
  pingReplies = 0;
  pingRoundDone = false;
  memset(&pingLast, 0, sizeof(pingLast));

  pingOpt.count       = PING_MAX_TRIES;   // el SDK encadena los 5 probes
  pingOpt.ip          = PING_TARGET_IP;    // 8.8.8.8
  pingOpt.coarse_time = PING_GAP_MS;       // 2 s entre probes
  pingOpt.recv_function = onPingRecv;
  pingOpt.sent_function = onPingSent;
  pingOpt.reverse     = NULL;

  if (ping_start(&pingOpt)) {
    pingRoundActive = true;
    pingRoundStart  = millis();
  } else {
    Serial.println(F("ping_start fallo"));
  }
  ESP.wdtFeed();
}

// Ronda de conectividad. No bloquea.
void servicePing() {
  uint32_t now = millis();
  uint32_t elapsed = now - pingRoundStart;

  if (pingRoundActive) {
    bool roundOver = pingRoundDone;
    // Margen: 5 probes x 2 s + 3 s de timeout por probe + holgura
    if (!roundOver && elapsed >= PING_ROUND_MAX_MS) { roundOver = true; }
    if (!roundOver) return;                       // ronda en curso

    pingRoundActive = false;
    // OJO: timeout_count no lo rellena el SDK de forma fiable — llega en 0
    // aunque no llegue ninguna respuesta). La unica senal valida es quantas
    // respuestas/entregó recv_function.
    bool ok = (pingReplies > 0);
    if (ok) {
      Serial.printf("Ping 8.8.8.8: OK (%lu respuesta(s))\n", (unsigned long)pingReplies);
    } else {
      Serial.printf("Ping 8.8.8.8: FALLO (0/%d respondieron)\n", PING_MAX_TRIES);
    }

    if (ok) {
      pingNextTryAt = now + (PING_INTERVAL * 1000UL);
      return;
    }

    // Ninguno de los 5 respondio.
    // Verificacion secundaria: si el broker responde, la red esta viva y el
    // problema es solo ICMP filtrado hacia afuera -> no reiniciar, para no
    // entrar en loop de reinicios.
    if (brokerReachable()) {
      Serial.println(F("Broker MQTT responde: no se reinicia (ICMP filtrado)"));
      pingNextTryAt = now + (PING_INTERVAL * 1000UL);
    } else {
      Serial.println(F("Broker MQTT tampoco responde. Reiniciando..."));
      rebootWithNewMac();
    }
    return;
  }

  // ¿Toca abrir una ronda nueva?
  if ((int32_t)(now - pingNextTryAt) >= 0) {
    startPingRound();
  }
}

// Chequeo TCP al broker: confirma que la red local y la salida a internet
// funcionan. OJO: el timeout debe ser holgado; con 3 ms el handshake TCP nunca
// alcanza a completarse y esto siempre devolvia false.
bool brokerReachable() {
  WiFiClient tc;
  tc.setTimeout(2000);
  bool ok = tc.connect(MQTT_BROKER, MQTT_PORT);
  if (ok) tc.stop();
  ESP.wdtFeed();
  return ok;
}

// Red de seguridad final: si el equipo lleva STALE_REPORT_MS sin publicar nada
// es que esta mudo aunque el WDT no haya disparado (broker caido, publishes
// siempre fallando, MQTT reconectando en bucle). Se reinicia.
void checkStaleReport() {
  if (!sensorOn) return;              // con el sensor OFF no se publica por diseno
  if ((uint32_t)(millis() - lastGoodPublish) < STALE_REPORT_MS) return;

  Serial.printf("Sin reporte correcto hace %lus. Reiniciando...\n",
                STALE_REPORT_MS / 1000);
  rebootWithNewMac();
}
