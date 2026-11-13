// DoorSensor-8266 — Direct WiFi Nv2 + reed + MQTT + resilience
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include "secrets.h"

extern "C" {
  #include <user_interface.h>
}

WiFiClient espClient;
PubSubClient client(espClient);
Ticker reedCheck, blinkStatus, blinkHeartbeat, pingCheck, wdtTicker;

#define REED_PIN   4
#define LED_PIN    2
#define REED_MS    500
#define PING_MS    30000
#define WDT_MS     180000
#define MAX_WIFI   40
#define MAX_MQTT   10
#define MAX_PING   20

int mqttFail = 0, wifiRetries = 0, pingFail = 0;
volatile bool shouldPing = false, wdtTriggered = false;
bool lastLocked = true;
typedef struct { uint32_t magic; uint16_t flag; } rtcData_t;

void IRAM_ATTR setPingFlag() { shouldPing = true; }
void IRAM_ATTR wdtCallback() { wdtTriggered = true; }
void feedWatchdog() { wdtTicker.once_ms(WDT_MS, wdtCallback); }
void blinkLed() { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); }

void randomizeMac() { WiFi.disconnect(); uint8_t m[6]; WiFi.macAddress(m); m[5]=random(0x01,0xFE); wifi_set_macaddr(STATION_IF,m); }
void rebootNewMac() { rtcData_t r; r.magic=0xDEAD; r.flag=1; ESP.rtcUserMemoryWrite(0,(uint32_t*)&r,sizeof(r)); delay(100); ESP.restart(); }

void sendToBroker(char* t, char* m) {
  if(client.connected()){char b[128];sprintf(b,"%s/%s",MQTT_CLIENT,t);if(client.publish(b,m))mqttFail=0;else{mqttFail++;if(mqttFail>=MAX_MQTT)rebootNewMac();}}
  else{mqttFail++;if(mqttFail>=MAX_MQTT)rebootNewMac();}
}

void checkReed() {
  bool l=(digitalRead(REED_PIN)==LOW);
  if(l!=lastLocked){lastLocked=l;Serial.println(l?"CLOSED":"OPEN");if(l)sendToBroker("report/lockedState","true");else sendToBroker("report/lockedState","false");}
}

void callback(char* topic, byte* payload, unsigned int length) {
  char msg[length+1]; memcpy(msg,payload,length); msg[length]=0;
  if(strstr(topic,"/directive/powerState")){if(!strcmp(msg,"ON"))sendToBroker("report/powerState","ON");else if(!strcmp(msg,"OFF"))sendToBroker("report/powerState","OFF");}
  else if(strstr(topic,"/directive/lockedState")){if(!strcmp(msg,"false"))sendToBroker("report/lockedState","false");else if(!strcmp(msg,"true"))sendToBroker("report/lockedState","true");}
}

void startWifi() {
  delay(10); Serial.println(); blinkStatus.attach(0.1,blinkLed);
  WiFi.persistent(false); WiFi.mode(WIFI_STA); WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  wifiRetries=0;
  while(WiFi.status()!=WL_CONNECTED){delay(250);Serial.print('.');feedWatchdog();if(++wifiRetries>=MAX_WIFI){Serial.println("FAIL");rebootNewMac();}}
  Serial.printf("\nIP:%s\n",WiFi.localIP().toString().c_str());
  digitalWrite(LED_PIN,LOW); blinkStatus.attach(0.5,blinkLed); wifiRetries=0;
}

void checkMqtt() {
  if(!client.connected()){
    if(WiFi.status()!=WL_CONNECTED)startWifi();
    blinkHeartbeat.detach();
    client.setServer(MQTT_BROKER,MQTT_PORT);client.setCallback(callback);
    unsigned long s=millis();
    while(!client.connected()){
      if(client.connect(MQTT_CLIENT,MQTT_USERNAME,MQTT_PASSWORD)){mqttFail=0;digitalWrite(LED_PIN,HIGH);delay(20);blinkHeartbeat.attach(20,blinkLed);}
      else{if(millis()-s>120000)rebootNewMac();feedWatchdog();delay(10000);}
    }
    char sub[128];sprintf(sub,"%s/#",MQTT_CLIENT);client.subscribe(sub);
    sendToBroker("report/online","true");sendToBroker("report/powerState","ON");checkReed();
  }
}

void checkPing() {
  feedWatchdog(); WiFiClient tc; tc.setTimeout(5);
  if(tc.connect(IPAddress(8,8,8,8),53)){tc.stop();pingFail=0;}else if(++pingFail>=MAX_PING){pingCheck.detach();wdtTicker.detach();ESP.restart();}
}

void setup() {
  pinMode(REED_PIN,INPUT_PULLUP);pinMode(LED_PIN,OUTPUT);digitalWrite(LED_PIN,HIGH);
  Serial.begin(115200);randomSeed(ESP.getCycleCount());
  rtcData_t r;ESP.rtcUserMemoryRead(0,(uint32_t*)&r,sizeof(r));
  if(r.magic==0xDEAD&&r.flag==1){randomizeMac();r.flag=0;ESP.rtcUserMemoryWrite(0,(uint32_t*)&r,sizeof(r));}
  lastLocked=(digitalRead(REED_PIN)==LOW);
  startWifi();checkMqtt();
  reedCheck.attach_ms(REED_MS,checkReed);
  pingCheck.attach_ms(PING_MS,setPingFlag);feedWatchdog();
  Serial.printf("Ready. Reed=%s\n",lastLocked?"CLOSED":"OPEN");
}

void loop() {
  if(wdtTriggered)ESP.restart();feedWatchdog();
  if(shouldPing){shouldPing=false;checkPing();}
  if(WiFi.status()!=WL_CONNECTED)startWifi();
  checkMqtt();client.loop();
}
