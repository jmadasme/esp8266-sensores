<div align="center">

#   LCD1602_I2C

**Controlador para display LCD 16×2 con interfaz I2C (PCF8574 + HD44780)**

![Arduino Compatible](https://img.shields.io/badge/Arduino-Compatible-00979D?style=for-the-badge&logo=arduino&logoColor=white)
![Version](https://img.shields.io/badge/version-1.0.0-blue?style=for-the-badge)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)
![Category](https://img.shields.io/badge/category-Display-FF6F00?style=for-the-badge)
![I2C](https://img.shields.io/badge/I2C-PCF8574-8A2BE2?style=for-the-badge)

---

[🇪🇸 Español](README.md) &nbsp;•&nbsp; _English version coming soon_

---

<p align="center">
  <b>Librería completa, robusta y fácil de usar para displays LCD 1602<br>
  con interfaz I2C en Arduino. Sin cables, sin complicaciones.</b>
</p>

**⭐ Detección automática de dirección · Caracteres personalizados · Scroll · Barra de progreso · Control de retroiluminación · Texto centrado**

</div>

---

##   Tabla de Contenidos

1. [Descripción General](#-descripción-general)
2. [Principio Físico de Funcionamiento](#-principio-físico-de-funcionamiento)
3. [El Chip HD44780](#-el-chip-hd44780)
4. [El Expansor I2C PCF8574](#-el-expansor-i2c-pcf8574)
5. [Especificaciones Técnicas](#-especificaciones-técnicas)
6. [Pinout y Conexiones](#-pinout-y-conexiones)
7. [Instalación](#-instalación)
8. [Guía de Inicio Rápido](#-guía-de-inicio-rápido)
9. [Ejemplos](#-ejemplos)
10. [Referencia de la Clase LCD1602_I2C](#-referencia-de-la-clase-lcd1602_i2c)
11. [Ajuste de Contraste (Potenciómetro)](#-ajuste-de-contraste-potenciómetro)
12. [Detección de Dirección I2C](#-detección-de-dirección-i2c)
13. [Advertencias y Cuidados](#-advertencias-y-cuidados)
14. [Solución de Problemas](#-solución-de-problemas)
15. [Licencia](#-licencia)

---

##   Descripción General

El **LCD1602_I2C** es un módulo que combina un display LCD de 16 caracteres × 2 filas (controlado por el conocido chip **HD44780**) con un expansor de puertos **PCF8574** que permite la comunicación a través del bus I2C. Esto reduce drásticamente la cantidad de pines necesarios: de **12 pines** en modo paralelo tradicional a solo **2 pines** (SDA + SCL) más alimentación.

###   ¿Por qué usar I2C en lugar del modo paralelo?

| Característica | Modo Paralelo (12 pines) | Modo I2C (2 pines) |
|----------------|:------------------------:|:------------------:|
| Pines necesarios | 12 | 2 (SDA, SCL) |
| Pines GPIO libres en Arduino Uno | 2 de 14 | 12 de 14 |
| Velocidad | Alta (varias MHz) | 100-400 kHz |
| Cables | 16 saltos | 4 cables |
| Complejidad de cableado | Alta (propensa a errores) | Mínima |

###   Características principales

| Funcionalidad | Descripción |
|---------------|-------------|
| **Detección automática de dirección I2C** | `beginAuto()` escanea el bus y detecta 0x27, 0x3F o cualquier otra dirección |
| **Control de retroiluminación** | Enciende/apaga el backlight por software (`backlightOn()`, `backlightOff()`) |
| **Caracteres personalizados** | Hasta 8 caracteres de 5×8 píxeles, ideales para iconos y animaciones |
| **Scroll horizontal** | Desplazamiento automático de texto (`scrollText()`, `scrollLeft()`, `scrollRight()`) |
| **Barra de progreso animada** | Indicador visual de nivel con 6 niveles de llenado |
| **Texto centrado** | Centrado automático en cualquier fila (`printCenter()`) |
| **Control de cursor** | Mostrar/ocultar cursor, activar/desactivar parpadeo |
| **Compatibilidad multiplataforma** | Arduino Uno, Mega, Nano, Leonardo, ESP8266, ESP32, etc. |

---

## ⚛ Principio Físico de Funcionamiento

### Cristal Líquido Nemático (LCD)

El display LCD 1602 utiliza la tecnología de **cristal líquido nemático** (Nematic Liquid Crystal). Cada carácter está compuesto por una matriz de **5×8 píxeles** (40 puntos por carácter). El principio de funcionamiento es el siguiente:

1. **Moléculas nemáticas**: En estado normal (sin campo eléctrico), las moléculas de cristal líquido se alinean paralelas a las ranuras de las placas de vidrio, torciendo la luz polarizada 90° y permitiendo que pase a través del polarizador frontal (el píxel aparece **transparente/claro**).

2. **Aplicación de voltaje**: Cuando se aplica un voltaje entre los electrodos de un píxel, las moléculas se reorientan perpendicularmente a las placas, **sin torcer la luz polarizada**. Esto bloquea el paso de la luz, haciendo que el píxel aparezca **oscuro**.

3. **Matriz pasiva**: El LCD 1602 utiliza una matriz pasiva donde cada píxel no tiene su propio transistor; en su lugar, se multiplexan las filas y columnas. El controlador HD44780 refresca continuamente la pantalla para mantener la imagen.

4. **Retroiluminación**: Detrás del panel de cristal líquido hay una capa de LEDs blancos (backlight) que ilumina los píxeles desde atrás. Cuando un píxel está "apagado" (sin voltaje), la luz pasa y el píxel se ve claro. Cuando un píxel está "encendido" (con voltaje), bloquea la luz y se ve oscuro (formando el carácter).

### El Chip HD44780

El **HD44780** (fabricado por Hitachi, ahora Renesas) es el controlador LCD más utilizado del mundo. Es un circuito integrado diseñado específicamente para controlar displays alfanuméricos LCD.

**Arquitectura interna del HD44780:**

```
                ┌─────────────────────────────────────┐
                │          HD44780                     │
                │                                     │
  RS ──────────►│  Register Select                    │
  RW ──────────►│  Read/Write                         │
  E  ──────────►│  Enable                             │
                │                                     │
  D4 ──────────►│  ┌─────────────┐   ┌─────────────┐  │
  D5 ──────────►│  │ Instruction │   │    Data     │  │
  D6 ──────────►│  │  Register   │   │  Register   │  │
  D7 ──────────►│  │    (IR)     │   │    (DR)     │  │
                │  └──────┬──────┘   └──────┬──────┘  │
                │         │                 │         │
                │  ┌──────┴──────────────────┴──────┐  │
                │  │     Address Control Circuit     │  │
                │  └──────┬──────────────────┬──────┘  │
                │         │                  │         │
                │  ┌──────┴──────┐   ┌───────┴───────┐ │
                │  │  DDRAM      │   │    CGRAM      │ │
                │  │  (80 bytes) │   │  (64 bytes)   │ │
                │  └─────────────┘   └───────────────┘ │
                │         │                             │
                │  ┌──────┴──────────────────────────┐  │
                │  │      LCD Driver / Segment       │  │
                │  │      (COM + SEG outputs)        │  │
                │  └─────────────────────────────────┘  │
                └─────────────────────────────────────┘
```

**Memorias internas del HD44780:**

| Memoria | Tamaño | Propósito |
|---------|:------:|-----------|
| **DDRAM** (Display Data RAM) | 80 bytes | Almacena los caracteres visibles en pantalla. Cada byte corresponde a una posición. El LCD 1602 usa 32 posiciones (16×2) de las 80 disponibles. |
| **CGRAM** (Character Generator RAM) | 64 bytes | Almacena hasta 8 caracteres personalizados de 8 bytes cada uno (5 bits por fila). |
| **CGROM** (Character Generator ROM) | 208 caracteres | Contiene la tabla de caracteres predeterminados (ASCII, japonés, símbolos). Es de solo lectura. |

**Registros de control:**

- **IR (Instruction Register)**: Recibe comandos (RS=0) como clear, home, cursor shift, function set, etc.
- **DR (Data Register)**: Almacena datos temporalmente (RS=1) para escritura/lectura desde DDRAM o CGRAM.

**Comandos principales del HD44780:**

| Comando | Código | Descripción | Tiempo de ejecución |
|---------|:------:|-------------|:-------------------:|
| Clear Display | `0x01` | Limpia DDRAM, cursor a home | 1.64 ms |
| Return Home | `0x02` | Cursor a (0,0), sin borrar DDRAM | 1.64 ms |
| Entry Mode Set | `0x06` | Incremento automático, sin shift | 40 µs |
| Display ON/OFF | `0x0C` | Display ON, cursor OFF, blink OFF | 40 µs |
| Display ON (cursor) | `0x0E` | Display ON, cursor ON | 40 µs |
| Display ON (blink) | `0x0F` | Display ON, cursor ON, blink ON | 40 µs |
| Cursor/Display Shift | `0x14` | Mover cursor a la derecha | 40 µs |
| Display Shift Left | `0x18` | Desplazar pantalla a la izquierda | 40 µs |
| Display Shift Right | `0x1C` | Desplazar pantalla a la derecha | 40 µs |
| Function Set | `0x28` | 4 bits, 2 líneas, 5×8 dots | 40 µs |
| Set CGRAM Address | `0x40 + addr` | Direccionar CGRAM para escritura | 40 µs |
| Set DDRAM Address | `0x80 + addr` | Direccionar DDRAM para escritura | 40 µs |

### El Expansor I2C PCF8574

El **PCF8574** (o PCF8574AT) es un expansor de puertos I/O de 8 bits con interfaz I2C fabricado por NXP (antes Philips). Actúa como un puente entre el bus I2C de Arduino y el bus paralelo del HD44780.

**Características del PCF8574:**

| Parámetro | Valor |
|-----------|-------|
| Voltaje de operación | 2.5V – 6V |
| Corriente máxima por pin | 25 mA |
| Corriente total máxima | 80 mA |
| Frecuencia I2C | 100 kHz (modo estándar), hasta 400 kHz (modo rápido) |
| Direcciones I2C disponibles | 8 (PCF8574T: 0x20–0x27; PCF8574AT: 0x38–0x3F) |
| Número de pines I/O | 8 (bidireccionales, cuasi-bidireccionales) |
| Salida en alta | Pull-up interno de 100 µA |
| Salida en baja | MOSFET de drenaje abierto |

**Mapa de pines PCF8574 → HD44780 en el módulo LCD1602 I2C:**

```
PCF8574                     HD44780 / LCD
─────────                   ──────────────
P0 (bit 0) ───────────────► RS  (Register Select)
P1 (bit 1) ───────────────► RW  (Read/Write)  ← normalmente a GND
P2 (bit 2) ───────────────► E   (Enable)
P3 (bit 3) ───────────────► BL  (Backlight LED)
P4 (bit 4) ───────────────► D4  (Dato, bit 4)
P5 (bit 5) ───────────────► D5  (Dato, bit 5)
P6 (bit 6) ───────────────► D6  (Dato, bit 6)
P7 (bit 7) ───────────────► D7  (Dato, bit 7)
```

**Protocolo de comunicación HD44780 en modo 4 bits:**

El HD44780 soporta dos modos de comunicación: **8 bits** (usa D0–D7) y **4 bits** (usa D4–D7). El modo 4 bits es el estándar para módulos I2C porque permite usar solo 4 líneas de datos del PCF8574.

Secuencia de escritura de un byte en modo 4 bits:

```
1. Separar el byte en dos nibbles: HIGH (bits 7-4) y LOW (bits 3-0)
2. Escribir el nibble HIGH en D4-D7 con RS=0/1 según comando/dato
3. Generar pulso en E (flanco ascendente → descendente)
4. Escribir el nibble LOW en D4-D7
5. Generar pulso en E

Cada escritura en el PCF8574 requiere:
  a. Colocar datos y RS en los bits correspondientes
  b. Poner E = 0
  c. Poner E = 1 (el LCD captura los datos en este flanco)
  d. Poner E = 0
```

**Secuencia de inicialización del LCD (modo 4 bits):**

```
┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐
│ 0x30 │  │ 0x30 │  │ 0x30 │  │ 0x20 │  │ 0x28 │  │ 0x0C │  │ 0x06 │
│ 15ms │  │ 5ms  │  │ 150µs│  │ 150µs│  │ 40µs │  │ 40µs │  │ 40µs │
└──────┘  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘
   │          │         │         │         │         │         │
   ▼          ▼         ▼         ▼         ▼         ▼         ▼
 Inicio    Reset    Reset    Modo     Function   Display   Entry
 (>15ms)   (5ms)    (150µs)  4 bits   Set:4bit   ON       Mode
                                        2 líneas           Increment
                                        5×8 dots           No shift
```

---

##   Especificaciones Técnicas

### Display LCD 1602

| Parámetro | Valor |
|-----------|-------|
| **Modelo** | LCD1602 (16 caracteres × 2 filas) |
| **Controlador** | HD44780 (Hitachi/Renesas) o compatible (Sitronix ST7066, etc.) |
| **Matriz de caracteres** | 5 × 8 píxeles por carácter |
| **Caracteres en CGROM** | 208 (alfanuméricos ASCII, katakana, símbolos) |
| **Caracteres personalizables** | Hasta 8 (en CGRAM) |
| **Tensión de alimentación lógica (VDD)** | 4.5V – 5.5V (típico 5.0V) |
| **Tensión de contraste (V0/VEE)** | 0V – 5V (ajustable con potenciómetro) |
| **Corriente típica (LCD + backlight)** | ~120 – 200 mA (típico 150 mA con backlight) |
| **Corriente sin backlight** | ~1 – 2 mA |
| **Temperatura de operación** | 0°C – 50°C |
| **Temperatura de almacenamiento** | -20°C – 70°C |
| **Tiempo de respuesta** | ~150 – 250 ms |
| **Área visible** | 64.5 × 16.0 mm (aprox.) |
| **Dimensiones del módulo** | 80 × 36 × 12 mm (aprox.) |
| **Peso** | ~40 g |

### Módulo I2C (PCF8574)

| Parámetro | Valor |
|-----------|-------|
| **Chip** | PCF8574T (dirección 0x27) o PCF8574AT (dirección 0x3F) |
| **Interfaz** | I2C (2 hilos: SDA, SCL) |
| **Velocidad I2C** | 100 kHz (standard) / 400 kHz (fast mode) |
| **Voltaje de operación** | 2.5V – 6.0V |
| **Corriente en reposo** | ~10 µA |
| **Corriente activa** | ~1 mA |
| **Resistencia pull-up I2C típica** | 4.7 kΩ – 10 kΩ |
| **Conector** | Header hembra de 4 pines (VCC, GND, SDA, SCL) |

### Consumo total del módulo LCD1602_I2C

| Estado | Consumo típico |
|--------|:--------------:|
| Backlight encendido, todos los píxeles activos | ~160 mA |
| Backlight encendido, modo reposo | ~120 mA |
| Backlight apagado, LCD activo | ~2 mA |
| Backlight apagado, LCD en reposo | ~0.5 mA |

**⚠️ Advertencia**: El Arduino Uno puede suministrar hasta ~500 mA en total por el pin 5V. Un único LCD1602 consume ~150 mA, lo cual es seguro. Sin embargo, si añades otros módulos (servos, motores, matrices LED), considera usar una fuente externa.

---

##   Pinout y Conexiones

### Conector del módulo LCD1602 I2C

El módulo I2C suele tener un conector hembra de **4 pines**:

```
    ┌──────────┐
    │  GND     │ ← Tierra (0V)
    │  VCC     │ ← Alimentación (5V)
    │  SDA     │ ← Datos I2C
    │  SCL     │ ← Reloj I2C
    └──────────┘
```

> **Nota**: En algunos módulos el orden puede variar. Verifica siempre la serigrafía de tu módulo.

### Diagrama de conexión con Arduino Uno

```
   LCD1602 I2C                  Arduino Uno
   ───────────                  ───────────
      GND    ─────────────────► GND (pin adyacente a 5V)
      VCC    ─────────────────► 5V
      SDA    ─────────────────► A4 (SDA)
      SCL    ─────────────────► A5 (SCL)
```

**Conexión física (textual):**

```
  Arduino Uno                    Módulo LCD1602 I2C
  ┌─────────────┐               ┌──────────────────┐
  │             │               │                  │
  │  [RST]      │               │   ┌──────────┐   │
  │  [3.3V]     │               │   │ GND VCC  │   │
  │  [5V]  ─────┼───────────────┼───┤ ●    ●   │   │
  │  [GND] ─────┼───────────────┼───┤ ●    ●   │   │
  │  [GND]      │               │   │ SDA SCL  │   │
  │  [Vin]      │               │   └──────────┘   │
  │             │               │                  │
  │  [A0]       │               │   ┌────────────┐  │
  │  [A1]       │               │   │░░░░░░░░░░░░│  │
  │  [A2]       │               │   │░░ LCD 16x2 │  │
  │  [A3]       │               │   │░░░░░░░░░░░░│  │
  │  [A4] SDA ──┼───────────────┼───┤░░░░░░░░░░░░│  │
  │  [A5] SCL ──┼───────────────┼───┤░░░░░░░░░░░░│  │
  │             │               │   └────────────┘  │
  │  [D0] RX    │               │                  │
  │  [D1] TX    │               │   ┌──┐           │
  │  ...        │               │   │  │ Potencióm.│
  │  [D13]      │               │   └──┘           │
  └─────────────┘               └──────────────────┘
```

### Conexión para otras placas

| Placa | SDA | SCL | 5V | GND |
|-------|:---:|:---:|:--:|:---:|
| **Arduino Uno / Nano / Mini** | A4 | A5 | 5V | GND |
| **Arduino Mega 2560** | 20 | 21 | 5V | GND |
| **Arduino Leonardo** | 2 | 3 | 5V | GND |
| **ESP8266 (NodeMCU, Wemos)** | D2 (GPIO4) | D1 (GPIO5) | 3.3V* | GND |
| **ESP32** | GPIO21 | GPIO22 | 3.3V* / 5V** | GND |
| **Raspberry Pi Pico** | GP0 | GP1 | 5V (VSYS) | GND |
| **STM32 (Blue Pill)** | PB7 | PB6 | 5V | GND |

> **\*** Los módulos LCD1602 I2C funcionan a 5V. En placas de 3.3V (ESP8266, ESP32), el bus I2C puede funcionar en algunos casos, pero el contraste puede ser bajo y el backlight más tenue. Se recomienda un **conversor de nivel lógico** o alimentar el módulo a 5V externo.
>
> **\*\*** Algunos ESP32 tienen pin de 5V (VIN) cuando se alimentan por USB.

---

##   Instalación

### Opción 1: Desde el IDE de Arduino (Recomendada)

1. Abre el **Arduino IDE**.
2. Ve a **Sketch → Include Library → Manage Libraries** (o presiona `Ctrl+Shift+I`).
3. En el gestor de librerías, busca `LCD1602_I2C`.
4. Haz clic en **Install**.

### Opción 2: Desde archivo ZIP (GitHub)

1. Descarga el repositorio como ZIP desde [GitHub](https://github.com/opencode/LCD1602_I2C).
2. En el **Arduino IDE**, ve a **Sketch → Include Library → Add .ZIP Library**.
3. Selecciona el archivo ZIP descargado.
4. La librería quedará instalada en `~/Arduino/libraries/LCD1602_I2C/`.

### Opción 3: Manual (Linux / macOS)

```bash
# Clonar el repositorio
git clone https://github.com/opencode/LCD1602_I2C.git

# Copiar al directorio de librerías de Arduino
cp -r LCD1602_I2C ~/Arduino/libraries/
```

En Windows, copia la carpeta a `Documentos/Arduino/libraries/`.

### Opción 4: Instalación de dependencias

Esta librería depende únicamente de **Wire.h**, que viene incluida en el Arduino IDE. No requiere librerías adicionales para su funcionamiento básico.

Para el ejemplo **DHTMonitor** con sensor real, instala:
- **DHT sensor library** de Adafruit (vía gestor de librerías)

### Verificar la instalación

Una vez instalada, reinicia el IDE Arduino y verifica que aparece en:
**File → Examples → LCD1602_I2C**

Deberías ver los 5 ejemplos:
- `HelloWorld`
- `CustomChars`
- `ScrollingText`
- `I2CScanner`
- `DHTMonitor`

---

##   Guía de Inicio Rápido

### Hello World en 3 minutos

**Paso 1**: Conecta el LCD1602 I2C al Arduino Uno:
- VCC → 5V
- GND → GND
- SDA → A4
- SCL → A5

**Paso 2**: Carga este código:

```cpp
#include <Wire.h>
#include <LCD1602_I2C.h>

LCD1602_I2C lcd(0x27, 16, 2);

void setup() {
    Serial.begin(9600);

    if (!lcd.beginAuto()) {
        Serial.println(F("ERROR: LCD no detectado"));
        while (1) delay(1000);
    }

    lcd.backlightOn();
    lcd.clear();
    lcd.printCenter("Hola Mundo!", 0);
    lcd.setCursor(0, 1);
    lcd.print("LCD OK");
}

void loop() {
    // No hace nada - texto estático
}
```

**Paso 3**: Compila y sube. ¡Deberías ver "Hola Mundo!" en tu LCD!

> **💡 Tip**: Si no ves nada, ajusta el potenciómetro de contraste (tornillo en la parte posterior del módulo I2C) con un destornillador pequeño.

### Flujo de trabajo típico

```
┌─────────────────────────────────────────────────────────┐
│                    FLUJO DE TRABAJO                      │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  1. #include <LCD1602_I2C.h>                             │
│  2. LCD1602_I2C lcd(direccion, columnas, filas);         │
│  3. lcd.beginAuto();   // o lcd.begin()                   │
│  4. lcd.backlightOn(); // encender retroiluminación      │
│  5. lcd.clear();       // limpiar pantalla               │
│  6. lcd.print("texto"); // mostrar texto                 │
│  7. lcd.setCursor(col, fila); // posicionar cursor       │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

---

##   Ejemplos

### 1. HelloWorld — Texto estático y contador

**Archivo**: `examples/HelloWorld/HelloWorld.ino`

**Propósito**: Demostrar la funcionalidad básica de la librería: inicialización, texto centrado, posicionamiento de cursor, impresión de números enteros y control de retroiluminación.

**Comportamiento**:
- Muestra "Hola Mundo!" centrado en la fila 0
- Muestra un contador incremental en la fila 1
- Alterna la retroiluminación cada 10 ciclos

**Salida esperada en LCD**:
```
│         Hola Mundo!         │  ← Fila 0 (centrado)
│         Contador: 42        │  ← Fila 1 (contador)
```

**Loop**:
```cpp
void loop() {
    static int counter = 0;

    lcd.printCenter("Hola Mundo!", 0);
    lcd.setCursor(0, 1);
    lcd.print("Contador: ");
    lcd.print(counter);
    counter++;

    if (counter % 10 == 0)
        lcd.setBacklight(!lcd.isBacklightOn());

    delay(1000);
}
```

**Conceptos aprendidos**:
- `beginAuto()` para detección automática de dirección
- `printCenter()` para centrar texto
- `setCursor()` para posicionar el cursor
- `print(int)` para números
- `backlightOn()` / `backlightOff()` / `setBacklight()` / `isBacklightOn()`

---

### 2. CustomChars — Caracteres personalizados y barra de progreso

**Archivo**: `examples/CustomChars/CustomChars.ino`

**Propósito**: Demostrar la creación de caracteres personalizados (formato 5×8) y la barra de progreso animada.

**Comportamiento** (4 partes en secuencia):

**Parte 1 — Caracteres estáticos personalizados**:
- Muestra un **corazón** (índice 6) y una **flecha derecha** (índice 7)
- Usa `createChar()` para definirlos y `displayChar()` para mostrarlos

**Parte 2 — Animación de carga**:
- 4 frames de animación (usando índices 0-3)
- Se redefinen y muestran secuencialmente

**Parte 3 — Barra de progreso**:
- Anima una barra de 0% a 100% usando `displayProgressBar()`
- Usa los índices 0-5 (6 caracteres: vacío, 1/5, 2/5, 3/5, 4/5, lleno)

**Parte 4 — Combinación**:
- Corazones decorativos en los extremos de la fila 1
- Barra de progreso al 75% entre ellos

**Salida esperada en LCD**:
```
│         CustomChars          │
│    Corazon: ♥   Flecha: →   │
```
*(tras ~5s, pasa a la animación de carga)*

```
│         Cargando...          │
│        █ (animación)         │
```
*(tras ~3s, pasa a la barra de progreso)*

```
│          Progreso:           │
│ ████████████░░░░            │  ← 75%
```
*(tras ~2s, pasa a la demo combinada)*

```
│         Demo completa!       │
│ ♥ ████████████░░░░░░░░░ ♥   │  ← 50% entre corazones
```

**Creación de caracteres personalizados**:

Cada carácter se define como un arreglo de **8 bytes**, donde cada byte representa una fila de **5 bits** (bit 4 = columna izquierda, bit 0 = columna derecha):

```
   Bit:  4 3 2 1 0
         x x x x x   ← Fila 0 (arriba)
         x x x x x   ← Fila 1
         x x x x x   ← Fila 2
         x x x x x   ← Fila 3
         x x x x x   ← Fila 4
         x x x x x   ← Fila 5
         x x x x x   ← Fila 6
         x x x x x   ← Fila 7 (abajo)
```

Ejemplo — corazón:
```cpp
const uint8_t heart[8] PROGMEM = {
    0b01010,  // .♦.♦.
    0b11111,  // ♦♦♦♦♦
    0b11111,  // ♦♦♦♦♦
    0b11111,  // ♦♦♦♦♦
    0b01110,  // .♦♦♦.
    0b00100,  // ..♦..
    0b00000,  // .....
    0b00000   // .....
};
```

**Conceptos aprendidos**:
- `createChar(index, data_array)` para definir caracteres
- `displayChar(index, col, row)` para mostrarlos
- Uso de `PROGMEM` para almacenar datos en Flash
- `displayProgressBar()` para barras de progreso
- Las barras de progreso sobrescriben los índices 0-5

---

### 3. ScrollingText — Desplazamiento de texto

**Archivo**: `examples/ScrollingText/ScrollingText.ino`

**Propósito**: Demostrar el desplazamiento automático de textos que exceden el ancho del display (16 caracteres).

**Comportamiento** (4 demostraciones en secuencia):

**Demo 1 — Texto largo**:
- El texto `"Bienvenido a la libreria LCD1602_I2C para Arduino! Este texto se desplaza automaticamente... "` (más de 90 caracteres) se desplaza horizontalmente de derecha a izquierda
- Usa `scrollText()` con un delay de 250 ms por paso

**Demo 2 — Texto corto**:
- El texto `"Hola!"` (5 caracteres) es más corto que el display
- `scrollText()` lo muestra centrado automáticamente

**Demo 3 — scrollLeft / scrollRight**:
- Escribe texto estático en ambas líneas
- Desplaza el contenido 5 posiciones a la izquierda y luego 5 a la derecha

**Demo 4 — Scroll continuo manual**:
- Combina llamadas a `scrollLeft()` y `scrollRight()` en bucles

**Salida esperada en LCD**:
```
│      Scroll texto:           │
│ Bienvenido a la libreria...  │  ← desplazándose
```
*(el texto se mueve gradualmente a la izquierda)*

**Funcionamiento de scrollText()**:

```
TEXTO ORIGINAL: "Bienvenido a la libreria LCD1602_I2C..."
                    │
                    ▼
POSICIÓN INICIAL:  Bienvenido a la  (solo primeros 16 caracteres visibles)
DESPUÉS DE 1 PASO: ienvenido a la l
DESPUÉS DE 2 PASOS:envenido a la li
...
DESPUÉS DE N PASOS: ...to se desplaza
```

**Conceptos aprendidos**:
- `scrollText(texto, delayMs)` para desplazamiento automático
- `scrollLeft(cols, delayMs)` y `scrollRight(cols, delayMs)` para control manual
- Manejo automático de textos cortos (centrado) vs largos (scroll)

---

### 4. I2CScanner — Escáner de bus I2C

**Archivo**: `examples/I2CScanner/I2CScanner.ino`

**Propósito**: Herramienta de diagnóstico para detectar dispositivos en el bus I2C. Escanea todas las direcciones (1-127), muestra resultados en el monitor Serial y opcionalmente en el LCD.

**Comportamiento**:
- Escanea el bus I2C cada 5 segundos
- Muestra las direcciones encontradas en formato hexadecimal
- Identifica dispositivos conocidos (PCF8574, BMP280, etc.)
- Muestra el total de dispositivos encontrados
- Si el LCD está conectado, también muestra el resultado en pantalla

**Salida esperada en Serial**:
```
╔══════════════════════════════════╗
║     I2C Scanner v1.0             ║
╚══════════════════════════════════╝

Escaneando bus I2C...
Direcciones detectadas:
  0x27  (PCF8574T / LCD)
  0x5C  (AM2320)

Total: 2 dispositivo(s)
────────────────────────────────────
Próximo escaneo en 5 segundos...
```

**Salida en LCD**:
```
│          I2C Scan OK            │
│       Encontrados: 2           │
```

**Salida si no hay dispositivos**:
```
│          I2C Scan OK            │
│       Sin dispositivos         │
```

**Conceptos aprendidos**:
- Uso de `Wire.beginTransmission()` + `endTransmission()` para detectar dispositivos
- `beginAuto()` como parte del flujo de diagnóstico
- Identificación de direcciones I2C comunes

---

### 5. DHTMonitor — Monitor ambiental con DHT11

**Archivo**: `examples/DHTMonitor/DHTMonitor.ino`

**Propósito**: Aplicación práctica que monitorea temperatura y humedad usando un sensor DHT11 (o DHT22) y muestra los datos en el LCD.

**Comportamiento**:
- Lee temperatura y humedad cada 2 segundos
- Muestra iconos personalizados (termómetro y gota de agua)
- Barra de progreso que representa visualmente el nivel de humedad
- **Modo simulado** por defecto (no requiere sensor físico)
- Opción de **modo real** (DHT físico) cambiando `#define USE_REAL_DHT 1`

**Salida esperada en LCD**:
```
│ ♦ Temp: 25.3°C                  │
│ ♦ Hum:  60.7%  ████████░░░     │
```

(arriba: temperatura, con icono de termómetro; abajo: humedad, con icono de gota y barra de progreso)

**Salida en modo real**:

Si `USE_REAL_DHT = 1` y hay un DHT11 conectado:
```
│ ♦ Temp: 22.5°C                  │
│ ♦ Hum:  55.0%  ██████░░░░░     │
```

**Salida en caso de error del DHT**:
```
│          ERROR DHT               │
│        Revisar sensor           │
```

**Caracteres personalizados utilizados**:

| Índice | Símbolo | Descripción |
|:------:|:-------:|-------------|
| 6 | `♦` (termómetro) | Indica temperatura |
| 7 | `♠` (gota) | Indica humedad |

**Diagrama de conexión con DHT11 real**:

```
   LCD1602 I2C         DHT11              Arduino Uno
   ───────────         ─────              ───────────
      GND      ────────┬───────► GND
      VCC      ────────┼───────► 5V
      SDA      ────────┤                A4 (SDA)
      SCL      ────────┤                A5 (SCL)
                        │
                       OUT ───────────► D7 (Digital)
```

**Función de simulación**:

```cpp
void simulateDHT(float& temp, float& hum) {
    float deltaTemp = (random(-10, 11)) * 0.1f;   // -1.0 a +1.0 °C
    float deltaHum  = (random(-15, 16)) * 0.1f;   // -1.5 a +1.5 %
    simTemp += deltaTemp;
    simHum  += deltaHum;
    // Mantener rangos realistas: temp 18-35°C, hum 30-90%
    if (simTemp < 18.0f) simTemp = 18.0f;
    if (simTemp > 35.0f) simTemp = 35.0f;
    if (simHum  < 30.0f) simHum  = 30.0f;
    if (simHum  > 90.0f) simHum  = 90.0f;
    temp = simTemp;
    hum  = simHum;
}
```

**Conceptos aprendidos**:
- Integración de LCD con sensores analógicos/digitales
- Uso de caracteres personalizados como iconografía
- Actualización periódica de datos en pantalla
- Manejo de errores de sensor
- Simulación de datos para desarrollo sin hardware

---

##   Referencia de la Clase LCD1602_I2C

### Constructores

| Método | Descripción |
|--------|-------------|
| `LCD1602_I2C(uint8_t addr, uint8_t cols = 16, uint8_t rows = 2)` | Crea una instancia con dirección I2C, número de columnas y filas |

### Inicialización y configuración

| Método | Descripción | Retorno |
|--------|-------------|:-------:|
| `bool begin()` | Inicializa el LCD usando la dirección del constructor | `true` si OK |
| `bool begin(uint8_t addr)` | Inicializa con dirección específica (8-119) | `true` si OK |
| `bool beginAuto()` | Escanea el bus I2C y detecta dirección automáticamente (prioriza 0x27 y 0x3F) | `true` si OK |

### Información del dispositivo

| Método | Descripción | Retorno |
|--------|-------------|:-------:|
| `uint8_t getAddress()` | Retorna la dirección I2C activa | Dirección I2C |
| `uint8_t getCols()` | Retorna el número de columnas | Columnas |
| `uint8_t getRows()` | Retorna el número de filas | Filas |

### Control de cursor y pantalla

| Método | Descripción |
|--------|-------------|
| `void setCursor(uint8_t col, uint8_t row)` | Posiciona el cursor en (col, fila) |
| `void home()` | Lleva el cursor a (0, 0) |
| `void clear()` | Limpia la pantalla y cursor a home |

### Salida de texto

| Método | Descripción |
|--------|-------------|
| `void print(const char* str)` | Muestra una cadena de texto |
| `void print(int value)` | Muestra un número entero |
| `void print(float value, uint8_t decimals = 2)` | Muestra un número decimal con N decimales |
| `void printCenter(const char* str, uint8_t row)` | Centra y muestra texto en una fila |

### Retroiluminación

| Método | Descripción |
|--------|-------------|
| `void backlightOn()` | Enciende la retroiluminación |
| `void backlightOff()` | Apaga la retroiluminación |
| `void setBacklight(bool on)` | Controla el estado de la retroiluminación |
| `bool isBacklightOn()` | Retorna `true` si la retroiluminación está encendida |

### Caracteres personalizados

| Método | Descripción |
|--------|-------------|
| `void createChar(uint8_t index, const uint8_t* data)` | Define un carácter personalizado en CGRAM (índice 0-7, data[8] de 5 bits cada byte) |
| `void displayChar(uint8_t index, uint8_t col, uint8_t row)` | Muestra un carácter personalizado en una posición |

### Desplazamiento (scroll)

| Método | Descripción |
|--------|-------------|
| `void scrollLeft(uint8_t cols, uint16_t delayMs)` | Desplaza el contenido a la izquierda |
| `void scrollRight(uint8_t cols, uint16_t delayMs)` | Desplaza el contenido a la derecha |
| `void scrollText(const char* text, uint16_t delayMs = 300)` | Desplaza texto largo automáticamente. Texto corto: centrado. |

### Apariencia del cursor

| Método | Descripción |
|--------|-------------|
| `void blinkCursor(bool enable)` | Activa/desactiva el parpadeo del cursor |
| `void showCursor(bool enable)` | Muestra/oculta el cursor subrayado |

### Efectos visuales

| Método | Descripción |
|--------|-------------|
| `void displayProgressBar(uint8_t percent, uint8_t col, uint8_t row, uint8_t length)` | Barra de progreso animada (0-100%). Usa índices CGRAM 0-5. |

### Tabla resumen de métodos

| Método | ¿Bloqueante? | ¿Usa Serial? | Tiempo típico |
|--------|:------------:|:------------:|:-------------:|
| `begin()` / `beginAuto()` | Sí | Sí | ~200 ms |
| `clear()` | Sí | No | 2 ms |
| `home()` | Sí | No | 2 ms |
| `setCursor()` | No | No | 50 µs |
| `print(const char*)` | Por carácter | No | 50 µs/char |
| `print(int)` | Depende de la longitud | No | 50 µs/digito |
| `print(float, n)` | Depende de la longitud | No | 50 µs/char |
| `printCenter()` | Por carácter | No | 50 µs/char |
| `scrollText()` | Sí | No | delayMs × (len - cols) |
| `scrollLeft()` / `scrollRight()` | Sí | No | delayMs × cols |
| `createChar()` | Sí (8 bytes) | No | ~400 µs |
| `displayChar()` | No | No | 50 µs |
| `backlightOn()` / `backlightOff()` | No | Sí (si error I2C) | ~3 ms |
| `displayProgressBar()` | Sí | No | ~300 µs + createChar |
| `blinkCursor()` / `showCursor()` | No | No | 50 µs |

---

##   Ajuste de Contraste (Potenciómetro)

El módulo LCD1602 I2C tiene un **potenciómetro ajustable** (tornillo pequeño) en la parte posterior de la placa I2C. Este controla el voltaje de contraste (pin V0/VEE del HD44780).

### Procedimiento de ajuste

1. **Conecta y alimenta** el módulo LCD.
2. **Carga el ejemplo HelloWorld** para que el LCD muestre texto.
3. **Localiza el potenciómetro**: Es un tornillo plateado o azul en la placa I2C.
4. **Gira lentamente** con un destornillador pequeño (cruz o plano, según el modelo):
   - **En sentido horario** → Mayor contraste (caracteres más oscuros)
   - **En sentido antihorario** → Menor contraste (caracteres más claros)
5. **Ajusta hasta que** los caracteres sean **nítidos y legibles** sin que el fondo se oscurezca demasiado.

```
   POTENCIÓMETRO
   ┌──────────┐
   │  ═══╪═══ │  ← Gira lentamente
   └──────────┘
      │    │
      │    └── Más contraste (sentido horario)
      └─────── Menos contraste (sentido antihorario)
```

### Síntomas y soluciones

| Síntoma | Causa probable | Solución |
|---------|---------------|----------|
| Pantalla completamente en blanco, sin rectángulo de caracteres | Contraste muy bajo | Girar potenciómetro en sentido horario |
| Rectángulos negros en toda la pantalla | Contraste muy alto | Girar potenciómetro en sentido antihorario |
| Caracteres visibles pero muy tenues | Contraste bajo o voltaje bajo | Ajustar potenciómetro o verificar 5V |
| Solo se ve la fila superior | Contraste incorrecto o mala conexión | Reajustar y verificar cableado |

> **💡 Tip**: Si incluso con el potenciómetro no logras buena visibilidad, verifica que el módulo reciba exactamente **5.0V**. Voltajes menores (como 3.3V) producen contraste pobre.

---

##   Detección de Dirección I2C

### Direcciones comunes

| Chip | Dirección I2C | Uso en el módulo LCD |
|------|:-------------:|----------------------|
| **PCF8574T** | **0x27** (39 decimal) | Módulos I2C estándar |
| **PCF8574AT** | **0x3F** (63 decimal) | Variante del expansor |

### Método 1: Detección automática (recomendado)

Usa `beginAuto()` para que la librería escanee y detecte la dirección automáticamente:

```cpp
LCD1602_I2C lcd(0x27, 16, 2);

void setup() {
    Serial.begin(9600);
    
    if (!lcd.beginAuto()) {
        Serial.println(F("ERROR: No se detectó el LCD"));
        while (1);
    }
    
    Serial.print(F("LCD detectado en 0x"));
    Serial.println(lcd.getAddress(), HEX);
}
```

### Método 2: Escáner I2C dedicado

Carga el ejemplo **I2CScanner** desde el IDE:
**File → Examples → LCD1602_I2C → I2CScanner**

Este escánea todo el bus I2C y muestra las direcciones detectadas.

### Método 3: Manual con el monitor Serial

```cpp
#include <Wire.h>

void setup() {
    Wire.begin();
    Serial.begin(9600);
    
    for (uint8_t addr = 1; addr < 128; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print(F("Dispositivo en 0x"));
            Serial.println(addr, HEX);
        }
    }
}

void loop() {}
```

### Algoritmo de detección automática (`beginAuto()`)

```
┌─────────────────────────────────┐
│         beginAuto()             │
├─────────────────────────────────┤
│                                 │
│  1. Wire.begin()                │
│  2. Probar 0x27                 │
│     ├─ ¿Responde? → Sí → ADDR = 0x27, initLCD()
│     └─ ¿Responde? → No → Continuar
│  3. Probar 0x3F                 │
│     ├─ ¿Responde? → Sí → ADDR = 0x3F, initLCD()
│     └─ ¿Responde? → No → Continuar
│  4. Escanear 8...119           │
│     ├─ ¿Alguna responde? → Sí → ADDR = addr, initLCD()
│     └─ Ninguna → Error: false  │
│                                 │
└─────────────────────────────────┘
```

---

## ⚠ Advertencias y Cuidados

### ⚡ Eléctricos

| ❌ No hacer | ✅ Hacer |
|------------|---------|
| Conectar el LCD a 3.3V (el módulo requiere 5V) | Usar siempre 5V para alimentación |
| Invertir VCC y GND (puede dañar el módulo permanentemente) | Verificar polaridad antes de conectar |
| Superar 5.5V en VCC | Usar una fuente regulada de 5V |
| Conectar SDA/SCL a pines de alta tensión | Usar resistencias pull-up de 4.7kΩ si no están incluidas |
| Desconectar cables mientras el módulo está alimentado | Apagar antes de hacer cambios en el cableado |

###   Físicos

| ❌ No hacer | ✅ Hacer |
|------------|---------|
| Ejercer presión sobre el panel LCD (el vidrio es frágil) | Manipular con cuidado, sujetar por los bordes |
| Exponer a temperaturas >50°C o <-20°C | Operar dentro del rango 0°C–50°C |
| Exponer a luz solar directa por tiempo prolongado | Ubicar en interiores o con protección UV |
| Limpiar con solventes (alcohol, acetona, thinner) | Limpiar con paño suave y seco o ligeramente humedecido con agua |
| Dejar caer o golpear el módulo | Almacenar en caja acolchonada cuando no se use |
| Doblar o forzar los pines del conector | Alinear recto al insertar |

###   Operativos

| ❌ No hacer | ✅ Hacer |
|------------|---------|
| Llamar a `print()` sin haber inicializado (`begin()`) | Siempre verificar que `begin()` retorne `true` |
| Usar índices >7 en `createChar()` o `displayChar()` | Índices válidos: 0–7 (solo 8 caracteres en CGRAM) |
| Esperar que `displayProgressBar()` conserve caracteres previos | Los índices 0–5 se sobrescriben al usar la barra |
| Enviar texto que exceda los 40 caracteres a `scrollText()` | La librería trunca a 40 (límite DDRAM), pero verificar |
| Asumir que `begin(addr)` acepta cualquier dirección | Solo direcciones I2C válidas: 8–119 |
| Usar `blinkCursor()` y `showCursor()` sin considerar el estado mutuo | Ambos flags se combinan correctamente con `_updateDisplayMode()` |

### 🔧 Instalación

| ❌ No hacer | ✅ Hacer |
|------------|---------|
| Instalar múltiples versiones de la librería (conflictos) | Desinstalar versiones antiguas antes de actualizar |
| Editar los archivos de la librería en `Arduino/libraries/` | Hacer fork del repositorio si necesitas personalizar |
| Olvidar incluir `Wire.h` antes de `LCD1602_I2C.h` | Siempre incluir primero `Wire.h`, luego `LCD1602_I2C.h` |

---

##   Solución de Problemas

### Problema: La pantalla no muestra nada

| Posible causa | Solución |
|--------------|----------|
| Dirección I2C incorrecta | Usar `beginAuto()` o el escáner I2C |
| Contraste mal ajustado | Girar potenciómetro lentamente |
| Cableado incorrecto | Verificar VCC→5V, GND→GND, SDA→A4, SCL→A5 |
| Falta de alimentación | Medir 5V entre VCC y GND con multímetro |
| Dirección I2C no soportada | Solo 0x27 (PCF8574T) y 0x3F (PCF8574AT) son comunes |

### Problema: Caracteres borrosos o ilegibles

| Posible causa | Solución |
|--------------|----------|
| Contraste mal ajustado | Ajustar potenciómetro |
| Voltaje de alimentación bajo | Medir 5V exactos |
| Conexiones flojas | Revisar cables jumper |
| Frecuencia I2C incompatible | Reducir a 100 kHz (estándar) |

### Problema: Solo se ven rectángulos negros

| Posible causa | Solución |
|--------------|----------|
| Contraste demasiado alto | Girar potenciómetro en sentido antihorario |
| Inicialización fallida | Verificar `begin()` retorna `true` |
| Secuencia de inicialización incorrecta | Revisar cableado y dirección I2C |

### Problema: El LCD parpadea o titila

| Posible causa | Solución |
|--------------|----------|
| Alimentación insuficiente | Usar fuente externa si hay muchos módulos |
| Pull-ups I2C faltantes | Agregar resistencias 4.7kΩ en SDA y SCL a 5V |
| Interferencia electromagnética | Alejar cables de fuentes de ruido |

### Problema: La retroiluminación no funciona

| Posible causa | Solución |
|--------------|----------|
| `backlightOn()` no se llamó | Llamar después de `begin()` |
| PCF8574 dañado | Probar con otro módulo |
| Bit BL no se envía correctamente | Verificar función `_writeExpander()` |

### Problema: `beginAuto()` retorna `false`

| Posible causa | Solución |
|--------------|----------|
| No hay LCD conectado | Verificar cableado |
| Dirección I2C fuera de rango | Probar con dirección manual |
| Bus I2C bloqueado | Resetear Arduino y desconectar/reconectar |
| Pull-ups I2C dañados | Medir resistencia entre SDA/SCL y 5V |

---

##   Licencia

```
MIT License

Copyright (c) 2025 OpenCode

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

<div align="center">

**LCD1602_I2C** — Librería para Arduino

[Reportar un problema](https://github.com/opencode/LCD1602_I2C/issues) · [Solicitar una funcionalidad](https://github.com/opencode/LCD1602_I2C/issues) · [Ver documentación del desarrollador](DEVELOPER.md)

</div>
