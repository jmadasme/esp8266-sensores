# Referencias · ESP8266 Sensores

Documentación externa que se usó para entender el hardware, los protocolos y las
libertades del proyecto. No es una bibliografía decorativa: cada enlace apunta a
la fuente que explica una decisión concreta.

## Fuentes técnicas

| Fuente | Para qué se consultó |
|---|---|
| [arduino-esp8266](https://arduino-esp8266.readthedocs.io/en/latest/) | Core Arduino para ESP8266. |
| [MQTT](https://mqtt.org/) | Especificación y ecosistema del protocolo. |
| [HC-SR501](https://www.adafruit.com/product/189) | Módulo PIR y su operación. |
| [DHT22](https://www.adafruit.com/product/385) | Sensor de temperatura y humedad. |
| [DS18B20](https://www.analog.com/en/products/ds18b20.html) | Sensor de temperatura 1-Wire. |
| [Arduino Documentation](https://docs.arduino.cc/) | Referencia de placas, IDE y API. |

## Versiones de este repositorio

`AirSensor-8266`, `DHT22`, `DoorSensor-8266`, `LCD1602_I2C`, `Ldr-8266`, `ReedSwitchTester`, `SensorPIR-8266`, `TempSensor-8266`, `random-serial`, `sensorudp`, `port_scanner`

Cada carpeta puede tener dependencias y versiones distintas. Cuando el código
depende de una versión específica, esa versión está declarada en el README o en
el archivo de configuración de la carpeta, no en este documento.

## Criterio de preservación

- El código y los documentos se copiaron desde la carpeta original de trabajo.
- Se excluyeron `.git`, dependencias instaladas, compilados, registros y bases
  de datos locales.
- Se redactaron tokens, claves privadas y contraseñas detectadas en texto.
- Las plantillas `.example`, `.sample` y los valores de demostración se
  conservan porque documentan la configuración esperada.

## Limitaciones conocidas

Los sensores baratos se conectan fácil, pero fallan en silencio: se cae el WiFi, el watchdog no existe y el dato viejo parece actual.

La documentación de cada versión describe el alcance real. Si una función no
está implementada, se dice; si un hardware no fue probado, también.
