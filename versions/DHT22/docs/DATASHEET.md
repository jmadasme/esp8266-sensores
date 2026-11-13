# 📋 DHT22 / AM2302 — Ficha Técnica del Hardware

## Identificación

| Parámetro | Valor |
|:---|---|
| **Nombre** | DHT22 (también AM2302, RHT03) |
| **Tipo** | Sensor digital de temperatura y humedad relativa |
| **Interfaz** | Single-bus propietario (1-wire, bidireccional) |
| **Fabricante** | Aosong Electronics Co., Ltd. (Guangzhou) |
| **Datasheet oficial** | [AM2302 Datasheet (PDF)](https://cdn-shop.adafruit.com/datasheets/DHT22.pdf) |

---

## Características Eléctricas

### Condiciones Absolutas Máximas

| Parámetro | Símbolo | Mín. | Máx. | Unidad |
|:---|---|:---:|:---:|:---:|
| Voltaje de alimentación | VDD | -0.3 | 6.0 | V |
| Voltaje en pin DATA | V_DATA | -0.3 | VDD+0.3 | V |
| Corriente máxima por pin | I_MAX | — | 10 | mA |
| Temperatura de almacenamiento | T_STG | -50 | +125 | °C |

### Condiciones de Operación Recomendadas

| Parámetro | Símbolo | Mín. | Típ. | Máx. | Unidad |
|:---|---|:---:|:---:|:---:|:---:|
| Voltaje de alimentación | VDD | 3.3 | 5.0 | 5.5 | V |
| Corriente de reposo | I_STBY | — | 50 | 200 | µA |
| Corriente promedio en medición | I_ACT | — | 1.0 | 1.5 | mA |
| Corriente pico (START + respuesta) | I_PEAK | — | — | 5.0 | mA |
| Rango de temperatura de operación | T_OP | -40 | — | +80 | °C |
| Rango de humedad de operación | RH_OP | 0 | — | 100 | % RH |

### Características Lógicas del Bus

| Parámetro | Símbolo | Mín. | Típ. | Máx. | Unidad |
|:---|---|:---:|:---:|:---:|:---:|
| Voltaje de salida LOW (I_OL = 4 mA) | V_OL | -0.3 | — | 0.8 | V |
| Voltaje de salida HIGH (I_OH = -4 mA) | V_OH | 2.4 | — | VDD+0.3 | V |
| Voltaje de entrada LOW | V_IL | -0.3 | — | 0.9 | V |
| Voltaje de entrada HIGH | V_IH | 2.7 | — | VDD+0.3 | V |
| Corriente de fuga en HIGH-impedancia | I_LEAK | — | — | ±10 | µA |

---

## Características de Medición

### Temperatura

| Parámetro | Valor | Unidad |
|:---|---:|:---:|
| Rango de medición | -40 a +80 | °C |
| Resolución | 0.1 | °C |
| Precisión (±) | 0.5 | °C |
| Tiempo de estabilización (tras encendido) | 2 | s |
| Tiempo de respuesta (63% del escalón, en aire en movimiento) | 5–10 | s |
| Deriva a largo plazo | < 0.1 | °C/año |
| No linealidad típica | < 0.3 | °C |
| Repetibilidad | < 0.1 | °C |
| Histeresis | < 0.3 | °C |

### Humedad Relativa

| Parámetro | Valor | Unidad |
|:---|---:|:---:|
| Rango de medición | 0 a 100 | % RH |
| Resolución | 0.1 | % RH |
| Precisión (±) — 0 a 100% RH | 2 | % RH |
| Precisión (±) — 25°C, 10 a 90% RH | 1 | % RH |
| Histéresis | < 0.3 | % RH |
| Tiempo de respuesta (63%, aire quieto) | 5 | s |
| Tiempo de respuesta (63%, 2 m/s flujo) | 2 | s |
| Deriva a largo plazo | < 0.5 | % RH/año |
| No linealidad | < 1 | % RH |

### Condiciones de Calibración de Fábrica

| Parámetro | Condición |
|:---|---|
| Temperatura de calibración | 25°C ± 1°C |
| Humedad de calibración | 10%, 35%, 55%, 75%, 95% RH |
| Voltaje de calibración | 5.0 V |
| Estándar de referencia | Higrómetro de punto de rocío (±0.5% RH) |
| Coeficiente de temperatura del sensor de humedad | ±0.1% RH/°C (típico) |

---

## Temporización del Protocolo Single-Bus

### Diagrama General

```
MAESTRO (Arduino):
    ┌────────────────────┐
    │ START: LOW ≥ 1 ms  │
    └────────┬───────────┘
             │
             ▼  Línea liberada (HIGH, INPUT_PULLUP)
    ┌────────────────────┐
    │ Espera respuesta   │
    │ (max 100 µs)       │
    └────────┬───────────┘
             │
             ▼
DHT22 responde:
    ┌────────────────────┐
    │ LOW   80 µs        │
    │ HIGH  80 µs        │
    └────────┬───────────┘
             │
             ▼
40 bits de datos:
    ┌──────────────────────────────────────┐
    │ bit[39] ... bit[0] (MSB first)       │
    │ Cada bit: LOW 50 µs + HIGH (26-70 µs)│
    └──────────────────────────────────────┘
```

### Especificación de Temporización

| Sección | Parámetro | Símbolo | Mín. | Típ. | Máx. | Unidad |
|:---|---|:---:|:---:|:---:|:---:|:---:|
| **START** | LOW por maestro | T_START_L | 1000 | — | — | µs |
| | HIGH por maestro (espera) | T_START_H | 20 | 30 | 50 | µs |
| **Respuesta** | LOW por sensor | T_RES_L | 75 | 80 | 85 | µs |
| | HIGH por sensor | T_RES_H | 75 | 80 | 85 | µs |
| **Bit 0** | LOW (inicio de bit) | T_BIT_L | 45 | 50 | 55 | µs |
| | HIGH (representa 0) | T_BIT0_H | 24 | 27 | 30 | µs |
| **Bit 1** | LOW (inicio de bit) | T_BIT_L | 45 | 50 | 55 | µs |
| | HIGH (representa 1) | T_BIT1_H | 68 | 70 | 75 | µs |
| **Fin** | LOW final (liberación del bus) | T_END | 50 | — | — | µs |
| **Repetición** | Intervalo mínimo entre lecturas | T_REP | — | 2000 | — | ms |

### Notas de Temporización

1. La señal START debe durar **al menos 1 ms** (1000 µs). Valores menores pueden no ser detectados por el sensor. Valores mayores (>10 ms) son seguros pero retrasan la lectura.
2. El sensor responde dentro de los **20–50 µs** posteriores a la liberación del bus por parte del maestro.
3. La medición del bit se basa en la **duración del nivel HIGH**. El LOW de 50 µs es constante para todos los bits.
4. El tiempo total de transmisión de 40 bits es de aproximadamente **3–5 ms** (dependiendo de la proporción de bits 0 vs 1).
5. Después de la transmisión, el sensor libera el bus (HIGH) y el maestro debe esperar al menos 2 s antes de la siguiente lectura.

---

## Formato de Trama de Datos (40 bits)

### Estructura

```
  Byte 0      Byte 1      Byte 2      Byte 3      Byte 4
┌────────┬── ─ ─ ─ ───┬────────┬── ─ ─ ─ ───┬────────┐
│ HUM    │ HUM        │ TEMP   │ TEMP        │ CHKSUM │
│ MSB    │ LSB        │ MSB    │ LSB         │        │
│ [15:8] │ [7:0]      │ [15:8] │ [7:0]       │ [7:0]  │
└────────┴────────────┴────────┴─────────────┴────────┘
```

### Interpretación de Bytes

| Byte | Bits | Contenido | Fórmula |
|:---:|:---:|:---|---|
| 0 | 15:8 | Humedad — parte alta | — |
| 1 | 7:0 | Humedad — parte baja | `RH = (byte[0] << 8 \| byte[1]) / 10.0` |
| 2 | 15:8 | Temperatura — parte alta | Bit 15 = signo (1 = negativa) |
| 3 | 7:0 | Temperatura — parte baja | `T = ((byte[2] & 0x7F) << 8 \| byte[3]) / 10.0` (si bit 15=1, negativa) |
| 4 | 7:0 | Checksum | `CHK = (byte[0] + byte[1] + byte[2] + byte[3]) & 0xFF` |

### Ejemplos de Tramas

| Trama (hex) | Humedad | Temperatura | Checksum | Válido |
|:---|---:|---:|:---:|:---:|
| `03 E8 01 09 FB` | 100.0% | 26.5°C | ✅ | ✅ |
| `02 80 01 06 89` | 64.0% | 26.2°C | ✅ | ✅ |
| `00 00 80 02 82` | 0.0% | -0.2°C | ✅ | ✅ |
| `00 00 00 00 00` | 0.0% | 0.0°C | ✅ | ✅ |
| `01 90 00 01 92` | 40.0% | 0.1°C | ✅ | ✅ |
| `03 E8 FF 06 F8` | 100.0% | — | ❌ (checksum) | ❌ |

### Código de Verificación de Checksum

```cpp
uint8_t sum = data[0] + data[1] + data[2] + data[3];
bool valid = ((sum & 0xFF) == data[4]);
```

---

## Características Físicas

### Dimensiones

| Versión | Largo | Ancho | Alto | Paso de pines |
|:---|---:|---:|---:|---:|
| DHT22 (4 pines, sin PCB) | 15.1 mm | 12.0 mm | 5.5 mm | 2.54 mm |
| AM2302 (carcasa blanca, 3 pines) | 18.0 mm | 12.5 mm | 5.5 mm | 2.54 mm |
| Módulo con PCB (AZDelivery, etc.) | 24.0 mm | 14.0 mm | 7.0 mm | 2.54 mm |

### Pinout

```
CONECTOR DHT22 (4 pines, vista frontal)
            ┌──────┐
  VDD ─────┤ 1  4 ├───── GND
  DATA ────┤ 2  3 ├───── NC
           └──────┘
```

| Pin # | Nombre | Descripción | Conectar a |
|:---:|:---|---:|:---|
| 1 | VDD | Alimentación positiva | 3.3V – 5.5V DC |
| 2 | DATA | Bus de datos bidireccional | Pin digital Arduino + pull-up 4.7kΩ |
| 3 | NC | No conectar | — |
| 4 | GND | Tierra | GND |

### Peso

| Componente | Peso |
|:---|---:|
| DHT22 (sensor solo) | ~2.0 g |
| Módulo DHT22 con PCB | ~3.5 g |

---

## Esquema Eléctrico de Referencia

```
                   ┌───── VDD (5V)
                   │
                   ├───[4.7kΩ]──┐
                   │            │
          ┌────────┤            │
          │        │            │
     ┌────┴──┐     │            │
     │       │     │            │
     │ DHT22 │     │            │
     │       │     │            │
     └─┬───┬─┘     │            │
       │   │       │            │
       │   └───────┼────────────┤
       │           │     ┌──────┤
       │          ┌┴┐    │      │
       │          │100nF │      │
       │          └┬┘    │      │
       │           │     │      │
       │           │     │      │
       │     ┌─────┘     │      │
       │     │           │      │
       │   ┌─┴───────────┴──┐   │
       │   │   Arduino UNO  │   │
       │   │              ──┘   │
       │   │   D2 (DATA) ───────┘
       │   │   5V ──────────────
       │   │   GND ─────────────
       └───┤                    │
           └────────────────────┘
```

### Lista de Componentes

| Componente | Valor | Cantidad | Nota |
|:---|---:|:---:|:---|
| DHT22 / AM2302 | — | 1 | |
| Resistencia pull-up | 4.7kΩ – 10kΩ (recomendado: 4.7kΩ) | 1 | 1/4W o SMD 0805 |
| Capacitor cerámico | 100 nF (0.1 µF) | 1 | Desacople VDD–GND |
| Capacitor electrolítico | 10 µF / 16V | 1 | Solo si cables >2 m |
| Arduino Uno (o compatible) | — | 1 | |
| Cable dupont | MM o MF | 3–4 | |

---

## Rangos de Operación por Aplicación

| Aplicación | Temp. típica | Hum. típica | Notas |
|:---|---:|---:|:---|
| Meteorología doméstica | -10 a 45°C | 20–80% | Ideal |
| Invernadero | 10 a 40°C | 50–95% | Proteger de condensación directa |
| Incubadora | 35 a 40°C | 40–70% | Calibrar antes de uso crítico |
| Sauna / Baño turco | **NO RECOMENDADO** | **NO RECOMENDADO** | La condensación destruye el sensor |
| Exterior (intemperie) | -20 a 50°C | 10–100% | Usar carcasa ventilada con filtro de PTFE |
| Industria alimenticia | 0 a 50°C | 20–90% | Sensor debe estar en vaina de acero inoxidable |
| Laboratorio | 10 a 35°C | 30–70% | Excelente relación calidad/precio para uso no crítico |

---

## Limitaciones Conocidas

1. **Autocalentamiento**: El sensor incrementa su temperatura interna ~0.3–0.5°C durante la medición. Necesita 2 s de reposo entre lecturas.
2. **Deriva en humedad >80%**: La precisión se degrada en exposiciones prolongadas a humedad >80% RH. Se requiere recalibración periódica.
3. **No sellado**: El elemento sensor está expuesto al ambiente. Polvo, humo y químicos pueden dañarlo permanentemente.
4. **Sensibilidad a la luz solar directa**: La radiación infrarroja puede calentar la cápsula y falsear lecturas de temperatura.
5. **Tiempo de estabilización**: Al encender, esperar 2 s antes de la primera lectura confiable.
6. **Velocidad de respuesta lenta**: Comparado con sensores de estado sólido (BME280, SHT30), el DHT22 es lento en detectar cambios bruscos.

---

## Comparativa con Sensores Alternativos

| Sensor | Precio (aprox.) | Temp. prec. | Hum. prec. | Max. freq. | Interfaz | Tamaño | Sellado |
|:---|---:|:---:|:---:|:---:|:---:|:---:|:---:|
| **DHT22** | $3–5 | ±0.5°C | ±2% | 0.5 Hz | 1-wire | Medio | ❌ |
| DHT11 | $1–2 | ±2°C | ±5% | 1 Hz | 1-wire | Medio | ❌ |
| BME280 | $3–6 | ±0.5°C | ±3% | 10 Hz | I²C/SPI | Pequeño | ❌ |
| SHT30 | $4–7 | ±0.2°C | ±2% | 10 Hz | I²C | Pequeño | ❌ |
| SHT85 | $8–12 | ±0.1°C | ±1.5% | 10 Hz | I²C | Pequeño | ✅ |
| AM2320 | $3–4 | ±0.5°C | ±3% | 0.5 Hz | I²C/1-wire | Medio | ❌ |

---

## Referencias

- [Aosong AM2302 Datasheet oficial](https://cdn-shop.adafruit.com/datasheets/DHT22.pdf)
- [Aosong DHT22 Datasheet (alternativo)](https://www.sparkfun.com/datasheets/Sensors/Temperature/DHT22.pdf)
- [National Weather Service — Heat Index](https://www.weather.gov/safety/heat-index)
- [AN73xx — Application Note: Cálculo de Heat Index (Rothfusz regression)](https://www.weather.gov/media/ffc/ta_htindx.PDF)
