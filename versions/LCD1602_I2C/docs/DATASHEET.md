#   DATASHEET — Módulo LCD1602 con interfaz I2C (PCF8574 + HD44780)

> **Documentación técnica de usuario** — Versión 1.0.0

---

##   1. Identificación del producto

| Atributo | Valor |
|----------|-------|
| **Nombre** | Módulo LCD 16×2 con interfaz I2C |
| **Modelo** | LCD1602 I2C (LCM1602 IIC, YwRobot, HW-061, etc.) |
| **Controlador** | HD44780 (Hitachi/Renesas) o compatibles (ST7066, SPLC780D, KS0066) |
| **Expansor I2C** | PCF8574T o PCF8574AT (NXP) |
| **Interfaz host** | I2C (2 hilos: SDA, SCL) |
| **Tecnología de display** | TN (Twisted Nematic), matriz pasiva, reflectiva con backlight LED |

---

##   2. Especificaciones eléctricas

### 2.1 Condiciones absolutas máximas

| Parámetro | Mín. | Máx. | Unidad |
|-----------|:----:|:----:|:------:|
| Voltaje de alimentación lógica (VDD - VSS) | -0.3 | 7.0 | V |
| Voltaje de alimentación LCD (VDD - VO) | -0.3 | 13.0 | V |
| Voltaje en cualquier pin | -0.3 | VDD + 0.3 | V |
| Corriente por pin de datos (D4-D7) | — | 20 | mA |
| Corriente total (VDD + backlight) | — | 250 | mA |
| Temperatura de operación | -20 | 70 | °C |
| Temperatura de almacenamiento | -30 | 80 | °C |
| Humedad relativa (sin condensación) | — | 90 | % |

### 2.2 Características eléctricas (T<sub>A</sub> = 25°C, VDD = 5.0V)

| Parámetro | Símbolo | Condición | Mín. | Típ. | Máx. | Unidad |
|-----------|:-------:|-----------|:----:|:----:|:----:|:------:|
| Voltaje de alimentación lógica | VDD | — | 4.5 | 5.0 | 5.5 | V |
| Voltaje de alimentación LCD | V0 | — | — | — | VDD | V |
| Corriente de alimentación lógica | IDD | LCD sin backlight | — | 1.0 | 2.0 | mA |
| Corriente de backlight | IBL | LED blanco típico | — | 120 | 200 | mA |
| Corriente total del módulo | ITOT | Backlight + LCD activo | — | 150 | 200 | mA |
| Voltaje alto de entrada (HD44780) | VIH | — | 2.2 | — | VDD | V |
| Voltaje bajo de entrada (HD44780) | VIL | — | -0.3 | — | 0.8 | V |
| Voltaje alto de entrada (PCF8574) | VIH | — | 0.7 × VDD | — | VDD + 0.5 | V |
| Voltaje bajo de entrada (PCF8574) | VIL | — | -0.5 | — | 0.3 × VDD | V |
| Corriente de fuga entrada I2C | ILI | — | -1 | — | 1 | µA |
| Frecuencia de reloj I2C | fSCL | Modo estándar | — | 100 | — | kHz |
| Frecuencia de reloj I2C | fSCL | Modo rápido | — | — | 400 | kHz |
| Resistencia pull-up externa I2C | RP | Recomendada | 1.5 | 4.7 | 10 | kΩ |

### 2.3 Consumo de energía

| Estado | Corriente (típ.) | Potencia (5V) |
|--------|:----------------:|:--------------:|
| LCD apagado (sin alimentación) | 0 µA | 0 mW |
| Backlight OFF, display estático | 1.2 mA | 6 mW |
| Backlight ON (4 LEDs), display activo | 150 mA | 750 mW |
| Backlight ON máximo | 200 mA | 1000 mW |

---

##   3. Especificaciones del display

| Parámetro | Valor |
|-----------|-------|
| Formato de visualización | 16 caracteres × 2 filas |
| Matriz de caracteres | 5 × 8 píxeles (dot matrix) |
| Caracteres totales en pantalla | 32 (16 × 2) |
| Caracteres en CGROM | 208 (incluye ASCII 0x20–0x7F, katakana, símbolos) |
| Caracteres personalizables (CGRAM) | 8 (5 × 8 píxeles cada uno) |
| Tamaño de carácter (ancho × alto) | 2.95 × 5.55 mm (típico) |
| Tamaño de píxel | 0.55 × 0.55 mm (típico) |
| Separación entre caracteres | 0.55 mm |
| Área visible (V.A.) | 64.5 × 16.0 mm |
| Área total del módulo | 80.0 × 36.0 × 12.0 mm (aprox.) |
| Peso | ~40 g |
| Relación de contraste | 1:5 (típico) |
| Ángulo de visión | 6:00 (visto desde abajo, típico) |
| Tipo de polarizador | Reflectivo/Transflectivo |
| Tipo de backlight | LED blanco (4 LEDs en serie-paralelo) |
| Vida útil del backlight | 50,000 horas (típico) |
| MTBF del módulo | 100,000 horas |

---

##   4. Pinout

### 4.1 Pinout del módulo LCD (HD44780, 16 pines)

| Pin # | Símbolo | Función | Dirección | Conectado a |
|:-----:|:-------:|---------|:---------:|-------------|
| 1 | VSS | Tierra (0V) | — | GND |
| 2 | VDD | Alimentación (5V) | — | 5V |
| 3 | V0/VEE | Voltaje de contraste | Entrada | Potenciómetro |
| 4 | RS | Register Select | Entrada | PCF8574 P0 |
| 5 | RW | Read/Write | Entrada | GND (solo escritura) |
| 6 | E | Enable | Entrada | PCF8574 P2 |
| 7 | D0 | Data bit 0 (no usado en 4-bit) | — | NC |
| 8 | D1 | Data bit 1 (no usado en 4-bit) | — | NC |
| 9 | D2 | Data bit 2 (no usado en 4-bit) | — | NC |
| 10 | D3 | Data bit 3 (no usado en 4-bit) | — | NC |
| 11 | D4 | Data bit 4 | Entrada/Salida | PCF8574 P4 |
| 12 | D5 | Data bit 5 | Entrada/Salida | PCF8574 P5 |
| 13 | D6 | Data bit 6 | Entrada/Salida | PCF8574 P6 |
| 14 | D7 | Data bit 7 | Entrada/Salida | PCF8574 P7 |
| 15 | A/LED+ | Ánodo backlight (5V) | — | PCF8574 P3 (a través de transistor) |
| 16 | K/LED- | Cátodo backlight (GND) | — | GND |

### 4.2 Pinout del módulo I2C (conector de 4 pines)

| Pin (cable) | Símbolo | Función | Conectar a |
|:-----------:|:-------:|---------|------------|
| Rojo (o +) | VCC | Alimentación, 5V DC | 5V de Arduino |
| Negro (o -) | GND | Tierra, 0V | GND de Arduino |
| Amarillo (o DA) | SDA | Datos I2C | A4 (Uno) / 20 (Mega) / GPIO21 (ESP32) |
| Verde (o CL) | SCL | Reloj I2C | A5 (Uno) / 21 (Mega) / GPIO22 (ESP32) |

> **⛔ Atención**: El orden de los pines varía según el fabricante. Siempre verifica la serigrafía.

---

##   5. Direcciones I2C del PCF8574

### 5.1 Configuración de dirección

La dirección I2C del PCF8574 se configura mediante los pines A0, A1, A2:

| Chip | A2 | A1 | A0 | Dirección (HEX) | Dirección (DEC) |
|:----:|:--:|:--:|:--:|:---------------:|:---------------:|
| PCF8574T | 0 | 0 | 0 | **0x20** | 32 |
| PCF8574T | 0 | 0 | 1 | 0x21 | 33 |
| PCF8574T | 0 | 1 | 0 | 0x22 | 34 |
| PCF8574T | 0 | 1 | 1 | 0x23 | 35 |
| PCF8574T | 1 | 0 | 0 | 0x24 | 36 |
| PCF8574T | 1 | 0 | 1 | 0x25 | 37 |
| PCF8574T | 1 | 1 | 0 | 0x26 | 38 |
| PCF8574T | 1 | 1 | 1 | **0x27** | 39 |
| PCF8574AT | 0 | 0 | 0 | **0x38** | 56 |
| PCF8574AT | 0 | 0 | 1 | 0x39 | 57 |
| PCF8574AT | 0 | 1 | 0 | 0x3A | 58 |
| PCF8574AT | 0 | 1 | 1 | 0x3B | 59 |
| PCF8574AT | 1 | 0 | 0 | 0x3C | 60 |
| PCF8574AT | 1 | 0 | 1 | 0x3D | 61 |
| PCF8574AT | 1 | 1 | 0 | 0x3E | 62 |
| PCF8574AT | 1 | 1 | 1 | **0x3F** | 63 |

En los módulos LCD1602 I2C comerciales, A0-A2 suelen estar conectados a VCC (PCF8574T → 0x27) o a GND (PCF8574AT → 0x3F), de ahí que las dos direcciones más comunes sean **0x27** y **0x3F**.

### 5.2 Mapa de bits del byte I2C

```
Bit:   7     6     5     4     3     2     1     0
     ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐
     │ D7  │ D6  │ D5  │ D4  │ BL  │  E  │ RW  │ RS  │
     ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤
     │ P7  │ P6  │ P5  │ P4  │ P3  │ P2  │ P1  │ P0  │
     └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
                                                      
     │←─── Dato LCD (4 bits) ──→│  ←────── Control ──────→│
```

Donde:
- **D7–D4** (bits 7–4): Datos del bus HD44780 (nibble superior/inferior)
- **BL** (bit 3): Backlight (1 = encendido, 0 = apagado)
- **E** (bit 2): Enable (pulso positivo)
- **RW** (bit 1): Read/Write (fijo a 0 = escritura)
- **RS** (bit 0): Register Select (0 = comando, 1 = dato)

---

##   6. Temporización del HD44780 (modo 4 bits)

### 6.1 Tiempos de ciclo de escritura

| Parámetro | Símbolo | Condición | Mín. | Típ. | Máx. | Unidad |
|-----------|:-------:|-----------|:----:|:----:|:----:|:------:|
| Tiempo de setup E → datos | tAS | — | 40 | — | — | ns |
| Ancho de pulso E alto | tPW | — | 150 | — | — | ns |
| Tiempo de hold datos → E↓ | tAH | — | 10 | — | — | ns |
| Tiempo de recuperación E | tCYC | — | 200 | — | — | ns |
| Tiempo de ejecución de comando | tEX | Clear/Home | — | 1.64 | 2.0 | ms |
| Tiempo de ejecución otros cmd | tEX | Otros comandos | — | 40 | — | µs |
| Tiempo de estabilización power-on | tPON | VDD → inicio | — | 15 | 50 | ms |

### 6.2 Diagrama de temporización de escritura

```
       ┌────┐              ┌──────┐              ┌──────┐
  E    │    │              │      │              │      │
  ─────┘    └──────────────┘      └──────────────┘      └──
       │<tCYC>│                    │                      │
       │<tPW>│                                              │
       │     │                                              │
  RS   │     │◄── tAS ──►│                                  │
  ─────┘     └──────────────────────────────────────────────
             │                                              │
  D4-D7      │◄──── tAH ────►│                              │
  ───────────┘      ────────┘──────────────────────────────
              DATO              DATO
```

### 6.3 Secuencia de inicialización del HD44780

| Orden | Acción | Valor | Descripción | Espera |
|:-----:|--------|:----:|-------------|:------:|
| 1 | Esperar tras power-on | — | Estabilización del LCD | >15 ms |
| 2 | Write 4-bit (D7-D4 = 0011) | 0x30 | Configuración 8-bit (intento 1) | >4.1 ms |
| 3 | Write 4-bit (D7-D4 = 0011) | 0x30 | Configuración 8-bit (intento 2) | >100 µs |
| 4 | Write 4-bit (D7-D4 = 0011) | 0x30 | Configuración 8-bit (intento 3) | >100 µs |
| 5 | Write 4-bit (D7-D4 = 0010) | 0x20 | Cambio a modo 4-bit | >100 µs |
| 6 | Function Set | 0x28 | 4-bit, 2 líneas, 5×8 | >40 µs |
| 7 | Display OFF | 0x08 | Apagar display durante config | >40 µs |
| 8 | Clear Display | 0x01 | Borrar DDRAM | >1.64 ms |
| 9 | Entry Mode Set | 0x06 | Incrementar, sin shift | >40 µs |
| 10 | Display ON | 0x0C | Encender, cursor OFF, blink OFF | >40 µs |

---

##   7. Tabla de caracteres CGROM (ASCII)

El HD44780 incluye una tabla de caracteres estándar en la CGROM. A continuación se muestran los caracteres más relevantes (modo europeo):

| Código | Carácter | Código | Carácter | Código | Carácter | Código | Carácter |
|:------:|:--------:|:------:|:--------:|:------:|:--------:|:------:|:--------:|
| 0x20 | (espacio) | 0x40 | @ | 0x60 | ` |
| 0x21 | ! | 0x41 | A | 0x61 | a |
| 0x22 | " | 0x42 | B | 0x62 | b |
| 0x23 | # | 0x43 | C | 0x63 | c |
| 0x24 | $ | 0x44 | D | 0x64 | d |
| 0x25 | % | 0x45 | E | 0x65 | e |
| 0x26 | & | 0x46 | F | 0x66 | f |
| 0x27 | ' | 0x47 | G | 0x67 | g |
| 0x28 | ( | 0x48 | H | 0x68 | h |
| 0x29 | ) | 0x49 | I | 0x69 | i |
| 0x2A | * | 0x4A | J | 0x6A | j |
| 0x2B | + | 0x4B | K | 0x6B | k |
| 0x2C | , | 0x4C | L | 0x6C | l |
| 0x2D | - | 0x4D | M | 0x6D | m |
| 0x2E | . | 0x4E | N | 0x6E | n |
| 0x2F | / | 0x4F | O | 0x6F | o |
| 0x30 | 0 | 0x50 | P | 0x70 | p |
| 0x31 | 1 | 0x51 | Q | 0x71 | q |
| 0x32 | 2 | 0x52 | R | 0x72 | r |
| 0x33 | 3 | 0x53 | S | 0x73 | s |
| 0x34 | 4 | 0x54 | T | 0x74 | t |
| 0x35 | 5 | 0x55 | U | 0x75 | u |
| 0x36 | 6 | 0x56 | V | 0x76 | v |
| 0x37 | 7 | 0x57 | W | 0x77 | w |
| 0x38 | 8 | 0x58 | X | 0x78 | x |
| 0x39 | 9 | 0x59 | Y | 0x79 | y |
| 0x3A | : | 0x5A | Z | 0x7A | z |
| 0x3B | ; | 0x5B | [ | 0x7B | { |
| 0x3C | < | 0x5C | ¥ | 0x7C | \| |
| 0x3D | = | 0x5D | ] | 0x7D | } |
| 0x3E | > | 0x5E | ^ | 0x7E | → |
| 0x3F | ? | 0x5F | _ | 0x7F | ← |

> **Nota**: Los códigos 0x00–0x07 corresponden a caracteres personalizables (CGRAM). Los códigos 0x08–0x1F contienen símbolos japoneses/katakana.

---

##   8. Características mecánicas del conector

| Parámetro | Valor |
|-----------|-------|
| Tipo de conector I2C | Header hembra 4 pines (PHR-4) |
| Pitch del conector I2C | 2.54 mm |
| Tipo de conector LCD | Pines macho 16 pines |
| Pitch del conector LCD | 2.54 mm |
| Distancia entre agujeros de montaje | 75.0 × 31.0 mm |
| Diámetro de agujeros de montaje | 3.2 mm |
| Espesor del PCB | 1.6 mm |

---

##   9. Información sobre pedidos

| Código de producto | Descripción | Voltaje | Dirección I2C típica |
|-------------------|-------------|:-------:|:--------------------:|
| LCM1602 IIC | Módulo LCD 16×2 + adaptador I2C | 5V | 0x27 |
| HW-061 | Módulo LCD 16×2 con I2C integrado | 5V | 0x27 |
| YwRobot 1602 I2C | Módulo LCD + PCF8574 | 5V | 0x27 o 0x3F |
| DFRobot LCD1602 I2C | Módulo LCD + I2C (Gravity) | 5V | 0x3F |
| SunFounder LCD1602 I2C | Módulo LCD + I2C | 5V | 0x27 |

---

<div align="center">

**Fin del datasheet** — LCD1602_I2C v1.0.0

[← Volver al README principal](README.md) · [Documentación para desarrolladores →](DEVELOPER.md)

</div>
