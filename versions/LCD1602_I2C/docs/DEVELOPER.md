#   DEVELOPER — Documentación para desarrolladores

> **Arquitectura, diseño y extensión de la librería LCD1602_I2C** — v1.0.0

---

##   Tabla de Contenidos

1. [Arquitectura de la librería](#-arquitectura-de-la-librería)
2. [Estructura de archivos](#-estructura-de-archivos)
3. [Diagrama de clases](#-diagrama-de-clases)
4. [Protocolo HD44780 modo 4 bits](#-protocolo-hd44780-modo-4-bits)
5. [Mapa de pines PCF8574 → HD44780](#-mapa-de-pines-pcf8574--hd44780)
6. [Referencia completa de métodos](#-referencia-completa-de-métodos)
7. [Detección automática de dirección I2C](#-detección-automática-de-dirección-i2c)
8. [Secuencia de inicialización](#-secuencia-de-inicialización)
9. [Manejo de errores](#-manejo-de-errores)
10. [Cómo extender la librería](#-cómo-extender-la-librería)
11. [Pruebas y verificación](#-pruebas-y-verificación)

---

##   Arquitectura de la librería

### Visión general

La librería **LCD1602_I2C** implementa el protocolo de comunicación **HD44780 en modo 4 bits** a través de un expansor **PCF8574** conectado al bus I2C de Arduino. La arquitectura sigue un diseño en capas:

```
┌─────────────────────────────────────────────────────────────┐
│                     CÓDIGO DE USUARIO                        │
│  (#include <LCD1602_I2C.h>)                                  │
│  lcd.beginAuto(); lcd.print("Hola"); lcd.setCursor(0,1);    │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                LCD1602_I2C (API pública)                     │
│  ┌───────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐  │
│  │ Inicializ.│ │  Texto   │ │ Cursor   │ │  Efectos      │  │
│  │ begin()   │ │ print()  │ │ home()   │ │ scrollText()  │  │
│  │ beginAuto │ │ print()  │ │ clear()  │ │ progBar()     │  │
│  │           │ │ center() │ │ setCur.. │ │ createChar()  │  │
│  │ getAddr() │ │ print()  │ │ blink()  │ │ displayChar() │  │
│  └─────┬─────┘ └─────┬───┘ └────┬─────┘ └───────┬───────┘  │
│        │              │          │                │          │
│        └──────────────┴──────────┴────────────────┘          │
│                           │                                  │
│                    ┌──────┴──────┐                           │
│                    │  _sendCmd() │                           │
│                    │  _sendData()│                           │
│                    └──────┬──────┘                           │
└───────────────────────────┼─────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              CAPA DE PROTOCOLO HD44780 (4 bits)             │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  _writeByte(data, rs)                                   │ │
│  │    ├── _writeNibble(highNibble, rs)  [E pulse]         │ │
│  │    └── _writeNibble(lowNibble, rs)   [E pulse]         │ │
│  └──────────────────────┬──────────────────────────────────┘ │
└─────────────────────────┼───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│               CAPA DE COMUNICACIÓN I2C                       │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  _writeExpander(data)                                   │ │
│  │    ├── Wire.beginTransmission(_addr)                    │ │
│  │    ├── Wire.write(data)                                 │ │
│  │    └── Wire.endTransmission() → error check             │ │
│  └─────────────────────────────────────────────────────────┘ │
│                                                               │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │  _detectAddress(addr)                                   │ │
│  │    ├── Wire.beginTransmission(addr)                     │ │
│  │    └── return (Wire.endTransmission() == 0)             │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                  HARDWARE                                     │
│  ┌───────────────┐          ┌──────────────────────────────┐ │
│  │  Arduino      ├──I2C──►  │  PCF8574 (expansor)          │ │
│  │  (Wire lib)   │          │  └──► HD44780 (LCD 16×2)    │ │
│  └───────────────┘          └──────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Flujo de datos de una operación típica

```
Usuario: lcd.print('A', 0, 0)
   │
   ▼
print(cadena) ──► bucle sobre caracteres
   │
   ▼
_sendData('A')
   │
   ▼
_writeByte(0x41, RS=1)       // 'A' = 0x41
   │
   ├── _writeNibble(0x04, 1)  // nibble alto = 0x4
   │     └── _writeExpander(0x44)  // BL on, E=0, RS=1
   │     └── _writeExpander(0x4C)  // E=1 (pulso)
   │     └── _writeExpander(0x44)  // E=0
   │
   └── _writeNibble(0x01, 1)  // nibble bajo = 0x1
         └── _writeExpander(0x41)  // BL on, E=0, RS=1
         └── _writeExpander(0x49)  // E=1 (pulso)
         └── _writeExpander(0x41)  // E=0
```

---

##   Estructura de archivos

```
LCD1602_I2C/                           # Raíz de la librería
│
├── LCD1602_I2C.h                      # Header: declaración de la clase
│   ├── class LCD1602_I2C
│   │   ├── Métodos públicos (API)
│   │   └── Métodos privados (implementación)
│   └── Constantes y macros
│
├── LCD1602_I2C.cpp                    # Implementación completa
│   ├── Constructor
│   ├── Inicialización (begin, beginAuto)
│   ├── Control LCD (clear, home, setCursor)
│   ├── Texto (print, printCenter)
│   ├── Retroiluminación (backlightOn/Off)
│   ├── Caracteres personalizados (createChar, displayChar)
│   ├── Scroll (scrollLeft, scrollRight, scrollText)
│   ├── Cursor (blinkCursor, showCursor)
│   ├── Barra de progreso (displayProgressBar)
│   ├── Comunicación I2C (_writeExpander, _detectAddress)
│   ├── Protocolo HD44780 (_writeNibble, _writeByte)
│   ├── Comandos HD44780 (_sendCommand, _sendData)
│   └── Inicialización interna (_initLCD, _createProgressChars)
│
├── library.properties                 # Metadatos para Arduino IDE
│
└── examples/
    ├── HelloWorld/HelloWorld.ino      # Ejemplo básico
    ├── CustomChars/CustomChars.ino    # Caracteres personalizados
    ├── ScrollingText/ScrollingText.ino# Scroll de texto
    ├── I2CScanner/I2CScanner.ino     # Escáner de bus I2C
    └── DHTMonitor/DHTMonitor.ino      # Monitor DHT11

docs/                                  # Documentación
├── README.md                          # Documentación principal
├── DATASHEET.md                       # Ficha técnica del hardware
└── DEVELOPER.md                       # Esta guía para desarrolladores
```

---

##   Diagrama de clases

```
┌──────────────────────────────────────────────┐
│               LCD1602_I2C                     │
├──────────────────────────────────────────────┤
│ - _addr: uint8_t                              │
│ - _cols: uint8_t                              │
│ - _rows: uint8_t                              │
│ - _backlight: bool                            │
│ - _initialized: bool                          │
│ - _cursorVisible: bool                        │
│ - _cursorBlink: bool                          │
├──────────────────────────────────────────────┤
│ + LCD1602_I2C(addr, cols, rows)              │
│                                               │
│ + begin(): bool                               │
│ + begin(addr): bool                           │
│ + beginAuto(): bool                           │
│ + getAddress(): uint8_t                       │
│ + getCols(): uint8_t                          │
│ + getRows(): uint8_t                          │
│                                               │
│ + setCursor(col, row): void                   │
│ + home(): void                                │
│ + clear(): void                               │
│                                               │
│ + print(const char*): void                    │
│ + print(int): void                            │
│ + print(float, decimals): void                │
│ + printCenter(str, row): void                 │
│                                               │
│ + backlightOn(): void                         │
│ + backlightOff(): void                        │
│ + setBacklight(on): void                      │
│ + isBacklightOn(): bool                       │
│                                               │
│ + createChar(index, data): void               │
│ + displayChar(index, col, row): void          │
│                                               │
│ + scrollLeft(cols, delayMs): void             │
│ + scrollRight(cols, delayMs): void            │
│ + scrollText(text, delayMs): void             │
│                                               │
│ + blinkCursor(enable): void                   │
│ + showCursor(enable): void                    │
│                                               │
│ + displayProgressBar(percent, col, row, len): │
│                                               │
│ # _writeExpander(data): bool                  │
│ # _writeNibble(nibble, rs): void              │
│ # _writeByte(data, rs): void                  │
│ # _sendCommand(cmd): void                     │
│ # _sendData(data): void                       │
│ # _detectAddress(addr): bool                  │
│ # _updateDisplayMode(): void                  │
│ # _initLCD(): bool                            │
│ # _createProgressChars(): void                │
└──────────────────────────────────────────────┘
```

---

##   Protocolo HD44780 modo 4 bits

### Fundamentos

El HD44780 puede operar en modo **8 bits** (usando D0–D7) o **4 bits** (usando solo D4–D7). El modo 4 bits reduce la cantidad de pines necesarios de 11 (RS, RW, E, D0–D7) a 6 (RS, RW, E, D4–D7). Con el PCF8574, que tiene solo 8 bits I/O, esta reducción es esencial.

En modo 4 bits:
1. Cada **byte** se transmite como **dos nibbles** (4 bits cada uno).
2. Primero se envía el **nibble superior** (bits 7–4).
3. Luego se envía el **nibble inferior** (bits 3–0).
4. Cada nibble va acompañado de un **pulso en E** (enable) para que el LCD lo capture.

### La función `_writeNibble`

Esta es la función core del protocolo. Transfiere 4 bits de datos al LCD generando un pulso en E:

```cpp
void LCD1602_I2C::_writeNibble(uint8_t nibble, uint8_t rs)
{
    // Construir byte para PCF8574:
    //   bits 7-4 = nibble de datos (D7-D4 del HD44780)
    //   bit 3    = BL (backlight)
    //   bit 2    = E  (enable)
    //   bit 1    = RW (siempre 0 → solo escritura)
    //   bit 0    = RS (0=comando, 1=dato)
    uint8_t data = (nibble << 4)
                 | (rs ? 0x01 : 0x00)
                 | (_backlight ? 0x08 : 0x00);

    _writeExpander(data & ~0x04);  // E = 0 (preparar)
    delayMicroseconds(1);          // tAS > 40 ns

    _writeExpander(data | 0x04);   // E = 1 (flanco ascendente: LCD captura)
    delayMicroseconds(1);          // tPW > 150 ns

    _writeExpander(data & ~0x04);  // E = 0 (flanco descendente)
    delayMicroseconds(1);          // hold
}
```

### La función `_writeByte`

Ensambla dos nibbles para formar un byte completo:

```cpp
void LCD1602_I2C::_writeByte(uint8_t data, uint8_t rs)
{
    _writeNibble(data >> 4, rs);    // Nibble superior (bits 7-4)
    _writeNibble(data & 0x0F, rs); // Nibble inferior (bits 3-0)
}
```

### La función `_writeExpander`

Capa más baja: escribe un byte en el PCF8574 vía I2C:

```cpp
bool LCD1602_I2C::_writeExpander(uint8_t data)
{
    Wire.beginTransmission(_addr);
    Wire.write(data);
    uint8_t error = Wire.endTransmission();
    if (error != 0) {
        Serial.print(F("LCD1602_I2C: I2C error "));
        Serial.print(error);
        Serial.print(F(" at 0x"));
        Serial.println(_addr, HEX);
        return false;
    }
    return true;
}
```

### Diagrama de temporización de `_writeNibble`

```
                     ┌──────────────┐
PCF8574 output:      │  data & ~0x04│  ← E = 0
                     └──────────────┘
                         │ 1 µs
                         ▼
                     ┌──────────────┐
                     │  data \| 0x04 │  ← E = 1 (pulso)
                     └──────────────┘
                         │ 1 µs
                         ▼
                     ┌──────────────┐
                     │  data & ~0x04│  ← E = 0
                     └──────────────┘
```

---

##   Mapa de pines PCF8574 → HD44780

| Pin PCF8574 | Bit | Señal HD44780 | Descripción | Estado típico |
|:-----------:|:---:|:-------------:|-------------|:-------------:|
| P0 | 0 | RS | Register Select (0=comando, 1=dato) | Según operación |
| P1 | 1 | RW | Read/Write (0=escritura, 1=lectura) | Siempre 0 (GND) |
| P2 | 2 | E | Enable (flanco positivo captura datos) | 0, pulso a 1 |
| P3 | 3 | BL | Backlight LED (0=apagado, 1=encendido) | 1 (encendido) |
| P4 | 4 | D4 | Dato, bit 4 (LSB del nibble) | Según dato |
| P5 | 5 | D5 | Dato, bit 5 | Según dato |
| P6 | 6 | D6 | Dato, bit 6 | Según dato |
| P7 | 7 | D7 | Dato, bit 7 (MSB del nibble) | Según dato |

### Construcción del byte I2C

```
Bit:  7    6    5    4    3    2    1    0
     D7   D6   D5   D4   BL    E    RW   RS

Byte = (nibble << 4) | (BL << 3) | (E << 2) | (RW << 1) | RS

Ejemplo: Enviar comando 0x28 con backlight ON, E=0:
  nibble_high = 0x2, nibble_low = 0x8
  data_high = (0x2 << 4) | (1 << 3) | (0 << 2) | (0 << 1) | 0
            = 0x20 | 0x08 | 0x00 | 0x00 | 0x00
            = 0x28
  data_low  = (0x8 << 4) | (1 << 3) | (0 << 2) | (0 << 1) | 0
            = 0x80 | 0x08 | 0x00 | 0x00 | 0x00
            = 0x88
```

---

##   Referencia completa de métodos

### Métodos públicos

#### Constructor

```cpp
LCD1602_I2C(uint8_t addr, uint8_t cols = 16, uint8_t rows = 2);
```

Inicializa los miembros privados con valores por defecto:
- `_addr = addr`
- `_cols = cols`
- `_rows = rows`
- `_backlight = true`
- `_initialized = false`
- `_cursorVisible = false`
- `_cursorBlink = false`

No inicia la comunicación I2C (eso ocurre en `begin()`).

---

#### `bool begin()`

```cpp
bool LCD1602_I2C::begin() {
    Wire.begin();
    if (!_detectAddress(_addr)) {
        Serial.print(F("LCD1602_I2C: No device found at 0x"));
        Serial.println(_addr, HEX);
        return false;
    }
    return _initLCD();
}
```

**Flujo**:
1. Inicializa `Wire` (bus I2C).
2. Verifica que haya un dispositivo en `_addr`.
3. Si no hay respuesta, imprime error y retorna `false`.
4. Si hay respuesta, ejecuta `_initLCD()`.
5. Retorna `true` si la inicialización fue exitosa.

---

#### `bool begin(uint8_t addr)`

```cpp
bool LCD1602_I2C::begin(uint8_t addr) {
    if (addr < 8 || addr > 119) {
        Serial.print(F("LCD1602_I2C: Invalid I2C address 0x"));
        Serial.println(addr, HEX);
        return false;
    }
    _addr = addr;
    return begin();
}
```

**Validaciones**:
- La dirección debe estar entre **8 y 119** (rango I2C válido para dispositivos: 0x08–0x77).
- Si la dirección es inválida, imprime error y retorna `false` sin tocar el I2C.

---

#### `bool beginAuto()`

```cpp
bool LCD1602_I2C::beginAuto()
{
    Wire.begin();

    // 1. Probar direcciones comunes
    const uint8_t commonAddrs[] = {0x27, 0x3F};
    for (uint8_t i = 0; i < 2; i++) {
        if (_detectAddress(commonAddrs[i])) {
            _addr = commonAddrs[i];
            return _initLCD();
        }
    }

    // 2. Escaneo completo del bus I2C
    for (uint8_t addr = 8; addr < 120; addr++) {
        if (_detectAddress(addr)) {
            _addr = addr;
            return _initLCD();
        }
    }

    // 3. No se encontró nada
    Serial.println(F("LCD1602_I2C: ERROR - No I2C LCD module found"));
    return false;
}
```

**Algoritmo**:
1. **Prioridad 1**: Prueba `0x27` (PCF8574T).
2. **Prioridad 2**: Prueba `0x3F` (PCF8574AT).
3. **Escaneo completo**: Itera por todas las direcciones I2C válidas (8–119).
4. Si no encuentra nada, retorna `false` con mensaje en Serial.

---

#### `uint8_t getAddress()`

Retorna `_addr` (dirección I2C activa).

---

#### `uint8_t getCols()`

Retorna `_cols` (número de columnas del display).

---

#### `uint8_t getRows()`

Retorna `_rows` (número de filas del display).

---

#### `void setCursor(uint8_t col, uint8_t row)`

```cpp
void LCD1602_I2C::setCursor(uint8_t col, uint8_t row) {
    if (col >= _cols) col = _cols - 1;
    if (row >= _rows) row = _rows - 1;

    uint8_t ddramAddr = (row == 0) ? col : (0x40 + col);
    _sendCommand(0x80 | ddramAddr);
}
```

**Detalles**:
- El HD44780 tiene un mapa de DDRAM donde la fila 0 (row=0) inicia en **0x00** y la fila 1 (row=1) en **0x40**.
- Para un LCD 16×2, las direcciones válidas son:
  - Fila 0: 0x00–0x0F (columnas 0–15)
  - Fila 1: 0x40–0x4F (columnas 0–15)
- El comando `0x80 | ddramAddr` fija la dirección DDRAM para la siguiente escritura.
- Si col ≥ _cols, se trunca a `_cols - 1`.
- Si row ≥ _rows, se trunca a `_rows - 1`.

---

#### `void home()`

Envía el comando `0x02` (Return Home):
- Lleva el cursor a (0, 0).
- Restaura la posición de desplazamiento de la pantalla.
- **No** borra la DDRAM (a diferencia de `clear()`).
- Requiere ~1.64 ms de espera (se usa `delay(2)`).

---

#### `void clear()`

Envía el comando `0x01` (Clear Display):
- Borra todo el contenido de DDRAM (llena con espacios `0x20`).
- Lleva el cursor a (0, 0).
- Requiere ~1.64 ms de espera (se usa `delay(2)`).

---

#### `void print(const char* str)`

```cpp
void LCD1602_I2C::print(const char* str) {
    while (*str) {
        _sendData(*str);
        str++;
    }
}
```

Itera por cada carácter de la cadena (terminada en null) y lo envía al LCD mediante `_sendData()`.

---

#### `void print(int value)`

Convierte un entero a cadena con `itoa()` (base 10) y llama a `print(const char*)`.

---

#### `void print(float value, uint8_t decimals)`

```cpp
void LCD1602_I2C::print(float value, uint8_t decimals) {
    if (value < 0.0f) {
        _sendData('-');
        value = -value;
    }

    int intPart = (int)value;
    print(intPart);

    if (decimals > 0) {
        _sendData('.');
        float fracPart = value - (float)intPart;
        for (uint8_t i = 0; i < decimals; i++) {
            fracPart *= 10.0f;
            int digit = (int)fracPart;
            _sendData('0' + digit);
            fracPart -= (float)digit;
        }
    }
}
```

**Detalles**:
- Maneja números negativos (antepone '-').
- Parte entera: convierte a entero y usa `print(int)`.
- Parte decimal: solo se imprime si `decimals > 0` (corrección I-001).
- Extrae dígito por dígito multiplicando por 10 y truncando.

---

#### `void printCenter(const char* str, uint8_t row)`

```cpp
void LCD1602_I2C::printCenter(const char* str, uint8_t row) {
    uint8_t len = strlen(str);
    uint8_t col = (len >= _cols) ? 0 : (_cols - len) / 2;
    setCursor(col, row);
    print(str);
}
```

Calcula el desplazamiento necesario para centrar el texto:
- Si el texto es más ancho que el display, empieza en columna 0.
- Si no, col = (16 - len) / 2.

---

#### `void backlightOn()` / `void backlightOff()` / `void setBacklight(bool on)` / `bool isBacklightOn()`

```cpp
void LCD1602_I2C::backlightOn() {
    _backlight = true;
    _writeExpander(_backlight ? 0x08 : 0x00);
}

void LCD1602_I2C::backlightOff() {
    _backlight = false;
    _writeExpander(0x00);
}
```

**Detalles**:
- `backlightOn()` y `backlightOff()` escriben directamente en el PCF8574 el bit 3 (BL).
- `setBacklight(bool)` es un wrapper que delega en los dos anteriores.
- `isBacklightOn()` retorna el flag `_backlight`.

---

#### `void createChar(uint8_t index, const uint8_t* data)`

```cpp
void LCD1602_I2C::createChar(uint8_t index, const uint8_t* data) {
    if (index > 7) return;

    _sendCommand(0x40 | (index << 3));

    for (uint8_t i = 0; i < 8; i++) {
        _sendData(data[i]);
    }
}
```

**Detalles**:
- CGRAM base: 0x40. Cada carácter usa 8 bytes. Índice N → dirección 0x40 + N×8.
- `index` debe ser 0–7 (máximo 8 caracteres personalizables).
- `data` debe contener 8 bytes, cada byte representa una fila usando solo 5 bits (bit 4 = izquierda, bit 0 = derecha).
- Después de crear caracteres, la dirección DDRAM queda en una posición indefinida. El usuario debe llamar `setCursor()` o `home()` antes de imprimir.

---

#### `void displayChar(uint8_t index, uint8_t col, uint8_t row)`

```cpp
void LCD1602_I2C::displayChar(uint8_t index, uint8_t col, uint8_t row) {
    if (index > 7) return;
    setCursor(col, row);
    _sendData(index);
}
```

**Detalles**:
- Los caracteres CGRAM se muestran enviando su índice (0–7) como si fuera un carácter normal.
- `setCursor()` posiciona, `_sendData(index)` envía el índice del carácter personalizado.

---

#### `void scrollLeft(uint8_t cols, uint16_t delayMs)`

Envía el comando `0x18` (Display Shift Left) `cols` veces con `delayMs` de espera entre cada desplazamiento.

---

#### `void scrollRight(uint8_t cols, uint16_t delayMs)`

Envía el comando `0x1C` (Display Shift Right) `cols` veces con `delayMs` de espera entre cada desplazamiento.

---

#### `void scrollText(const char* text, uint16_t delayMs)`

```cpp
void LCD1602_I2C::scrollText(const char* text, uint16_t delayMs) {
    clear();

    size_t rawLen = strlen(text);
    if (rawLen > 40) rawLen = 40;
    uint8_t len = (uint8_t)rawLen;

    // Texto corto: centrado
    if (len <= _cols) {
        printCenter(text, 0);
        return;
    }

    // Texto largo: escribir y desplazar
    setCursor(0, 0);
    for (uint8_t i = 0; i < len; i++) {
        _sendData(text[i]);
    }

    int maxShift = len - _cols;
    for (int i = 0; i < maxShift; i++) {
        delay(delayMs);
        _sendCommand(0x18);
    }
    delay(delayMs);
}
```

**Detalles**:
- Si el texto es **corto** (≤ _cols caracteres): lo muestra centrado.
- Si el texto es **largo**: escribe los primeros `len` caracteres (máx. 40 por límite DDRAM) y luego desplaza la pantalla `len - _cols` veces.
- Desplazamiento solo a la izquierda (de derecha a izquierda, como un ticker).

---

#### `void blinkCursor(bool enable)` / `void showCursor(bool enable)`

```cpp
void LCD1602_I2C::blinkCursor(bool enable) {
    _cursorBlink = enable;
    _updateDisplayMode();
}

void LCD1602_I2C::showCursor(bool enable) {
    _cursorVisible = enable;
    _updateDisplayMode();
}
```

Ambos modifican su respectivo flag y llaman a `_updateDisplayMode()`.

---

#### `void displayProgressBar(uint8_t percent, uint8_t col, uint8_t row, uint8_t length)`

```cpp
void LCD1602_I2C::displayProgressBar(uint8_t percent, uint8_t col,
                                     uint8_t row, uint8_t length) {
    if (percent > 100) percent = 100;

    uint8_t maxLen = _cols - col;
    if (length > maxLen) length = maxLen;
    if (length == 0) return;

    _createProgressChars();  // Crea caracteres en índices 0-5

    uint16_t totalSubSegments = (uint16_t)length * 5;
    uint16_t filledSubSegments = ((uint16_t)percent * totalSubSegments) / 100;

    uint8_t fullChars = filledSubSegments / 5;
    uint8_t partial   = filledSubSegments % 5;

    if (percent == 100) {
        fullChars = length;
        partial = 0;
    }

    for (uint8_t i = 0; i < length; i++) {
        setCursor(col + i, row);
        if (i < fullChars) {
            _sendData(5);       // █ lleno
        } else if (i == fullChars) {
            _sendData(partial); // ▏▎▍▌ parcial (0-4)
        } else {
            _sendData(0);       // ▯ vacío
        }
    }
}
```

**Detalles**:
- Cada carácter del progreso se divide en 5 sub-segmentos (5 columnas de píxeles).
- Total de sub-segmentos = length × 5.
- Se calculan cuántos sub-segmentos están llenos según el porcentaje.
- `fullChars`: caracteres completamente llenos (índice 5).
- `partial`: carácter parcialmente lleno (índices 0-4).
- El resto son caracteres vacíos (índice 0).
- **Importante**: Sobrescribe los caracteres personalizados en índices CGRAM 0–5.

---

### Métodos privados

#### `bool _writeExpander(uint8_t data)`

Escribe un byte en el PCF8574. Retorna `true` si la transmisión fue exitosa. En caso de error I2C, imprime el código de error y retorna `false`.

**Códigos de error de `endTransmission()`**:

| Código | Significado |
|:------:|-------------|
| 0 | Éxito |
| 1 | Buffer overflow (datos demasiado largos) |
| 2 | NACK recibido en dirección (dispositivo no responde) |
| 3 | NACK recibido en datos |
| 4 | Error desconocido |
| 5 | Timeout (en algunas placas) |

---

#### `void _writeNibble(uint8_t nibble, uint8_t rs)`

Envía 4 bits (nibble) al LCD a través del PCF8574, generando un pulso en E. Ver sección [Protocolo HD44780 modo 4 bits](#protocolo-hd44780-modo-4-bits).

---

#### `void _writeByte(uint8_t data, uint8_t rs)`

Envía un byte completo al LCD: primero el nibble superior, luego el inferior.

---

#### `void _sendCommand(uint8_t cmd)`

```cpp
void LCD1602_I2C::_sendCommand(uint8_t cmd) {
    _writeByte(cmd, 0);
    delayMicroseconds(50);

    if (cmd == 0x01 || cmd == 0x02) {
        delay(2);
    }
}
```

Envía un comando con RS=0. Los comandos Clear (0x01) y Home (0x02) necesitan >1.64 ms de espera.

---

#### `void _sendData(uint8_t data)`

```cpp
void LCD1602_I2C::_sendData(uint8_t data) {
    _writeByte(data, 1);
    delayMicroseconds(50);
}
```

Envía un dato (carácter) con RS=1.

---

#### `bool _detectAddress(uint8_t addr)`

```cpp
bool LCD1602_I2C::_detectAddress(uint8_t addr) {
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}
```

Verifica si un dispositivo responde en una dirección I2C específica. Retorna `true` si `endTransmission()` devuelve 0 (ACK recibido).

---

#### `void _updateDisplayMode()`

```cpp
void LCD1602_I2C::_updateDisplayMode() {
    uint8_t cmd = 0x08;  // Display OFF base
    cmd |= 0x04;          // Display ON (bit 2)
    if (_cursorVisible) cmd |= 0x02;
    if (_cursorBlink)   cmd |= 0x01;
    _sendCommand(cmd);
}
```

Construye el comando Display On/Off combinando los flags `_cursorVisible` y `_cursorBlink`:

| Bit | Flag | Valor si activo |
|:---:|------|:---------------:|
| 2 | Display ON | Siempre 1 (0x04) |
| 1 | Cursor visible | 0x02 |
| 0 | Cursor blink | 0x01 |

---

#### `bool _initLCD()`

Implementa la secuencia de inicialización del HD44780 en modo 4 bits según el datasheet:

| Orden | Acción | Código | Propósito |
|:-----:|--------|:------:|-----------|
| 1 | Esperar 50 ms | — | Estabilización power-on |
| 2 | Escribir 0x30 × 3 | 0x30, 0x30, 0x30 | Configurar en 8 bits (intentos) |
| 3 | Escribir 0x20 | 0x20 | Cambiar a modo 4 bits |
| 4 | Función Set | 0x28 | 4-bit, 2 líneas, 5×8 |
| 5 | Display OFF | 0x08 | Apagar durante configuración |
| 6 | Clear | 0x01 | Limpiar DDRAM |
| 7 | Entry Mode | 0x06 | Incrementar, sin shift |
| 8 | Display ON | 0x0C | Encender, cursor OFF, blink OFF |

Los primeros 3 pasos (0x30 × 3 y 0x20) se hacen a nivel de **nibble manual** (sin usar `_sendCommand`) porque el LCD aún no está en modo 4 bits.

---

#### `void _createProgressChars()`

Crea los 6 caracteres personalizados para la barra de progreso en los índices CGRAM 0–5:

```cpp
static const uint8_t PROGMEM charData[6][8] = {
    { 0b00000, 0b00000, 0b00000, 0b00000,   // 0: vacío (0/5)
      0b00000, 0b00000, 0b00000, 0b00000 },
    { 0b10000, 0b10000, 0b10000, 0b10000,   // 1: 1/5
      0b10000, 0b10000, 0b10000, 0b10000 },
    { 0b11000, 0b11000, 0b11000, 0b11000,   // 2: 2/5
      0b11000, 0b11000, 0b11000, 0b11000 },
    { 0b11100, 0b11100, 0b11100, 0b11100,   // 3: 3/5
      0b11100, 0b11100, 0b11100, 0b11100 },
    { 0b11110, 0b11110, 0b11110, 0b11110,   // 4: 4/5
      0b11110, 0b11110, 0b11110, 0b11110 },
    { 0b11111, 0b11111, 0b11111, 0b11111,   // 5: lleno (5/5)
      0b11111, 0b11111, 0b11111, 0b11111 }
};
```

Visualmente:
```
Índice 0: ▯        (00000)
Índice 1: ▏        (10000)
Índice 2: ▎        (11000)
Índice 3: ▍        (11100)
Índice 4: ▌        (11110)
Índice 5: █        (11111)
```

---

##   Detección automática de dirección I2C

### Algoritmo

```
beginAuto()
   │
   ├─ 1. Wire.begin()
   │
   ├─ 2. _detectAddress(0x27)
   │      ├─ ACK → _addr = 0x27, _initLCD(), return true
   │      └─ NACK → continuar
   │
   ├─ 3. _detectAddress(0x3F)
   │      ├─ ACK → _addr = 0x3F, _initLCD(), return true
   │      └─ NACK → continuar
   │
   ├─ 4. loop addr = 8 to 119:
   │      ├─ _detectAddress(addr)
   │      │    ├─ ACK → _addr = addr, _initLCD(), return true
   │      │    └─ NACK → continuar
   │      └─ addr++
   │
   └─ 5. Serial.println("ERROR: No I2C LCD module found")
        return false
```

### Criterios de diseño

1. **Prioridad a direcciones comunes** (0x27, 0x3F): aceleran el 99% de los casos.
2. **Escaneo completo** como fallback: cubre direcciones no estándar.
3. **Rango válido**: 8–119 (0x08–0x77). Las direcciones 0–7 y 120–127 son reservadas o no disponibles para dispositivos en el bus I2C.
4. **`_detectAddress()`** usa `Wire.endTransmission()` que retorna 0 si el dispositivo envió ACK. Esto no modifica ningún registro del dispositivo, por lo que es seguro de usar.

### Verificación `begin(uint8_t addr)`

Se agregó validación de rango (corrección I-004):
```cpp
if (addr < 8 || addr > 119) {
    // Error: dirección inválida
    return false;
}
```

---

##   Secuencia de inicialización

### Diagrama completo

```
Power ON / Reset
      │
      ▼
delay(50 ms)    ─── Esperar >15 ms que requiere el HD44780
      │
      ▼
_writeExpander(0x30 | BL)    ─── E=0, RS=0, nibble=0011
_writeExpander(0x34 | BL)    ─── E=1 (pulso, 1 µs)
_writeExpander(0x30 | BL)    ─── E=0
delay(5 ms)                  ─── Espera >4.1 ms
      │
      ▼
_writeExpander(0x30 | BL)    ─── Segundo intento 0x30
_writeExpander(0x34 | BL)
_writeExpander(0x30 | BL)
delay(150 µs)                ─── Espera >100 µs
      │
      ▼
_writeExpander(0x30 | BL)    ─── Tercer intento 0x30
_writeExpander(0x34 | BL)
_writeExpander(0x30 | BL)
delay(150 µs)
      │
      ▼
_writeExpander(0x20 | BL)    ─── Enviar 0x20: cambio a modo 4 bits
_writeExpander(0x24 | BL)
_writeExpander(0x20 | BL)
delay(150 µs)
      │
      ▼
_sendCommand(0x28)   ─── Function Set: 4-bit, 2 líneas, 5×8
_sendCommand(0x08)   ─── Display OFF
_sendCommand(0x01)   ─── Clear (delay 2 ms)
_sendCommand(0x06)   ─── Entry Mode: incrementar, sin shift
_sendCommand(0x0C)   ─── Display ON, cursor OFF, blink OFF
      │
      ▼
   ¡LCD listo!
```

---

##   Manejo de errores

### Estrategia general

| Condición de error | Comportamiento |
|-------------------|----------------|
| Dirección I2C inválida en `begin(addr)` | Retorna `false`, imprime error en Serial |
| Sin dispositivo en la dirección | Retorna `false`, imprime error en Serial |
| Error de transmisión I2C en `_writeExpander()` | Retorna `false`, imprime código de error |
| Dirección I2C no detectada en `beginAuto()` | Retorna `false`, imprime error |
| Índice de carácter personalizado > 7 | Ignora (no hace nada) |
| Porcentaje > 100 en barra de progreso | Trunca a 100 |
| col ≥ _cols en setCursor | Trunca a _cols - 1 |
| row ≥ _rows en setCursor | Trunca a _rows - 1 |
| Texto > 40 caracteres en scrollText | Trunca a 40 |
| long itud de barra excede display | Trunca al máximo disponible |

### Códigos de error I2C

Cuando `_writeExpander()` detecta un error, imprime en Serial:

```
LCD1602_I2C: I2C error <code> at 0x<address>
```

Códigos:
- **1**: Buffer overflow. Los datos enviados superan la capacidad del buffer I2C de Arduino (32 bytes). En esta librería solo se envía 1 byte, así que este error no debería ocurrir.
- **2**: NACK en dirección. El dispositivo en la dirección no respondió. Causas: dirección incorrecta, dispositivo apagado, bus I2C bloqueado.
- **3**: NACK en datos. El dispositivo recibió la dirección pero rechazó los datos. Causa rara en PCF8574.
- **4**: Error desconocido.
- **5**: Timeout (en algunas placas como Due, Zero).

---

##   Cómo extender la librería

### 1. Agregar un nuevo tipo de display (LCD 20×4)

Si bien la librería está diseñada para LCD 1602, puede soportar otros tamaños con cambios mínimos:

```cpp
// En el sketch de usuario:
LCD1602_I2C lcd(0x27, 20, 4);  // 20 columnas × 4 filas
```

**Limitación actual**: `setCursor()` mapea filas 0→0x00 y 1→0x40. Para 4 filas (20×4) el mapa DDRAM es:
- Fila 0: 0x00–0x13
- Fila 1: 0x40–0x53
- Fila 2: 0x14–0x27
- Fila 3: 0x54–0x67

**Para extender**: Modificar `setCursor()` con un arreglo de offsets:

```cpp
static const uint8_t rowOffsets[] = { 0x00, 0x40, 0x14, 0x54 };
if (row < sizeof(rowOffsets)) {
    _sendCommand(0x80 | (rowOffsets[row] + col));
}
```

### 2. Agregar soporte para modo 8 bits

Aunque el propósito del I2C es minimizar cables, podrías extender la librería para modo 8 bits directo:

```cpp
class LCD1602_8bit : public LCD1602_I2C {
    // Override _writeByte() y _writeNibble()
};
```

### 3. Agregar nuevas funcionalidades

#### Efecto de desvanecimiento (fade)

```cpp
void fadeOut(uint16_t stepDelay = 50) {
    for (uint8_t i = 0; i < 8; i++) {
        // Alternar backlight rápidamente para efecto parpadeo
        setBacklight(i % 2 == 0);
        delay(stepDelay);
    }
    backlightOff();
}
```

#### Animación de desplazamiento vertical

```cpp
void scrollUp() {
    // Leer DDRAM y desplazar contenido hacia arriba
    // (más complejo: requiere modo lectura del HD44780)
}
```

#### Reloj en tiempo real

```cpp
void displayClock(uint8_t hours, uint8_t mins, uint8_t secs,
                  uint8_t col = 4, uint8_t row = 0) {
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", hours, mins, secs);
    setCursor(col, row);
    print(buffer);
}
```

### 4. Agregar soporte para sensor de contraste automático

```cpp
// Podría usar un potenciómetro digital (ej. MCP4018) para ajustar
// el contraste por software mediante un pin PWM.
void setContrast(uint8_t level) {
    // level: 0-255 → PWM al pin de contraste
    analogWrite(CONTRAST_PIN, level);
}
```

### 5. Personalizar la tabla de caracteres de la barra de progreso

```cpp
// En LCD1602_I2C.h: agregar método virtual
virtual void _createProgressChars();  // Ahora es sobreescribible

// En subclase:
void MyLCD::_createProgressChars() {
    // Definir caracteres con estilo diferente
    // Por ejemplo, barra vertical en lugar de horizontal
}
```

### 6. Soporte para múltiples LCD en el mismo bus I2C

```cpp
LCD1602_I2C lcd1(0x27, 16, 2);
LCD1602_I2C lcd2(0x3F, 16, 2);

void setup() {
    lcd1.begin();
    lcd2.begin();

    lcd1.print("LCD 1");
    lcd2.print("LCD 2");
}
```

### 7. Buffer de pantalla (framebuffer)

Para operaciones más complejas (animaciones, sprites), podrías implementar un framebuffer en RAM:

```cpp
class LCD1602_I2C_Framebuffer : public LCD1602_I2C {
private:
    uint8_t _buffer[2][16];  // Buffer de 32 bytes

public:
    void setPixel(uint8_t col, uint8_t row, uint8_t charIndex);
    void render();         // Copia buffer al LCD
    void scrollBuffer();   // Desplaza buffer internamente
};
```

---

##   Pruebas y verificación

### Casos de prueba automatizados

| # | Caso | Entrada | Salida esperada | Verificación |
|:-:|------|---------|-----------------|:------------:|
| 1 | HelloWorld básico | `init + backlight + print` | "Hola Mundo" en fila 0 | Visual |
| 2 | Dirección auto 0x27 | `beginAuto()` con módulo 0x27 | true, addr=0x27 | Serial |
| 3 | Dirección auto 0x3F | `beginAuto()` con módulo 0x3F | true, addr=0x3F | Serial |
| 4 | Carácter custom | `createChar(0, data)` + `displayChar(0,0,0)` | Símbolo en (0,0) | Visual |
| 5 | Backlight OFF | `backlightOff()` | Pantalla oscura | Visual |
| 6 | Backlight ON | `backlightOn()` | Retroiluminación encendida | Visual |
| 7 | Scroll texto largo | `scrollText("texto largo...", 300)` | Texto se desplaza | Visual |
| 8 | Texto centrado | `printCenter("Centrado", 0)` | Texto centrado | Visual |
| 9 | Barra progreso | `displayProgressBar(50, 0, 1, 16)` | Barra al 50% | Visual |
| 10 | Clear | `clear()` | Pantalla vacía | Visual |
| 11 | Números | `print(123)` | "123" | Visual |
| 12 | Flotantes | `print(3.14, 2)` | "3.14" | Visual |
| 13 | I2C Scanner | `scan()` | Lista de direcciones | Serial |
| 14 | DHT simulado | loop cada 2s | Temp y humedad actualizadas | Visual + Serial |
| 15 | Error: sin LCD | `begin()` sin módulo | false, mensaje Serial | Serial |
| 16 | Dirección inválida | `begin(1)` con addr < 8 | false, mensaje error | Serial |
| 17 | Cursor blink | `blinkCursor(true)` | Cursor parpadea | Visual |
| 18 | Cursor visible | `showCursor(true)` | Cursor subrayado visible | Visual |
| 19 | print(float, 0) | `print(3.14, 0)` | "3" (sin punto decimal) | Visual |
| 20 | scrollText truncado | texto de 50 caracteres | Truncado a 40, desplazado | Visual |

### Verificación de correcciones post-auditoría

| ID | Descripción | Archivo | Estado |
|:--:|-------------|---------|:------:|
| I-001 | `print(float, 0)` ya no imprime punto decimal sin dígitos | `LCD1602_I2C.cpp:169` | ✅ Corregido |
| I-002 | `_writeExpander()` retorna `bool` y captura errores I2C | `LCD1602_I2C.cpp:365` | ✅ Corregido |
| I-003 | `blinkCursor`/`showCursor` preservan estado mutuo con `_updateDisplayMode()` | `LCD1602_I2C.cpp:440` | ✅ Corregido |
| I-004 | `begin(addr)` valida rango I2C (8–119) | `LCD1602_I2C.cpp:53` | ✅ Corregido |
| I-005 | `scrollText()` trunca a 40 caracteres y envía con bucle | `LCD1602_I2C.cpp:258` | ✅ Corregido |

---

##   Historial de versiones

| Versión | Fecha | Cambios |
|:-------:|:-----:|---------|
| 1.0.0 | 2025-03 | Lanzamiento inicial. Implementación completa de todas las funcionalidades. |

---

<div align="center">

**LCD1602_I2C** — Documentación para desarrolladores v1.0.0

[← Volver al README principal](README.md) · [Ver datasheet →](DATASHEET.md)

</div>
