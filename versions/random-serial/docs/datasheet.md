# Datasheet — random-serial

## Identificación

| Campo              | Valor                                      |
|--------------------|--------------------------------------------|
| Nombre             | random-serial                              |
| Versión            | 1.0.0                                      |
| Placa objetivo     | ESP32-CAM AI-Thinker                       |
| MCU                | ESP32-D0WD-V3 (Xtensa LX6, 240 MHz, 2 núcleos) |
| Framework          | Arduino (core esp32:esp32:esp32cam v3.3.10) |
| Lenguaje           | C++ (Arduino Wiring)                       |

## Especificaciones técnicas

| Parámetro              | Valor          | Notas                    |
|------------------------|----------------|--------------------------|
| Baudrate Serial        | 115200         | 8N1                      |
| Intervalo generación   | 2000 ms        | ±1 ms por delay()        |
| Rango de números       | 1–100          | Enteros, distribución uniforme |
| Seed RNG               | esp_random()   | Hardware RNG del ESP32   |
| Consumo de Flash       | 298,747 bytes  | 9.5% de 4 MB             |
| Consumo de RAM         | 22,252 bytes   | 6.8% de 320 KB           |
| Frecuencia CPU         | 240 MHz        | Default                  |
| Voltaje operación      | 5V (VIN) / 3.3V (regulado) | |

## Pines utilizados

| Pin      | Función      | Tipo     | Especificaciones eléctricas           |
|----------|--------------|----------|---------------------------------------|
| TX (U0TXD) | Serial TX  | Salida   | 3.3V CMOS, 115200 baud                |
| RX (U0RXD) | Serial RX  | Entrada  | 3.3V CMOS, 115200 baud                |
| GPIO 4   | LED Flash    | Salida   | 3.3V, 12 mA máx (LED interno)         |

## Comunicación

- **Protocolo**: Serial asíncrono (UART)
- **Formato**: Texto ASCII plano, líneas terminadas en `\r\n`
- **Baudrate**: 115200 bps
- **Bits de datos**: 8
- **Paridad**: Ninguna
- **Bits de parada**: 1

### Formato de los mensajes

```
Random: <número>\r\n
```

Donde `<número>` es un entero decimal entre 1 y 100.

## Comportamiento temporal

```
Estado: INICIO → [2s] → Random: N → [2s] → Random: N+1 → [2s] → ...
```

El LED flash (GPIO 4) se enciende 100 ms durante cada impresión de número.

## Requisitos de alimentación

- **Voltaje de entrada**: 5V DC (por pin VIN o conector USB-UART)
- **Corriente mínima**: 500 mA
- **Conector**: Micro USB (solo datos y alimentación)

## Compatibilidad

| Placa               | FQBN                       | Compatible |
|---------------------|----------------------------|:----------:|
| ESP32-CAM AI-Thinker | esp32:esp32:esp32cam       | ✅          |
| ESP32-DevKitC       | esp32:esp32:esp32          | ✅          |
| ESP32-WROOM-32      | esp32:esp32:esp32          | ✅          |
| ESP32-S3            | esp32:esp32:esp32s3        | ⚠️ GPIO4 puede variar |

## Limitaciones conocidas

- El programa usa `delay()` bloqueante. No puede realizar otras tareas durante la espera.
- Sin conexión WiFi ni Bluetooth (las radios no se inicializan).
- El LED flash solo parpadea si la placa tiene el LED conectado a GPIO 4.
