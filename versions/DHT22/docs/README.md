<p align="center">
  <img src="https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white" alt="Arduino"/>
  <img src="https://img.shields.io/badge/PlatformIO-FF7F00?style=for-the-badge&logo=PlatformIO&logoColor=white" alt="PlatformIO"/>
  <img src="https://img.shields.io/badge/License-MIT-green?style=for-the-badge" alt="MIT License"/>
  <img src="https://img.shields.io/badge/version-1.0.0-blue?style=for-the-badge" alt="Version 1.0.0"/>
  <img src="https://img.shields.io/badge/Status-Est%C3%A1ble-success?style=for-the-badge" alt="Status"/>
</p>

<h1 align="center">🌡️ DHT22 (AM2302) — Librería Arduino para Temperatura y Humedad</h1>

<p align="center">
  <b>Implementación raw del protocolo single-bus propietario</b><br>
  Sin dependencias externas • Compacta • Robusta • Auditable
</p>

---

## 📋 Descripción General

El **DHT22** (también conocido como **AM2302** o **RHT03**) es un sensor digital de temperatura y humedad relativa de bajo costo y alta precisión. Esta librería implementa el protocolo single-bus propietario del sensor a nivel de *bit-banging* sin utilizar ninguna librería externa (Adafruit, DHT-sensor-library, etc.).

| Característica | Valor |
|:---|---:|
| Rango de temperatura | -40°C a +80°C |
| Precisión de temperatura | ±0.5°C |
| Resolución de temperatura | 0.1°C |
| Rango de humedad | 0% a 100% RH |
| Precisión de humedad | ±2% RH (0–100%) |
| Resolución de humedad | 0.1% RH |
| Frecuencia máxima de muestreo | 0.5 Hz (1 lectura cada 2 segundos) |
| Voltaje de operación | 3.3V – 6V DC |

---

## 🔬 Principio Físico

### Sensor de Humedad (Capacitivo)
El DHT22 mide la humedad relativa mediante un **sensor capacitivo de película delgada de polímero**. El material dieléctrico absorbe o libera vapor de agua del ambiente, cambiando su permitividad relativa y, por lo tanto, la capacitancia del elemento sensor. Un oscilador interno convierte esta variación de capacitancia en una señal eléctrica que es digitalizada y transmitida.

### Sensor de Temperatura (Termistor NTC)
La temperatura se mide mediante un **termistor NTC** (*Negative Temperature Coefficient*). La resistencia del elemento disminuye al aumentar la temperatura. Un ADC interno de 14 bits (para temperatura) y 14 bits (para humedad) digitaliza ambas mediciones, que son transmitidas en una trama de 40 bits a través del bus single-wire.

### Protocolo de Comunicación
El DHT22 utiliza un **protocolo single-bus propietario** con temporización en microsegundos:

1. **START**: El microcontrolador pone la línea DATA en LOW durante ≥1 ms, luego la libera (INPUT + pull-up).
2. **RESPUESTA**: El sensor responde con LOW durante 80 µs, seguido de HIGH durante 80 µs.
3. **DATOS**: 40 bits (5 bytes) transmitidos MSB-first. Cada bit comienza con LOW durante 50 µs, seguido de HIGH:
   - **26–28 µs** → bit `0`
   - **~70 µs** → bit `1`
4. **CHECKSUM**: La suma de los 4 primeros bytes (módulo 256) debe coincidir con el quinto byte.

---

## 📊 Especificaciones Técnicas

### Eléctricas

| Parámetro | Mín. | Típ. | Máx. | Unidad |
|:---|---:|---:|---:|:---|
| Voltaje de alimentación (VDD) | 3.3 | 5.0 | 6.0 | V |
| Corriente de reposo (standby) | — | 50 | 200 | µA |
| Corriente en medición | — | 1.0 | 1.5 | mA |
| Corriente máxima (pico) | — | — | 5.0 | mA |
| Voltaje lógico LOW | -0.3 | — | 0.8 | V |
| Voltaje lógico HIGH | 2.4 | — | VDD+0.3 | V |

### Ambientales

| Parámetro | Mín. | Máx. | Unidad |
|:---|---:|---:|:---|
| Temperatura de operación | -40 | +80 | °C |
| Temperatura de almacenamiento | -50 | +125 | °C |
| Humedad de operación | 0 | 100 | % RH |
| Humedad de almacenamiento (sin condensación) | 0 | 100 | % RH |

### Temporización del Bus

| Parámetro | Mín. | Típ. | Máx. | Unidad |
|:---|---:|---:|---:|:---|
| Señal START (LOW por microcontrolador) | 1000 | — | — | µs |
| Tiempo de respuesta del sensor (LOW→HIGH) | 70 | 80 | 90 | µs |
| LOW por bit | 45 | 50 | 55 | µs |
| HIGH para bit `0` | 24 | 27 | 30 | µs |
| HIGH para bit `1` | 68 | 70 | 75 | µs |
| Intervalo mínimo entre lecturas | — | 2000 | — | ms |

### Precisión del DHT22 vs DHT11

| Parámetro | DHT11 | **DHT22** |
|:---|---:|---:|
| Rango de temperatura | 0°C a 50°C | **-40°C a 80°C** |
| Precisión de temperatura | ±2°C | **±0.5°C** |
| Rango de humedad | 20% a 90% | **0% a 100%** |
| Precisión de humedad | ±5% | **±2%** |
| Resolución | 1°C / 1% | **0.1°C / 0.1%** |
| Frecuencia máxima | 1 Hz | **0.5 Hz** |

---

## 🔌 Pinout y Diagrama de Conexión

### Pinout del DHT22 (4 pines)

```
        ┌────────────┐
        │    DHT22   │
   ┌────┤ (AM2302)   │
   │    │            │
  ─┴────┴────┬───────┘
  1   2   3   4

  1: VDD  (Alimentación 3.3–6V)
  2: DATA (Bus de datos bidireccional)
  3: NC   (No conectar)
  4: GND  (Tierra)
```

### Diagrama de Conexión con Arduino Uno

```
   DHT22                     Arduino Uno
   ─────                     ───────────
   VDD (1) ───────────────── 5V
   DATA(2) ─────┬──────────── D2
                │
                ├────[4.7kΩ]──── 5V  (Resistencia pull-up externa)
                │
                ├────[100nF]──── GND (Condensador de desacople opcional)*
   NC   (3) ─── (sin conectar)
   GND  (4) ───────────────── GND
```

> **\*** Se recomienda un capacitor cerámico de **100 nF** entre VDD y GND, colocado lo más cerca posible al sensor, para filtrar ruido de alta frecuencia en la alimentación. Para cables largos (>2 m), agregar un capacitor electrolítico de **10 µF** en paralelo.

### Conexión mínima recomendada

| Componente | Valor | Propósito |
|:---|---:|:---|
| Resistencia pull-up | 4.7kΩ – 10kΩ | Evita flote en la línea DATA |
| Capacitor de desacople | 100 nF | Filtro de ruido de alta frecuencia |
| Capacitor electrolítico | 10 µF | Estabilización para cables >2 m |

> ⚠️ **CRÍTICO**: La resistencia pull-up es **obligatoria** si el módulo no la incluye (módulos genéricos sin PCB). Sin ella, la línea DATA queda en flote y las lecturas serán erráticas o nulas. Los módulos con PCB (como el AZDelivery) ya incluyen la resistencia.

### Conexión para módulo con PCB (recomendado)

Si usas un **módulo DHT22 con PCB** (el más común), generalmente ya incluye la resistencia pull-up de 4.7kΩ y el capacitor de desacople. La conexión se simplifica a:

```
   Módulo DHT22              Arduino Uno
   ──────────              ───────────
   VCC (≡) ──────────────── 5V
   DATA (S) ─────────────── D2
   GND (─)  ─────────────── GND
```

> Verifica siempre el pinout específico de tu módulo. Algunos módulos etiquetan los pines como `VCC`, `DATA`, `GND`, y un cuarto pin `NC` o simplemente no lo traen.

---

## 📦 Instalación de la Librería

### Opción 1: Instalación manual (carpeta `libraries` de Arduino)

```bash
# Clonar o copiar la carpeta DHT22/ en:
# Windows:  C:\Users\<tu_usuario>\Documents\Arduino\libraries\
# macOS:    ~/Documents/Arduino/libraries/
# Linux:    ~/Arduino/libraries/

cp -r DHT22/ ~/Arduino/libraries/
```

Luego reinicia el IDE de Arduino. La librería aparecerá en **Sketch → Include Library → DHT22**.

### Opción 2: Instalación desde GitHub (Arduino IDE)

1. **Sketch → Include Library → Manage Libraries**
2. Buscar `DHT22`
3. Hacer clic en **Install**

### Opción 3: PlatformIO

```ini
; platformio.ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
lib_deps =
    https://github.com/ag-creadorcontenido/DHT22
```

### Verificación de instalación

```cpp
#include <DHT22.h>

void setup() {
    Serial.begin(9600);
    DHT22 dht(2);
    if (dht.begin()) {
        Serial.println(F("DHT22 instalado correctamente!"));
    }
}

void loop() {}
```

---

## 🚀 Guía Rápida (Hello World)

```cpp
#include <DHT22.h>

DHT22 dht(2);

void setup() {
    Serial.begin(9600);
    dht.begin();
}

void loop() {
    float temp  = dht.readTemperature();
    float hum   = dht.readHumidity();

    if (!isnan(temp) && !isnan(hum)) {
        Serial.print(F("Temp: "));
        Serial.print(temp, 1);
        Serial.print(F("°C  Hum: "));
        Serial.print(hum, 1);
        Serial.println(F("%"));
    } else {
        Serial.println(F("Error de lectura"));
    }

    delay(2000);
}
```

---

## 💻 Ejemplos

### 1. BasicRead — Lectura básica de temperatura y humedad

**Archivo:** `examples/BasicRead/BasicRead.ino`

Muestra cada 2 segundos la temperatura en °C y °F, y la humedad relativa en %. Es el punto de partida ideal para verificar que el sensor funciona correctamente.

**Salida esperada por Serial Monitor (9600 baud):**

```
=== DHT22 BasicRead ===
Inicializando sensor...
Sensor DHT22 inicializado correctamente.

Temperatura(C)  Temperatura(F)  Humedad(%)
----------------------------------------------
25.3 C          77.5 F          60.2 %
25.4 C          77.7 F          60.1 %
...
```

**Flujo del código:**
1. Incluye la librería `DHT22.h` y crea un objeto en el pin D2.
2. En `setup()`: inicializa el Serial y llama a `dht.begin()`.
3. En `loop()`: lee temperatura y humedad, verifica `isnan()`, muestra por Serial, espera 2 s.

---

### 2. HeatIndex — Lectura con Índice de Calor

**Archivo:** `examples/HeatIndex/HeatIndex.ino`

Además de temperatura y humedad, calcula y muestra el **Heat Index** (índice de calor o sensación térmica) en °C y °F, junto con un mensaje de advertencia categorizado:

| Heat Index (°C) | Advertencia |
|:---|---:|
| ≥ 54.0 | 🔴 ¡PELIGRO EXTREMO! Golpe de calor inminente |
| 41.0 – 53.9 | 🟠 ¡PELIGRO! Calambres/agotamiento por calor probable |
| 32.0 – 40.9 | 🟡 Precaución: posible agotamiento con actividad prolongada |
| 27.0 – 31.9 | 🟢 Precaución: posible fatiga con exposición prolongada |
| < 27.0 | Sin riesgo por calor |

**Salida esperada:**

```
=== DHT22 Heat Index ===
...
Temp(C)  Temp(F)  Hum(%)  HI(C)   HI(F)   Sensacion
----------------------------------------------------------
30.5 C   86.9 F   70.2 %  37.2 C  99.0 F   Precaución: posible agotamiento...
```

**Flujo del código:**
1. Lee temperatura y humedad con `readTemperature()` y `readHumidity()`.
2. Calcula HI con `computeHeatIndex(tempC, humidity)` y `computeHeatIndexF(tempF, humidity)`.
3. Clasifica el resultado en 5 niveles de riesgo e imprime una advertencia.

---

### 3. GreenhouseControl — Control Automatizado de Invernadero

**Archivo:** `examples/GreenhouseControl/GreenhouseControl.ino`

Sistema completo de control ambiental para invernadero con los siguientes elementos:

| Componente | Pin Arduino | Función |
|:---|---:|:---|
| DHT22 DATA | D2 | Sensor de temperatura y humedad |
| Extractor | D8 | Reduce humedad ambiental (activo HIGH) |
| Nebulizador | D9 | Enfría el ambiente (activo HIGH) |
| LED Verde | D10 | Indicador de estado normal |
| LED Rojo | D11 | Indicador de alarma/error |

**Lógica de control con histéresis:**

```
  ┌─────────────────────────────────────────────────────────┐
  │                    LÓGICA DE CONTROL                     │
  ├─────────────────────────────────────────────────────────┤
  │                                                         │
  │  Extractor:                                             │
  │    ┌─── ON ───────────────────────────── OFF ───┐      │
  │    │             ┌──────────┐                   │      │
  │    │   Humedad   │  ZONA DE │    Humedad        │      │
  │    │   > 70%     │ HISTÉRE- │    < 65%          │      │
  │    │             │   SIS    │                   │      │
  │    └────────────>│──────────│<──────────────────┘      │
  │                  └──────────┘                           │
  │                                                         │
  │  Nebulizador:                                           │
  │    ┌─── ON ───────────────────────────── OFF ───┐      │
  │    │             ┌──────────┐                   │      │
  │    │ Temperatura  │  ZONA DE │   Temperatura    │      │
  │    │   > 30°C    │ HISTÉRE- │    < 28°C        │      │
  │    │             │   SIS    │                   │      │
  │    └────────────>│──────────│<──────────────────┘      │
  │                  └──────────┘                           │
  └─────────────────────────────────────────────────────────┘
```

**Salida esperada por Serial:**

```
=== DHT22 Greenhouse Control ===
...
[ACCION] Extractor ENCENDIDO (humedad alta)
[ACCION] Nebulizador ENCENDIDO (temperatura alta)
[MONITOR] Temp: 31.2 C, Hum: 72.5 %, HI: 37.8 C | Extractor: ON | Nebulizador: ON | Errores: 0
```

**Indicación LED:**
- **LED Verde fijo**: Sistema funcionando correctamente, datos válidos.
- **LED Rojo parpadeante** (cada 300 ms): Error de lectura o comunicación con el sensor.

---

## 📚 Referencia de la API

### Clase `DHT22`

| Método | Descripción | Retorna |
|:---|---|:---:|
| `DHT22(uint8_t dataPin)` | Constructor. Asigna el pin digital para DATA. | — |
| `bool begin()` | Inicializa el pin y verifica comunicación con el sensor. | `true` si OK |
| `bool read()` | Fuerza una lectura completa (ignora el intervalo mínimo). | `true` si OK |
| `float readTemperature()` | Retorna temperatura en °C (respeta intervalo mínimo). | `float` / `NAN` |
| `float readTemperature(bool force)` | °C; si `force=true`, ignora el intervalo mínimo. | `float` / `NAN` |
| `float readTemperatureF()` | Retorna temperatura en °F. | `float` / `NAN` |
| `float readTemperatureF(bool force)` | °F con opción de forzar lectura. | `float` / `NAN` |
| `float readHumidity()` | Retorna humedad relativa en % (0–100). | `float` / `NAN` |
| `float readHumidity(bool force)` | Humedad con opción de forzar lectura. | `float` / `NAN` |
| `float computeHeatIndex(float tempC, float humidity)` | Calcula el Heat Index en °C (Rothfusz / Steadman). | `float` |
| `float computeHeatIndexF(float tempF, float humidity)` | Calcula el Heat Index en °F. | `float` |
| `uint32_t getLastReadTime()` | Timestamp (`millis()`) de la última lectura exitosa. | `uint32_t` |
| `bool isReadingValid()` | `true` si la última lectura fue válida. | `bool` |
| `uint8_t getErrorCount()` | Contador acumulado de errores de comunicación. | `uint8_t` |
| `void resetErrorCount()` | Reinicia el contador de errores a 0. | — |
| `void setMinInterval(uint16_t ms)` | Configura el intervalo mínimo entre lecturas (predeterminado: 2000 ms). | — |
| `uint16_t getMinInterval()` | Retorna el intervalo mínimo configurado. | `uint16_t` |

### Métodos Privados (no accesibles desde el sketch)

| Método | Descripción |
|:---|---|
| `bool _sendStartSignal()` | Envía la señal START y espera la respuesta del sensor. |
| `bool _readRawData()` | Lee 40 bits del sensor, midiendo la duración de cada pulso HIGH. |
| `bool _verifyChecksum()` | Verifica que la suma de los 4 primeros bytes coincida con el checksum. |
| `void _parseData()` | Convierte los 5 bytes raw en valores de temperatura y humedad. |
| `bool _validateRange()` | Comprueba que temperatura y humedad estén dentro de rangos físicos. |

---

## ⚠️ Advertencias y Buenas Prácticas

### 1. Frecuencia Mínima de 2 Segundos
El DHT22 **no debe** ser leído más de una vez cada 2 segundos. El sensor se autocalienta durante la medición (~1.5 mA). Si se lee con mayor frecuencia, la temperatura medida será artificialmente alta. La librería respeta automáticamente este intervalo: si llamas a `readTemperature()` antes de que pasen 2 s desde la última lectura, retorna el valor en caché.

### 2. Valores NaN (Not a Number)
Si el sensor no responde (desconectado, timeout, checksum inválido, rango fuera de especificación), los métodos de lectura retornan `NAN`. Siempre verifica con `isnan()` antes de usar los valores:

```cpp
float t = dht.readTemperature();
if (isnan(t)) {
    // manejar error
}
```

### 3. Cables Largos (>20 m)
Con cables de más de 20 metros, la capacitancia parasítica degrada la forma de onda del single-bus, causando errores de temporización y checksums inválidos. Para distancias largas:
- Usar cable apantallado (par trenzado) para DATA y GND.
- Colocar un capacitor de 10 µF cerca del sensor.
- Reducir la velocidad de comunicación o usar un repetidor/line driver como el MAX485.

### 4. Exposición a Químicos y Condensación
El DHT22 **no está sellado herméticamente**. La exposición prolongada a:
- Vapores químicos (alcoholes, solventes, amoníaco)
- Condensación directa de agua
- Polvo en suspensión

…puede degradar el elemento sensor capacitivo de humedad, causando lecturas inexactas y permanentes. En ambientes hostiles, protege el sensor con una cubierta de PTFE (Gore-Tex) o utiliza sensores industriales sellados (SHT85, BME280).

### 5. Pull-up Externo
**Siempre usa una resistencia pull-up de 4.7kΩ–10kΩ** entre VDD y DATA si tu módulo no la incluye. Sin ella, las lecturas serán intermitentes o nulas.

### 6. Temperatura y Autocalentamiento
El sensor incrementa su temperatura interna ~0.5°C durante la medición. La librería previene esto forzando el intervalo de 2 s. No montes el sensor cerca de fuentes de calor (disipadores, motores, reguladores de voltaje).

### 7. noInterrupts()
La comunicación single-bus requiere temporización precisa en microsegundos. Durante la lectura, las interrupciones globales se deshabilitan por un máximo de ~8 ms. Esto puede afectar:
- Recepción Serial (pérdida de caracteres)
- Temporización de `millis()` (mínimo desfase)
- Servomotores y generación de PWM por software

En aplicaciones críticas, programa las lecturas en momentos donde sea aceptable esta ventana sin interrupciones.

---

## 🌡️ Heat Index (Índice de Calor) Explicado

El **Heat Index** (HI), también conocido como *sensación térmica*, es la temperatura que el cuerpo humano percibe cuando se combinan el calor y la humedad. El sudor se evapora de la piel para enfriar el cuerpo; cuando la humedad es alta, la evaporación se ralentiza y la sensación de calor aumenta.

### Fórmulas Implementadas

#### Para temperaturas ≤ 27°C (fórmula simplificada de Steadman)

La fórmula de Steadman es una aproximación lineal válida para temperaturas moderadas, operada completamente en °F:

```
T_F   = tempC × 9/5 + 32
HI_F  = 0.5 × (T_F + 61 + (T_F − 68) × 1.2 + humedad × 0.094)
HI_C  = (HI_F − 32) × 5/9
```

Si el HI calculado es menor que la temperatura ambiente, se retorna la temperatura ambiente (el calor no puede sentirse más frío que el aire).

#### Para temperaturas > 27°C (regresión completa de Rothfusz)

La ecuación de Rothfusz es una regresión polinómica de segundo orden calibrada para altas temperaturas:

```
HI = −42.379
     + 2.04901523 × T
     + 10.14333127 × R
     − 0.22475541 × T × R
     − 6.83783×10⁻³ × T²
     − 5.481717×10⁻² × R²
     + 1.22874×10⁻³ × T² × R
     + 8.5282×10⁻⁴ × T × R²
     − 1.99×10⁻⁶ × T² × R²
```

Donde:
- **T** = temperatura en °F
- **R** = humedad relativa en %

### Categorías de Riesgo (NWS - National Weather Service)

| HI (°C) | HI (°F) | Categoría | Síntomas |
|:---:|:---:|:---|---|
| ≥ 54 | ≥ 130 | 🔴 **PELIGRO EXTREMO** | Golpe de calor inminente |
| 41–53 | 105–129 | 🟠 **PELIGRO** | Calambres, agotamiento probable |
| 32–40 | 90–104 | 🟡 **PRECAUCIÓN** | Posible agotamiento con actividad prolongada |
| 27–31 | 80–89 | 🟢 **PRECAUCIÓN MÍNIMA** | Posible fatiga con exposición prolongada |
| < 27 | < 80 | ✅ **SIN RIESGO** | Condiciones confortables |

---

## 📐 Formato de Trama de 40 Bits

```
Byte 0 (MSB)         Byte 1                Byte 2                Byte 3                Byte 4
┌──────────────────┬────────────────────┬────────────────────┬────────────────────┬────────────────────┐
│   Humedad (MSB)  │   Humedad (LSB)    │  Temperatura (MSB) │  Temperatura (LSB) │     Checksum       │
│      bits 15:8   │      bits 7:0      │      bits 15:8     │      bits 7:0      │      bits 7:0      │
└──────────────────┴────────────────────┴────────────────────┴────────────────────┴────────────────────┘
        MSB ────────────────────────────────────────────────────────────────────────── LSB
```

**Interpretación:**
- **Humedad**: `(byte[0] << 8) | byte[1]` → dividir entre 10 → % RH
- **Temperatura**: `(byte[2] << 8) | byte[3]` → dividir entre 10 → °C
  - Si el bit 15 (`byte[2] & 0x80`) está activo, la temperatura es negativa
- **Checksum**: `(byte[0] + byte[1] + byte[2] + byte[3]) & 0xFF == byte[4]`

**Ejemplo:**
```
Datos raw:  0x03 0xE8 0x01 0x06 0xF2
Humedad:    0x03E8 = 1000 → 100.0% RH
Temperatura: 0x0106 = 262  → 26.2°C (bit 15 = 0, positiva)
Checksum:   0x03 + 0xE8 + 0x01 + 0x06 = 0xF2 ✅
```

---

## 📄 Licencia

```
MIT License

Copyright (c) 2026 AG Creador de Contenido

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

<p align="center">
  <sub>Documentación generada para la librería DHT22 v1.0.0</sub><br>
  <sub>Arduino • PlatformIO • MIT License • 2026</sub>
</p>
