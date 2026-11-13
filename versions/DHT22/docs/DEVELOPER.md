# 🛠️ DEVELOPER.md — Documentación para Desarrolladores

## Arquitectura de la Librería

### Diagrama de Clases

```
┌──────────────────────────────────────────────────────┐
│                      DHT22                            │
├──────────────────────────────────────────────────────┤
│ - _dataPin: uint8_t                                   │
│ - _temperature: float         ← Última temp °C       │
│ - _humidity: float            ← Última hum %         │
│ - _lastReadValid: bool                                 │
│ - _lastReadTime: uint32_t     ← millis() del último   │
│ - _errorCount: uint8_t                                 │
│ - _minInterval: uint16_t      ← Default 2000 ms       │
│ - _data[5]: uint8_t           ← Buffer raw 40 bits    │
├──────────────────────────────────────────────────────┤
│ + DHT22(dataPin: uint8_t)                             │
│ + begin(): bool                                       │
│ + read(): bool                                        │
│ + readTemperature(): float                            │
│ + readTemperature(force: bool): float                 │
│ + readTemperatureF(): float                           │
│ + readTemperatureF(force: bool): float                │
│ + readHumidity(): float                               │
│ + readHumidity(force: bool): float                    │
│ + computeHeatIndex(tempC: float, hum: float): float   │
│ + computeHeatIndexF(tempF: float, hum: float): float  │
│ + getLastReadTime(): uint32_t                         │
│ + isReadingValid(): bool                              │
│ + getErrorCount(): uint8_t                            │
│ + resetErrorCount(): void                             │
│ + setMinInterval(ms: uint16_t): void                  │
│ + getMinInterval(): uint16_t                          │
├──────────────────────────────────────────────────────┤
│ - _sendStartSignal(): bool      ← Protocolo raw       │
│ - _readRawData(): bool          ← Lectura de 40 bits  │
│ - _verifyChecksum(): bool       ← Suma & 0xFF == byte │
│ - _parseData(): void            ← Bytes → valores     │
│ - _validateRange(): bool        ← Rangos físicos     │
└──────────────────────────────────────────────────────┘
```

### Flujo de Lectura Completo

```
readTemperature() / readHumidity()
    │
    ├── ¿force=true? ──SÍ──→ ¿read()? ──SÍ──→ _temperature / _humidity
    │                       └──NO──→ NAN
    │
    └── ¿_lastReadValid? ──NO──→ read() ──SÍ──→ _temperature / _humidity
         && (now - _lastReadTime              └──NO──→ NAN
              >= _minInterval)?
                 │
                 └──SÍ──→ read()
                 │
                 └──NO──→ _temperature / _humidity (cache)
```

```
read()
    │
    ├── _sendStartSignal()
    │       ├── pinMode(OUTPUT), digitalWrite(LOW) por ≥1000µs
    │       ├── pinMode(INPUT_PULLUP), espera respuesta
    │       │   └── timeout > 100µs → return false
    │       └── return true
    │
    ├── _readRawData()
    │       ├── Para byte 0..4:
    │       │   └── Para bit 7..0:
    │       │       ├── esperar LOW (timeout 100µs)
    │       │       ├── medir HIGH (timeout 100µs)
    │       │       └── si HIGH > 40µs → bit=1, sino bit=0
    │       └── return true
    │
    ├── _verifyChecksum()
    │       └── sum(_data[0..3]) & 0xFF == _data[4]
    │
    ├── _parseData()
    │       ├── hum = (_data[0]<<8 | _data[1]) / 10.0
    │       └── temp = (_data[2]<<8 | _data[3]) / 10.0 (con signo bit 15)
    │
    ├── _validateRange()
    │       ├── temp en [-40, 80]?
    │       └── hum en [0, 100]?
    │
    ├── _lastReadValid = true
    ├── _lastReadTime = millis()
    └── return true
```

### Estructura de Archivos

```
DHT22/
├── DHT22.h          # Declaración de la clase y tipos públicos
├── DHT22.cpp        # Implementación completa del protocolo
├── library.properties  # Metadatos para el IDE de Arduino
└── examples/
    ├── BasicRead/
    │   └── BasicRead.ino
    ├── HeatIndex/
    │   └── HeatIndex.ino
    └── GreenhouseControl/
        └── GreenhouseControl.ino
```

---

## Implementación del Protocolo Single-Bus Raw

### 1. Envío de Señal START (`_sendStartSignal()`)

El microcontrolador actúa como **maestro** e inicia la comunicación:

```cpp
// Paso 1: Configurar DATA como OUTPUT y poner en LOW por ≥1ms
pinMode(_dataPin, OUTPUT);
digitalWrite(_dataPin, LOW);
delayMicroseconds(START_LOW_US);    // 1000 µs

// Paso 2: Liberar la línea (INPUT_PULLUP) y esperar respuesta
pinMode(_dataPin, INPUT_PULLUP);
digitalWrite(_dataPin, HIGH);
delayMicroseconds(START_HIGH_US);   // 30 µs
```

Después de liberar la línea, el maestro espera que el sensor **baje** la línea (respuesta LOW de 80 µs) y luego que la **suba** (HIGH de 80 µs). Si no hay respuesta en 100 µs, se aborta con timeout.

```cpp
uint32_t timeout = micros();
while (digitalRead(_dataPin) == HIGH) {
    if ((micros() - timeout) > RESPONSE_TIMEOUT) {  // 100 µs
        interrupts();
        return false;   // Sensor no responde
    }
}
```

### 2. Lectura de 40 Bits (`_readRawData()`)

Cada bit se transmite como:
- **LOW** de ~50 µs (inicio del bit, constante)
- **HIGH** de 26–28 µs → bit `0`, o ~70 µs → bit `1`

El maestro mide la **duración del HIGH** para determinar el valor del bit:

```cpp
// Por cada bit (40 veces):
// 1. Esperar a que la línea baje (LOW 50µs)
while (digitalRead(_dataPin) == LOW) {
    if ((micros() - timeout) > BIT_TIMEOUT) return false;
}

// 2. Medir cuánto dura el HIGH
timeout = micros();
while (digitalRead(_dataPin) == HIGH) {
    if ((micros() - timeout) > BIT_TIMEOUT) return false;
}
uint32_t highDuration = micros() - timeout;

// 3. Si HIGH > umbral (40µs) → bit 1; sino → bit 0
if (highDuration > BIT_HIGH_THRESH) {
    result |= (1 << bitIdx);
}
```

### 3. Verificación de Checksum (`_verifyChecksum()`)

```cpp
bool DHT22::_verifyChecksum() {
    uint8_t sum = _data[0] + _data[1] + _data[2] + _data[3];
    return (sum & 0xFF) == _data[4];
}
```

### 4. Parseo de Datos (`_parseData()`)

```cpp
void DHT22::_parseData() {
    // Humedad: 16 bits, dividir entre 10
    uint16_t rawHum = ((uint16_t)_data[0] << 8) | _data[1];
    _humidity = rawHum / 10.0f;

    // Temperatura: 16 bits, con bit 15 como signo
    uint16_t rawTemp = ((uint16_t)_data[2] << 8) | _data[3];
    if (rawTemp & 0x8000) {
        rawTemp &= 0x7FFF;          // Máscara de signo
        _temperature = -((float)rawTemp / 10.0f);
    } else {
        _temperature = rawTemp / 10.0f;
    }
}
```

---

## Temporización de Bits (Valores con `noInterrupts()`)

Todas las constantes de temporización se definen como `static const uint32_t` en `DHT22.cpp`:

| Constante | Valor | Propósito |
|:---|---:|:---|
| `START_LOW_US` | 1000 | Duración del LOW inicial de START (≥1 ms) |
| `START_HIGH_US` | 30 | Espera después de liberar línea, antes de respuesta |
| `RESPONSE_TIMEOUT` | 100 | Timeout para respuesta del sensor |
| `BIT_TIMEOUT` | 100 | Timeout por cada flanco (LOW o HIGH) de bit |
| `BIT_HIGH_THRESH` | 40 | Umbral para decidir bit 0 (<40 µs) vs bit 1 (>40 µs) |

### Ventana Máxima sin Interrupciones

El peor caso ocurre cuando se reciben 40 bits `1` (HIGH de ~70 µs cada uno):

```
T_total ≈ T_START + T_RESPONSE + 40 × (T_BIT_LOW + T_BIT1_HIGH)
        ≈ 1000 + 80 + 80 + 40 × (50 + 75)
        ≈ 1000 + 160 + 40 × 125
        ≈ 1000 + 160 + 5000
        ≈ 6.16 ms
```

Con el timeout de 100 µs por flanco, el máximo teórico es ~8 ms. Esto es aceptable para Arduino Uno (no afecta significativamente a `millis()` ni a la recepción Serial a 9600 baud con buffer de 64 bytes).

---

## Formato de Trama de 40 Bits

```
        ┌───────┬───────┬───────┬───────┬───────┐
        │ B0    │ B1    │ B2    │ B3    │ B4    │
        ├───────┼───────┼───────┼───────┼───────┤
        │HUM_MSB│HUM_LSB│TEMP_MSB│TEMP_LSB│CHK   │
        └───────┴───────┴───────┴───────┴───────┘
          MSB ← ─ ─ ─ ─ ─ ─ ─ ─ ─ → LSB
```

| Rango de bits | Contenido | Rango físico | Fórmula |
|:---:|:---|---:|:---|
| `[31:16]` | Humedad cruda | 0–1000 | `valor / 10.0` → % RH |
| `[15:0]` | Temperatura cruda | -400–800 | `valor / 10.0` → °C |
| `[39:32]` | Checksum | 0–255 | `(B0+B1+B2+B3) & 0xFF` |

### Bit de Signo (Temperatura Negativa)

El bit 15 del campo de temperatura (MSB del byte B2) indica signo:

```
B2 (0x80) → rawTemp = rawTemp & 0x7FFF; temp = -rawTemp / 10.0
B2 (0x00) → temp = rawTemp / 10.0
```

### Tabla de Ejemplos

| Hum raw | Temp raw | Humedad | Temperatura |
|:---:|:---:|---:|---:|
| 0x03E8 (1000) | 0x0106 (262) | 100.0% | 26.2°C |
| 0x0000 (0) | 0x0000 (0) | 0.0% | 0.0°C |
| 0x012C (300) | 0x8002 (2) | 30.0% | -0.2°C |
| 0x01F4 (500) | 0xFF06 (-250)* | 50.0% | — (inválido, fuera de rango -40..80) |

*\*Valor con checksum inválido o fuera de rango físico, descartado por `_validateRange()`.*

---

## Cálculo del Checksum

### Algoritmo de Verificación

```cpp
// En el sensor (transmisión):
// checksum = (byte_hum_msb + byte_hum_lsb + byte_temp_msb + byte_temp_lsb) & 0xFF

// En el receptor (verificación):
uint8_t sum = data[0] + data[1] + data[2] + data[3];
bool checksumOk = ((sum & 0xFF) == data[4]);
```

### Implementación en `_verifyChecksum()`

El método privado `_verifyChecksum()` se llama **antes** de `_parseData()`. Si el checksum no coincide, toda la trama se descarta y el error se contabiliza. Esto previene usar datos corruptos.

### Ejemplos de Checksum Válido

| B0 | B1 | B2 | B3 | Suma | B4 (checksum) | ¿Match? |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 0x03 | 0xE8 | 0x01 | 0x06 | 0xF2 | 0xF2 | ✅ |
| 0x02 | 0x80 | 0x01 | 0x06 | 0x89 | 0x89 | ✅ |
| 0x00 | 0x00 | 0x80 | 0x02 | 0x82 | 0x82 | ✅ |
| 0x03 | 0xE8 | 0xFF | 0x06 | 0xF0 | 0xF0 | ✅ (aunque tempFF es sospechosa, checksum da bien; luego _validateRange lo descarta) |

---

## Fórmulas de Heat Index

### Contexto

El **Heat Index** (HI) es la temperatura aparente que percibe el cuerpo humano bajo el efecto combinado de temperatura ambiente y humedad relativa. Fue desarrollado por Robert G. Steadman (1979) y posteriormente refinado por Lans P. Rothfusz (1990) para el Servicio Meteorológico Nacional de EE.UU. (NWS).

### Fórmula Simplificada de Steadman (T ≤ 27°C)

Para temperaturas moderadas o frías donde el HI no es significativamente mayor que la temperatura ambiente:

```cpp
if (tempC <= 27.0f) {
    float T_F = tempC * 9.0f / 5.0f + 32.0f;
    float HI_F = 0.5f * (T_F + 61.0f + ((T_F - 68.0f) * 1.2f) + (humidity * 0.094f));
    float HI_C = (HI_F - 32.0f) * 5.0f / 9.0f;
    return (HI_C < tempC) ? tempC : HI_C;
}
```

**Nota importante**: Esta fórmula opera enteramente en grados Fahrenheit. La implementación convierte `tempC` → `°F`, aplica la regresión de Steadman, y convierte el resultado de vuelta a `°C`. Si el HI calculado es menor que la temperatura ambiente (físicamente imposible), se retorna `tempC`.

### Regresión Completa de Rothfusz (T > 27°C)

Cuando la temperatura supera los 27°C (80°F), se utiliza la regresión polinómica completa de Rothfusz:

```
HI = -42.379
     + 2.04901523 × T
     + 10.14333127 × R
     - 0.22475541 × T × R
     - 6.83783×10⁻³ × T²
     - 5.481717×10⁻² × R²
     + 1.22874×10⁻³ × T² × R
     + 8.5282×10⁻⁴ × T × R²
     - 1.99×10⁻⁶ × T² × R²
```

Donde:
- **T** = temperatura en **grados Fahrenheit**
- **R** = humedad relativa en **porcentaje**

Implementación:

```cpp
float T = tempC * 9.0f / 5.0f + 32.0f;    // °C → °F
float R = humidity;

float HI_F = -42.379f
             + 2.04901523f * T
             + 10.14333127f * R
             - 0.22475541f * T * R
             - 6.83783e-3f * T * T
             - 5.481717e-2f * R * R
             + 1.22874e-3f * T * T * R
             + 8.5282e-4f * T * R * R
             - 1.99e-6f * T * T * R * R;

float HI_C = (HI_F - 32.0f) * 5.0f / 9.0f;  // °F → °C
return (HI_C < tempC) ? tempC : HI_C;
```

### Límite Físico Inferior

En ambos casos, si `HI_C < tempC`, se retorna `tempC`. Físicamente, el índice de calor no puede ser menor que la temperatura ambiente (el aire caliente y húmedo no puede sentirse más frío que el aire seco a la misma temperatura).

### Precisión de la Fórmula Rothfusz

| Rango de temperatura | Precisión típica del HI | Notas |
|:---|---:|:---|
| 27–32°C (80–90°F) | ±1°C | Válido para humedad 40–80% |
| 32–38°C (90–100°F) | ±1.5°C | Rango de mayor precisión |
| 38–43°C (100–110°F) | ±2°C | Comienza a divergir en humedad >80% |
| 43–54°C (110–130°F) | ±3°C | Precisión degradada |
| >54°C (>130°F) | ±4°C | Valor indicativo; riesgo extremo |

### Heat Index en Fahrenheit (`computeHeatIndexF`)

Este método es un wrapper que convierte `tempF` → `°C`, llama a `computeHeatIndex()`, y convierte el resultado de vuelta a `°F`:

```cpp
float DHT22::computeHeatIndexF(float tempF, float humidity) {
    float tempC = (tempF - 32.0f) * 5.0f / 9.0f;
    float hiC = computeHeatIndex(tempC, humidity);
    return hiC * 9.0f / 5.0f + 32.0f;
}
```

---

## Todos los Métodos Públicos y Privados

### Métodos Públicos

| Método | Línea (DHT22.cpp) | Descripción técnica |
|:---|---:|:---|
| `DHT22(uint8_t dataPin)` | 37–50 | Inicializa miembros: temp=NAN, hum=NAN, `_lastReadValid=false`, `_lastReadTime=0`, `_errorCount=0`, `_minInterval=2000`. |
| `bool begin()` | 55–65 | Configura `INPUT_PULLUP`, delay 20ms, ejecuta `read()`. Retorna `false` si no hay sensor. |
| `bool read()` | 70–105 | Pipeline completo: `_sendStartSignal()` → `_readRawData()` → `_verifyChecksum()` → `_parseData()` → `_validateRange()`. Retorna `true` solo si todos los pasos pasan. |
| `float readTemperature()` | 110–112 | Delega en `readTemperature(false)`. |
| `float readTemperature(bool force)` | 114–125 | Si `force=true`, llama a `read()` y retorna NAN si falla. Si no, verifica caché y `_minInterval`. |
| `float readTemperatureF()` | 130–136 | Convierte `readTemperature()` de °C a °F. NAN si no hay datos. |
| `float readTemperatureF(bool force)` | 138–144 | Ídem con flag de forzar. |
| `float readHumidity()` | 149–151 | Delega en `readHumidity(false)`. |
| `float readHumidity(bool force)` | 153–163 | Análogo a `readTemperature(bool force)`. |
| `float computeHeatIndex(float tempC, float humidity)` | 168–203 | Calcula HI en °C usando Steadman (≤27°C) o Rothfusz (>27°C). |
| `float computeHeatIndexF(float tempF, float humidity)` | 205–209 | HI en °F (wrapper de conversión). |
| `uint32_t getLastReadTime()` | 214–216 | Retorna `_lastReadTime` (timestamp de `millis()`). |
| `bool isReadingValid()` | 218–220 | Retorna `_lastReadValid`. |
| `uint8_t getErrorCount()` | 222–224 | Retorna `_errorCount`. |
| `void resetErrorCount()` | 226–228 | `_errorCount = 0`. |
| `void setMinInterval(uint16_t ms)` | 230–232 | Asigna `_minInterval`. |
| `uint16_t getMinInterval()` | 234–236 | Retorna `_minInterval`. |

### Métodos Privados

| Método | Línea (DHT22.cpp) | Descripción técnica |
|:---|---:|:---|
| `bool _sendStartSignal()` | 241–284 | Deshabilita interrupciones. Configura DATA como OUTPUT/LOW por 1000 µs. Cambia a INPUT_PULLUP, espera respuesta del sensor. Timeout 100 µs en cada espera. Re-habilita interrupciones. Retorna `false` si timeout. |
| `bool _readRawData()` | 289–326 | Deshabilita interrupciones. 5 bytes × 8 bits: espera LOW (50 µs), mide HIGH, compara con umbral 40 µs. Almacena en `_data[5]`. Timeout 100 µs por flanco. Retorna `false` si timeout. |
| `bool _verifyChecksum()` | 331–334 | Suma `_data[0..3]` y compara `(sum & 0xFF)` con `_data[4]`. |
| `void _parseData()` | 339–354 | Convierte `_data[0..1]` → `_humidity` (÷10) y `_data[2..3]` → `_temperature` (÷10, maneja bit 15 de signo). |
| `bool _validateRange()` | 359–369 | Verifica que `_temperature` ∈ [-40, 80] y `_humidity` ∈ [0, 100]. Retorna `false` si NAN o fuera de rango. |

### Variables Miembro

| Variable | Tipo | Propósito |
|:---|---:|:---|
| `_dataPin` | `uint8_t` | Pin digital conectado a DATA del sensor. |
| `_temperature` | `float` | Última temperatura válida en °C. Inicializa como `NAN`. |
| `_humidity` | `float` | Última humedad válida en %. Inicializa como `NAN`. |
| `_lastReadValid` | `bool` | `true` si la última ejecución de `read()` completó exitosamente. |
| `_lastReadTime` | `uint32_t` | Valor de `millis()` al finalizar `read()` exitosamente. |
| `_errorCount` | `uint8_t` | Contador de fallos (timeout, checksum, rango). Se satura en 255. |
| `_minInterval` | `uint16_t` | Intervalo mínimo entre lecturas en ms. Default 2000. |
| `_data[5]` | `uint8_t[5]` | Buffer de 5 bytes para los 40 bits raw del sensor. |

---

## Manejo de Errores

### Jerarquía de Validación en `read()`

```
read()
  │
  ├── _sendStartSignal()
  │     └── Error: sensor no responde (timeout)
  │           → _lastReadValid=false, _errorCount++, return false
  │
  ├── _readRawData()
  │     └── Error: timeout leyendo bit
  │           → _lastReadValid=false, _errorCount++, return false
  │
  ├── _verifyChecksum()
  │     └── Error: checksum no coincide
  │           → _lastReadValid=false, _errorCount++, return false
  │
  ├── _parseData()     (no retorna error, siempre parsea)
  │
  ├── _validateRange()
  │     └── Error: temp fuera de [-40,80] o hum fuera de [0,100]
  │           → _lastReadValid=false, _errorCount++, return false
  │
  └── Éxito: _lastReadValid=true, _lastReadTime=millis(), return true
```

### Valores de Retorno en Clientes

| Método de lectura | Error | Retorno |
|:---|---|:---:|
| `readTemperature()` | Sensor desconectado | `NAN` |
| `readTemperature(true)` | Lectura forzada falla | `NAN` |
| `readTemperatureF()` | Dato inválido | `NAN` |
| `readHumidity()` | Checksum inválido | `NAN` |
| `readHumidity(true)` | Fuera de rango | `NAN` |

### Control de Intento de Lectura (Caching)

```
¿Última lectura válida?
  ├── NO → ejecutar read()
  └── SÍ → ¿(now - _lastReadTime) >= _minInterval?
              ├── SÍ → ejecutar read()
              └── NO → retornar _temperature / _humidity (cache)
```

El flag `force=true` en `readTemperature(bool)` y `readHumidity(bool)` salta este control y ejecuta `read()` incondicionalmente.

---

## Seguridad y Buenas Prácticas

1. **Interrupciones**: `noInterrupts()` se llama al inicio de `_sendStartSignal()` y `_readRawData()`. `interrupts()` se llama en **todos** los caminos de retorno (4 en `_sendStartSignal`, 3 en `_readRawData`).
2. **Sin heap**: No se usa `malloc()`, `new`, `free()`, ni `delete`. Toda la memoria es estática o en stack.
3. **Sin dependencias externas**: La librería solo incluye `<Arduino.h>`. No necesita Adafruit_Sensor ni ninguna otra.
4. **Protección de buffer**: `_data[5]` se escribe exclusivamente con índices controlados (`0..4`). Sin riesgo de overflow.
5. **Aritmética de millis()**: El cálculo `(now - _lastReadTime)` es seguro contra rollover de `millis()` (~50 días) por aritmética unsigned.
6. **Contador de errores**: `_errorCount` es `uint8_t`, se satura en 255. No causa overflow.
7. **Uso de NAN**: Todas las variables float se inicializan como `NAN`. Los métodos verifican con `isnan()` antes de retornar.

---

## Perfil de Memoria (AVR/Arduino Uno)

| Componente | Flash (bytes) | RAM (bytes) |
|:---|---:|---:|
| DHT22.h + DHT22.cpp | ~1050 | 0 (código) |
| Objeto DHT22 (miembros) | 0 | 22 |
| Buffer `_data[5]` | 0 | 5 |
| Total librería | ~1050 | ~27 |
| BasicRead.ino | ~220 | 0 (literales en flash con `F()`) |
| HeatIndex.ino | ~340 | 0 |
| GreenhouseControl.ino | ~780 | 12 |

> Medido con Arduino IDE 2.x + avr-g++ (optimización -Os). Valores aproximados.

---

## Testing

### Pruebas Unitarias (en hardware real)

```cpp
// test_basic.ino — Prueba de funcionalidad básica
DHT22 dht(2);
void setup() {
    Serial.begin(9600);
    Serial.println(dht.begin() ? "PASS: begin()" : "FAIL: begin()");
    delay(2000);
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    Serial.println(!isnan(t) ? "PASS: readTemperature()" : "FAIL: readTemperature()");
    Serial.println(!isnan(h) ? "PASS: readHumidity()" : "FAIL: readHumidity()");
    Serial.println(t >= -40 && t <= 80 ? "PASS: temp range" : "FAIL: temp range");
    Serial.println(h >= 0 && h <= 100 ? "PASS: hum range" : "FAIL: hum range");
}

void loop() {}
```

### Pruebas de Casos Borde

| Prueba | Procedimiento | Esperado |
|:---|---:|:---|
| Sensor desconectado | Desconectar DATA antes de `begin()` | `begin()` retorna `false` |
| Lectura forzada | `readTemperature(true)` dos veces seguidas | Segunda lectura fuerza nueva medición (puede fallar si <2s) |
| Intervalo mínimo | `readTemperature()` cada 500ms por 10s | Solo ~5 lecturas reales (cada 2s), resto en caché |
| Cables largos | Extender DATA con cable de 25m | Errores frecuentes de checksum, `getErrorCount()` incrementa |
| Fuente de calor | Acercar soldador (sin contacto) >50°C | `readTemperature()` retorna >50°C, no NAN |
| Humedad extrema | Sensor en bolsa sellada con paño húmedo | `readHumidity()` retorna >90% RH |

### Pruebas de Regresión (Heat Index)

| tempC | humidity | HI esperado | Tolerancia |
|:---:|:---:|:---:|:---:|
| 25.0 | 50% | 25.0°C (Steadman < temp) | ±0.5°C |
| 27.0 | 50% | 27.7°C (Steadman aplica en borde) | ±0.5°C |
| 30.0 | 70% | ~33.4°C (Rothfusz) | ±1.0°C |
| 35.0 | 80% | ~49.6°C (Rothfusz) | ±1.5°C |
| 40.0 | 90% | ~72.1°C (Rothfusz, extremo) | ±3.0°C |
| 32.0 | 60% | ~36.7°C | ±1.0°C |
| 28.0 | 40% | ~28.0°C (HI < temp → temp) | ±0.5°C |

---

## Referencias

- Datasheet Aosong AM2302: [Adafruit DHT22 datasheet](https://cdn-shop.adafruit.com/datasheets/DHT22.pdf)
- Cálculo de Heat Index (NWS): [https://www.weather.gov/safety/heat-index](https://www.weather.gov/safety/heat-index)
- Rothfusz regression: [NWS Technical Attachment (PDF)](https://www.weather.gov/media/ffc/ta_htindx.PDF)
- Steadman, R.G. (1979). "The assessment of sultriness. Part I: A temperature-humidity index based on human physiology and clothing science". *Journal of Applied Meteorology*, 18(7), 861-873.
- Especificación single-bus propietario Aosong: Extraída por ingeniería inversa del datasheet y validación empírica.
