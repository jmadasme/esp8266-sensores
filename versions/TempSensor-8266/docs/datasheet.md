# TempSensor-8266 — Datasheet de usuario

## 1. Qué es

Un nodo de sensado de temperatura para Nurses/plantas/equipos, basado en NodeMCU ESP8266
con un sensor digital DS18B20. Lee la temperatura y la publica por MQTT cada 60 segundos.

**No mide humedad.** El DS18B20 es un sensor de temperatura puro.

## 2. Especificaciones

| Parámetro | Valor |
|-----------|-------|
| Rango de temperatura | -55 °C a +125 °C |
| Precisión | ±0.5 °C entre -10 °C y +85 °C |
| Resolución | 12 bits = 0.0625 °C |
| Transmisión | MQTT 3.1.1, cada 60 s |
| Alimentación | 5 V por USB, o 3.3 V con fuente externa |
| Consumo | ~70 mA |

## 3. Hardware necesario

- NodeMCU ESP8266 (ESP-12E)
- DS18B20 en encapsulado TO-92 o sonda impermeable
- Resistencia 4.7 kΩ
- Protoboard y cables

## 4. Montaje

1. Con la placa **desconectada del USB**, conectar el DS18B20:

| DS18B20 | NodeMCU |
|---------|---------|
| VDD (rojo) | 3.3V |
| DATA (amarillo) | D2 (GPIO4) |
| GND (negro) | GND |

2. Conectar la resistencia 4.7 kΩ entre **DATA y VDD**. Sin ella el bus OneWire no funciona
   y el sensor no aparecerá.

3. En una sonda waterproof, distinguir los tres cables: el negro es GND y suele ser el más
   grueso o el que va por fuera de la funda metálica.

## 5. Puesta en marcha

```bash
# 1. Preparar credenciales
cp secrets.example.h secrets.h
$EDITOR secrets.h

# 2. Detectar la placa y subir el firmware
./scripts/detectar.sh
./scripts/subir.sh

# 3. Ver el log
./scripts/monitor.sh /dev/ttyUSB0 60
```

## 6. Interpretar el LED (D4, activo bajo)

| Patrón | Significado |
|--------|-------------|
| Parpadeo doble | MQTT conectado, reportes normales |
| Parpadeo simple | Conectado a WiFi, todavía negociando con el broker |
| Apagado fijo | Sin conexión o sensor desactivado |

## 7. Topics MQTT

Sustituir `<CLIENT_ID>` por el valor de `MQTT_CLIENT` en `secrets.h`.

| Topic | Sentido | Payload | Frecuencia |
|-------|---------|---------|-----------|
| `<CLIENT_ID>/report/temperature` | salida | Entero °C, ej. `"18"` | 60 s |
| `<CLIENT_ID>/report/temperatureDetail` | salida | JSON `t_c`, `t_f`, `rssi`, `heap`, `uptime` | 60 s |
| `<CLIENT_ID>/report/sensorStatus` | salida | `OK`, `DISCONNECTED`, `CONVERSION_TIMEOUT` | 60 s |
| `<CLIENT_ID>/report/humidity` | salida | `"N/A"` siempre | al conectar |
| `<CLIENT_ID>/report/online` | salida | `"true"` | 60 s |
| `<CLIENT_ID>/report/powerState` | salida | `"ON"` / `"OFF"` | al cambiar |
| `<CLIENT_ID>/directive/powerState` | entrada | `"ON"` / `"OFF"` | — |

## 8. Comandos

```bash
mosquitto_pub -h www.smartage.cl -p 1886 -u TU_USUARIO -P TU_PASSWORD \
  -t "<CLIENT_ID>/directive/powerState" -m "OFF"
```

Con el sensor en `OFF` el equipo deja de publicar temperatura. **No se reinicia por
ausencia de reportes mientras esté en `OFF`** — el guard de 5 minutos solo aplica cuando el
sensor está activo.

## 9. Supervisión automática

| Situación | Reacción |
|-----------|---------|
| El loop principal se cuelga | Reinicio por watchdog de hardware a los 8 s |
| Sin internet (ICMP y broker caídos) | Reinicio con nueva MAC tras 5 intentos |
| ICMP filtrado pero broker accesible | No reinicia — es comportamiento normal de la red |
| Sensor `ON` y 5 min sin publicar | Reinicio |
| WiFi no conecta tras 120 intentos | Reinicio con nueva MAC |

En condiciones normales **el equipo no debería reiniciarse nunca**. Si hay reinicios
frecuentes, revisar el log serie: `Ping ...: FALLO` seguido de `Broker MQTT responde: no`
indica pérdida de red real, no un problema del firmware.

## 10. Problemas frecuentes

| Síntoma | Causa | Acción |
|---------|-------|--------|
| `DS18B20 sensors found: 0` | Falta la resistencia, o cableado incorrecto | Revisar DATA/GND/VDD y la 4.7 kΩ |
| Lecturas absurdas (`85`, `-127`) | Ruido en el bus OneWire | Cable lo más corto posible, resistencia cerca del sensor |
| `MQTT auth error` en bucle | ClientId no registrado, o usuario/contraseña incorrectos | Este broker exige las **tres** cosas. Probar con `mosquitto_pub` desde la línea de comandos |
| Reinicios constantes | Router saturado o AP inestable | El reinicio genera MAC nueva; revisar el router |
| No cambia nada tras flashear | Se subió sin recompilar | `upload` no compila; usar `./scripts/compilar.sh` primero |

## 11. Calibración

El DS18B20 viene calibrado de fábrica y no requiere ajuste. La fuente de error es la
instalación:

- Instalar el sensor lejos de fuentes de calor directo (ESP8266, transformador, sol directo).
- Para medir temperatura de ambiente, aislar el sensor del calor que sube por los cables.
- En sondas impermeables, parte del termistor toca la vaina metálica: la lectura es de la
  vaina, no del aire. Es correcto y es lo que se espera.

## 12. Advertencias

- **El DS18B20 no mide humedad.** No confundir con sensores DHT o BME.
- El equipo reinicia solo ante fallos. Si se requiere comportamiento totalmente silencioso
  (sin reinicios), ajustar `PING_INTERVAL`, `PING_MAX_TRIES` y `STALE_REPORT_MS` en el `.ino`.
- Uso interior. Para exteriores, sensor con sonda y funda sellada.
