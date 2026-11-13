// AirSensor-8266 — MQ135 + direct WiFi + resilience
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>
#include "secrets.h"

extern "C" {
  #include <user_interface.h>
}

WiFiClient espClient;
PubSubClient client(espClient);
Ticker blinkStatus, blinkHeartbeat, sendUpdate, pingCheck, wdtTicker;

#define MQ135_PIN  A0
#define LED_PIN    2
#define REPORT_S   5
#define PING_MS    30000
#define WDT_MS     180000
#define MAX_WIFI   40
#define MAX_MQTT   10
#define MAX_PING   20
#define TH_LOW     130
#define TH_HIGH    700

int mqttFail=0, wifiRetries=0, pingFail=0, adecuado=0, alerta=0, flag=0;
volatile bool shouldPing=false, wdtTriggered=false;
unsigned long tStart=0;
bool sensorOn=true;
typedef struct { uint32_t magic; uint16_t flag; } rtcData_t;

void IRAM_ATTR setPingFlag() { shouldPing=true; }
void IRAM_ATTR wdtCallback() { wdtTriggered=true; }
void feedWatchdog() { wdtTicker.once_ms(WDT_MS,wdtCallback); }
void blinkLed() { digitalWrite(LED_PIN,!digitalRead(LED_PIN)); }
void randomizeMac(){WiFi.disconnect();uint8_t m[6];WiFi.macAddress(m);m[5]=random(0x01,0xFE);wifi_set_macaddr(STATION_IF,m);}
void rebootNewMac(){rtcData_t r;r.magic=0xDEAD;r.flag=1;ESP.rtcUserMemoryWrite(0,(uint32_t*)&r,sizeof(r));delay(100);ESP.restart();}

void sendToBroker(char* t, char* m) {
  if(client.connected()){char b[128];sprintf(b,"%s/%s",MQTT_CLIENT,t);if(client.publish(b,m))mqttFail=0;else{mqttFail++;if(mqttFail>=MAX_MQTT)rebootNewMac();}}
  else{mqttFail++;if(mqttFail>=MAX_MQTT)rebootNewMac();}
}

void sendAirValue() {
  int raw=analogRead(MQ135_PIN); char msg[8]; sprintf(msg,"%d",raw);
  sendToBroker("report/value",msg);
}

void turnOn(){sensorOn=true;sendToBroker("report/powerState","ON");}
void turnOff(){sensorOn=false;sendToBroker("report/powerState","OFF");}

void callback(char* topic, byte* payload, unsigned int length) {
  char msg[length+1]; memcpy(msg,payload,length); msg[length]=0;
  if(strstr(topic,"/directive/powerState")){if(!strcmp(msg,"ON"))turnOn();else if(!strcmp(msg,"OFF"))turnOff();}
}

void startWifi() {
  delay(10);Serial.println();blinkStatus.attach(0.1,blinkLed);
  WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
  wifiRetries=0;
  while(WiFi.status()!=WL_CONNECTED){delay(250);Serial.print('.');feedWatchdog();if(++wifiRetries>=MAX_WIFI){rebootNewMac();}}
  Serial.printf("\nIP:%s\n",WiFi.localIP().toString().c_str());
  digitalWrite(LED_PIN,LOW);blinkStatus.attach(0.5,blinkLed);wifiRetries=0;
}

void checkMqtt() {
  if(!client.connected()){
    if(WiFi.status()!=WL_CONNECTED)startWifi();
    blinkHeartbeat.detach();sendUpdate.detach();
    client.setServer(MQTT_BROKER,MQTT_PORT);client.setCallback(callback);
    unsigned long s=millis();
    while(!client.connected()){
      if(client.connect(MQTT_CLIENT,MQTT_USERNAME,MQTT_PASSWORD)){mqttFail=0;digitalWrite(LED_PIN,HIGH);delay(20);blinkHeartbeat.attach(20,blinkLed);sendUpdate.attach(REPORT_S,sendAirValue);}
      else{if(millis()-s>120000)rebootNewMac();feedWatchdog();delay(10000);}
    }
    char sub[128];sprintf(sub,"%s/#",MQTT_CLIENT);client.subscribe(sub);
    sendToBroker("report/online","true");
  }
}

void checkPing() {
  feedWatchdog();WiFiClient tc;tc.setTimeout(5);
  if(tc.connect(IPAddress(8,8,8,8),53)){tc.stop();pingFail=0;}else if(++pingFail>=MAX_PING){pingCheck.detach();wdtTicker.detach();ESP.restart();}
}

void setup() {
  pinMode(LED_PIN,OUTPUT);digitalWrite(LED_PIN,HIGH);
  Serial.begin(115200);delay(1000);randomSeed(ESP.getCycleCount());
  rtcData_t r;ESP.rtcUserMemoryRead(0,(uint32_t*)&r,sizeof(r));
  if(r.magic==0xDEAD&&r.flag==1){randomizeMac();r.flag=0;ESP.rtcUserMemoryWrite(0,(uint32_t*)&r,sizeof(r));}
  Serial.println("\n=== AirSensor MQ135 ===");
  startWifi();checkMqtt();
  pingCheck.attach_ms(PING_MS,setPingFlag);feedWatchdog();
}

void loop() {
  if(wdtTriggered)ESP.restart();feedWatchdog();
  if(shouldPing){shouldPing=false;checkPing();}
  if(WiFi.status()!=WL_CONNECTED)startWifi();
  checkMqtt();client.loop();

  if(adecuado==2){sendToBroker("report/airState","adecuado");adecuado=0;alerta=0;}
  if(alerta==3){sendToBroker("report/airState","alerta");alerta=0;}

  int val=analogRead(MQ135_PIN);
  if(val<TH_LOW&&!flag){tStart=millis();flag=1;}
  if(analogRead(MQ135_PIN)>TH_HIGH&&flag){
    unsigned long e=millis()-tStart;
    if(e>600&&e<900)adecuado++;else if(e>800&&e<1500)alerta++;else adecuado=0;
    flag=0;
  }
  delay(50);
}
