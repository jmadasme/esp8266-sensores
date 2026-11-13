// SensorPIR-8266 — Motion Sensor for NodeMCU ESP8266
// Base: lp101_secure architecture (watchdog, ping, MAC randomization)
// PIR detection: report/detectionState via MQTT (reference pattern)

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include <Arduino.h>
#include "secrets.h"

extern "C" {
  #include <user_interface.h>
}

// ─── Pins ──────────────────────────────────────────────────────────
#define PIR_PIN   4    // D2 (GPIO4) — PIR HC-SR501 OUT
#define LED_PIN   2    // D4 (GPIO2) — LED_BUILTIN

// ─── Timing ────────────────────────────────────────────────────────
#define PIR_WARMUP_MS        65000   // 65s
#define SENSOR_REPORT_MS     200     // min interval between reports
#define REPORT_INTERVAL      30      // online heartbeat (seconds)
#define PING_INTERVAL        30
#define WDT_TIMEOUT          180
#define MAX_WIFI_RETRIES     40
#define MAX_MQTT_FAILURES    10
#define MAX_PING_FAILS       20

// ─── Globals ───────────────────────────────────────────────────────
WiFiClient espClient;
PubSubClient client(espClient);
Ticker blinkStatus;
Ticker blinkHeartbeat;
Ticker sendUpdate;
Ticker pingCheck;
Ticker wdtTicker;

int mqttPublishFailCount = 0;
int wifiConnectRetries = 0;
int pingFailCount = 0;
volatile bool shouldPing = false;
volatile bool wdtTriggered = false;

// ─── PIR state ─────────────────────────────────────────────────────
bool sensorOn = true;
bool sensorTriggered = false;
unsigned long sensorReportSent = 0;
unsigned long warmupStart = 0;
bool pirReady = false;

// ─── RTC data ──────────────────────────────────────────────────────
typedef struct {
  uint32_t magic;
  uint16_t failureFlag;
} rtcData_t;

// ─── Declarations ──────────────────────────────────────────────────
void blinkLed();
void blinkMessageReceived();
void sendOnlineReport();
void startWifi();
void startMqtt();
void checkMqtt();
void checkSensor();
void sendReport(bool value);
void sendToBroker(char* topic, char* message);
void turnOn();
void turnOff();
int splitTopic(char* topic, char* tokens[], int tokensNumber);
void callback(char* topic, byte* payload, unsigned int length);
void randomizeMac();
void rebootWithNewMac();
void checkInternetConnectivity();
void feedWatchdog();

// ─── ISRs ──────────────────────────────────────────────────────────
void IRAM_ATTR setPingFlag() { shouldPing = true; }
void IRAM_ATTR wdtCallback() { wdtTriggered = true; }

// ═══════════════════════════════════════════════════════════════════
void setup() {
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.begin(115200);

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
  pingFailCount = 0;

  startWifi();
  startMqtt();

  pingCheck.attach(PING_INTERVAL, setPingFlag);
  feedWatchdog();

  warmupStart = millis();
  Serial.println(F("SensorPIR-8266 booted"));
  Serial.println(F("  PIR: D2/GPIO4 | MQTT: report/detectionState, report/powerState"));
  Serial.println(F("  Warmup: 65s..."));
}

// ═══════════════════════════════════════════════════════════════════
void loop() {
  if (wdtTriggered) { Serial.println(F("Watchdog. Rebooting...")); ESP.restart(); }
  feedWatchdog();
  if (shouldPing) { shouldPing = false; checkInternetConnectivity(); }
  if (WiFi.status() != WL_CONNECTED) { wifiConnectRetries = 0; startWifi(); }
  checkMqtt();
  checkSensor();
  client.loop();
}

// ═══════════════════════════════════════════════════════════════════
//  PIR SENSOR (reference pattern)
// ═══════════════════════════════════════════════════════════════════
void checkSensor() {
  if (!pirReady) {
    if (millis() - warmupStart >= PIR_WARMUP_MS) {
      pirReady = true;
      Serial.println(F("PIR ready"));
    }
    return;
  }
  int state = digitalRead(PIR_PIN);
  if (state == LOW && !sensorTriggered) {
    return;
  } else if (state == LOW && sensorTriggered) {
    sensorTriggered = false;
    sendReport(false);
  } else if (state == HIGH && !sensorTriggered) {
    sensorTriggered = true;
    sendReport(true);
  }
  // state == HIGH && sensorTriggered → nothing to do
}

void sendReport(bool value) {
  if (millis() - sensorReportSent > SENSOR_REPORT_MS && sensorOn) {
    if (value) {
      sendToBroker("report/detectionState", "true");
      Serial.println(F(">>> MOTION DETECTED <<<"));
    } else {
      sendToBroker("report/detectionState", "false");
      Serial.println(F("--- motion cleared ---"));
    }
    sensorReportSent = millis();
  }
}

void turnOn() {
  sensorOn = true;
  sendToBroker("report/powerState", "ON");
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
  int tokensNumber = 10;
  char* tokens[tokensNumber];
  char message[length + 1];
  splitTopic(topic, tokens, tokensNumber);
  sprintf(message, "%c", (char)payload[0]);
  for (unsigned int i = 1; i < length; i++) {
    sprintf(message, "%s%c", message, (char)payload[i]);
  }
  // directive/powerState
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
  digitalWrite(LED_PIN, LOW);  delay(50);
  digitalWrite(LED_PIN, HIGH); delay(50);
  digitalWrite(LED_PIN, LOW);  delay(50);
  digitalWrite(LED_PIN, HIGH);
}

// ═══════════════════════════════════════════════════════════════════
//  WIFI (lp101_secure direct)
// ═══════════════════════════════════════════════════════════════════
void startWifi() {
  delay(10); Serial.println();
  blinkStatus.attach(0.1, blinkLed);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to "); Serial.println(WIFI_SSID);
  wifiConnectRetries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(250); Serial.print('.'); feedWatchdog();
    wifiConnectRetries++;
    if (wifiConnectRetries >= MAX_WIFI_RETRIES) {
      Serial.println(F("\nWiFi timeout. Rebooting..."));
      rebootWithNewMac();
    }
  }
  Serial.println();
  Serial.print("Connected: "); Serial.println(WiFi.SSID());
  Serial.print("IP: "); Serial.println(WiFi.localIP());
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
    sendUpdate.detach();
    startMqtt();
  }
}

void startMqtt() {
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
  unsigned long start = millis();
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect(MQTT_CLIENT, MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqttPublishFailCount = 0;
      digitalWrite(LED_PIN, HIGH);
      blinkStatus.attach(20, blinkLed); delay(20);
      blinkHeartbeat.attach(20, blinkLed);
      sendUpdate.attach(REPORT_INTERVAL, sendOnlineReport);
    } else {
      if (client.state() == 5) {
        Serial.println(F("MQTT auth error. Check credentials/ClientId"));
      } else {
        Serial.print("MQTT error: "); Serial.println(client.state());
      }
      if (millis() - start > 120000) {
        Serial.println(F("MQTT timeout. Rebooting..."));
        rebootWithNewMac();
      }
      feedWatchdog(); delay(10000);
    }
  }
  char sub[100]; sprintf(sub, "%s/#", MQTT_CLIENT);
  client.subscribe(sub);
  sendToBroker("report/online", "true");
}

void sendToBroker(char* topic, char* message) {
  if (client.connected()) {
    char t[128]; sprintf(t, "%s/%s", MQTT_CLIENT, topic);
    if (client.publish(t, message)) {
      mqttPublishFailCount = 0;
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
//  RESILIENCE (lp101_secure)
// ═══════════════════════════════════════════════════════════════════
void randomizeMac() {
  WiFi.disconnect();
  uint8_t mac[6]; WiFi.macAddress(mac);
  uint8_t old = mac[5];
  mac[5] = random(0x01, 0xFE);
  wifi_set_macaddr(STATION_IF, mac);
  Serial.printf("MAC: %02x → %02x\n", old, mac[5]);
}

void rebootWithNewMac() {
  rtcData_t rtc; rtc.magic = 0xDEAD; rtc.failureFlag = 1;
  ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtc, sizeof(rtc));
  delay(100); ESP.restart();
}

void feedWatchdog() { wdtTicker.once(WDT_TIMEOUT, wdtCallback); }

void checkInternetConnectivity() {
  feedWatchdog();
  WiFiClient tc; tc.setTimeout(5);
  if (tc.connect(IPAddress(8, 8, 8, 8), 53)) { tc.stop(); pingFailCount = 0; }
  else {
    pingFailCount++;
    if (pingFailCount >= MAX_PING_FAILS) {
      Serial.println(F("20 ping fails. Rebooting..."));
      pingCheck.detach(); wdtTicker.detach(); ESP.restart();
    }
  }
}
