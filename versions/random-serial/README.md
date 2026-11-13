# random-serial — Generador de Números Aleatorios por Serial

![Arduino](https://img.shields.io/badge/Arduino-ESP32--CAM-blue) ![Serial](https://img.shields.io/badge/Serial-115200--baud-green)

## Descripción

Programa para **ESP32-CAM AI-Thinker** que genera e imprime números aleatorios (rango 1–100) por el puerto Serial cada **2 segundos**.

El código utiliza el **generador de números aleatorios por hardware** (`esp_random()`) del ESP32 para obtener una semilla criptográficamente segura, garantizando secuencias diferentes en cada reinicio.

## Principio de funcionamiento

1. El ESP32 se inicializa y establece comunicación Serial a 115200 baud.
2. Se siembra el generador pseudoaleatorio con un valor del hardware RNG (`esp_random()`).
3. Cada 2 segundos, el programa genera un número entre 1 y 100, lo imprime por Serial y hace parpadear el LED flash del ESP32-CAM (GPIO 4).

## Tabla de pines

| Pin   | Modo     | Componente         | Descripción                     |
|-------|----------|--------------------|---------------------------------|
| TX    | OUTPUT   | USB-UART (CP2102)  | Transmisión Serial a PC         |
| RX    | INPUT    | USB-UART (CP2102)  | Recepción Serial desde PC       |
| GPIO 4 | OUTPUT  | LED Flash integrado| Parpadea al generar cada número |

> **Nota**: GPIO 4 corresponde al LED flash de la cámara del ESP32-CAM. Si tu placa no tiene LED flash, esta salida simplemente no tendrá efecto visible; el programa funciona igual.

## Diagrama de conexión

```
PC (USB) ──── Cable USB-UART ──── ESP32-CAM AI-Thinker
                                     ┌──────────┐
                                     │ ESP32-CAM │
                                     │           │
                                     │ TX ──────► USB
                                     │ RX ◄────── USB
                                     │ GPIO 4 ──► LED Flash
                                     │ GND ────── USB
                                     │ 5V  ────── USB
                                     └──────────┘
```

## Requisitos

- **Placa**: ESP32-CAM AI-Thinker (o cualquier ESP32 compatible)
- **Conexión**: Cable USB-UART (CP2102 o similar)
- **Software**: Arduino CLI `arduino-cli` con el core `esp32:esp32` instalado

## Instalación y uso

### 1. Compilar y subir

```bash
arduino-cli compile --fqbn esp32:esp32:esp32cam ruta/al/proyecto/
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32cam ruta/al/proyecto/
```

### 2. Monitorear la salida Serial

```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### 3. Salida esperada

```
=======================================
 Generador de Numeros Aleatorios
 ESP32-CAM AI-Thinker
 Rango: 1-100 | Intervalo: 2s
=======================================
Random: 42
Random: 87
Random: 13
Random: 95
Random: 6
Random: 51
...
```

Cada número aparece aproximadamente cada 2 segundos.

## Calibración y advertencias

- No se requiere calibración. El programa funciona inmediatamente después de subirlo.
- Asegúrate de que el cable USB-UART tenga los pines TX/RX conectados correctamente (TX→RX, RX→TX).
- El ESP32-CAM necesita una fuente de alimentación adecuada (mínimo 500 mA). Algunos cables USB no proporcionan suficiente corriente; usa un cable de datos de calidad.

## Créditos

Proyecto generado automáticamente por el pipeline Arduino Orchestrator.
