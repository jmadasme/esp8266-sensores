/**
 * LCD1602_I2C.cpp - Implementación del controlador para LCD1602
 *                   con interfaz I2C (PCF8574)
 *
 * Esta implementación sigue el protocolo HD44780 en modo 4 bits
 * a través del expansor I2C PCF8574.
 *
 * Mapa de pines PCF8574 → HD44780:
 *   P0 → RS     (Register Select)
 *   P1 → RW     (Read/Write, normalmente a GND para solo escritura)
 *   P2 → E      (Enable)
 *   P3 → BL     (Backlight)
 *   P4 → D4     (Dato bit 4)
 *   P5 → D5     (Dato bit 5)
 *   P6 → D6     (Dato bit 6)
 *   P7 → D7     (Dato bit 7)
 */

#include "LCD1602_I2C.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────

LCD1602_I2C::LCD1602_I2C(uint8_t addr, uint8_t cols, uint8_t rows)
    : _addr(addr)
    , _cols(cols)
    , _rows(rows)
    , _backlight(true)
    , _initialized(false)
    , _cursorVisible(false)
    , _cursorBlink(false)
{
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inicialización pública
// ─────────────────────────────────────────────────────────────────────────────

bool LCD1602_I2C::begin()
{
    Wire.begin();
    if (!_detectAddress(_addr)) {
        Serial.print(F("LCD1602_I2C: No device found at 0x"));
        Serial.println(_addr, HEX);
        return false;
    }
    return _initLCD();
}

bool LCD1602_I2C::begin(uint8_t addr)
{
    if (addr < 8 || addr > 119) {
        Serial.print(F("LCD1602_I2C: Invalid I2C address 0x"));
        Serial.println(addr, HEX);
        return false;
    }
    _addr = addr;
    return begin();
}

bool LCD1602_I2C::beginAuto()
{
    Wire.begin();

    // 1. Probar direcciones comunes primero
    const uint8_t commonAddrs[] = {0x27, 0x3F};
    for (uint8_t i = 0; i < 2; i++) {
        if (_detectAddress(commonAddrs[i])) {
            _addr = commonAddrs[i];
            Serial.print(F("LCD1602_I2C: Module found at 0x"));
            Serial.println(_addr, HEX);
            return _initLCD();
        }
    }

    // 2. Escaneo completo del bus I2C (8-119)
    Serial.println(F("LCD1602_I2C: Scanning full I2C bus..."));
    for (uint8_t addr = 8; addr < 120; addr++) {
        if (_detectAddress(addr)) {
            _addr = addr;
            Serial.print(F("LCD1602_I2C: Module found at 0x"));
            Serial.println(_addr, HEX);
            return _initLCD();
        }
    }

    Serial.println(F("LCD1602_I2C: ERROR - No I2C LCD module found"));
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Información del dispositivo
// ─────────────────────────────────────────────────────────────────────────────

uint8_t LCD1602_I2C::getAddress()
{
    return _addr;
}

uint8_t LCD1602_I2C::getCols()
{
    return _cols;
}

uint8_t LCD1602_I2C::getRows()
{
    return _rows;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Control de cursor y pantalla
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::setCursor(uint8_t col, uint8_t row)
{
    if (col >= _cols) col = _cols - 1;
    if (row >= _rows) row = _rows - 1;

    // Dirección DDRAM: fila 0 inicia en 0x00, fila 1 en 0x40
    uint8_t ddramAddr = (row == 0) ? col : (0x40 + col);
    _sendCommand(0x80 | ddramAddr);
}

void LCD1602_I2C::home()
{
    _sendCommand(0x02);  // Return home
    delay(2);
}

void LCD1602_I2C::clear()
{
    _sendCommand(0x01);  // Clear display
    delay(2);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Salida de texto
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::print(const char* str)
{
    while (*str) {
        _sendData(*str);
        str++;
    }
}

void LCD1602_I2C::print(int value)
{
    char buffer[8];
    itoa(value, buffer, 10);
    print(buffer);
}

void LCD1602_I2C::print(float value, uint8_t decimals)
{
    // Manejar valores negativos
    if (value < 0.0f) {
        _sendData('-');
        value = -value;
    }

    // Parte entera
    int intPart = (int)value;
    print(intPart);

    // Parte decimal (solo si se solicitaron decimales)
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

// ─────────────────────────────────────────────────────────────────────────────
//  Retroiluminación
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::backlightOn()
{
    _backlight = true;
    // Re-envía el último estado para que el bit BL se actualice
    _writeExpander(_backlight ? 0x08 : 0x00);
}

void LCD1602_I2C::backlightOff()
{
    _backlight = false;
    _writeExpander(0x00);
}

void LCD1602_I2C::setBacklight(bool on)
{
    if (on) {
        backlightOn();
    } else {
        backlightOff();
    }
}

bool LCD1602_I2C::isBacklightOn()
{
    return _backlight;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Caracteres personalizados
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::createChar(uint8_t index, const uint8_t* data)
{
    if (index > 7) return;

    // CGRAM base: 0x40, cada carácter ocupa 8 bytes
    _sendCommand(0x40 | (index << 3));

    for (uint8_t i = 0; i < 8; i++) {
        _sendData(data[i]);
    }

    // Después de escribir en CGRAM, la dirección de DDRAM queda apuntando
    // a una posición indefinida. El usuario debe llamar a setCursor().
}

void LCD1602_I2C::displayChar(uint8_t index, uint8_t col, uint8_t row)
{
    if (index > 7) return;
    setCursor(col, row);
    _sendData(index);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Desplazamiento (scroll)
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::scrollLeft(uint8_t cols, uint16_t delayMs)
{
    for (uint8_t i = 0; i < cols; i++) {
        _sendCommand(0x18);  // Desplazar display a la izquierda
        delay(delayMs);
    }
}

void LCD1602_I2C::scrollRight(uint8_t cols, uint16_t delayMs)
{
    for (uint8_t i = 0; i < cols; i++) {
        _sendCommand(0x1C);  // Desplazar display a la derecha
        delay(delayMs);
    }
}

void LCD1602_I2C::scrollText(const char* text, uint16_t delayMs)
{
    clear();

    size_t rawLen = strlen(text);
    if (rawLen > 40) rawLen = 40;  // Límite DDRAM del HD44780
    uint8_t len = (uint8_t)rawLen;

    // Texto corto: mostrar centrado
    if (len <= _cols) {
        printCenter(text, 0);
        return;
    }

    // Texto largo: escribir solo los primeros 'len' caracteres (truncado a 40)
    setCursor(0, 0);
    for (uint8_t i = 0; i < len; i++) {
        _sendData(text[i]);
    }

    // Desplazar hacia la izquierda para revelar las partes ocultas
    int maxShift = len - _cols;
    for (int i = 0; i < maxShift; i++) {
        delay(delayMs);
        _sendCommand(0x18);  // Shift display left
    }
    delay(delayMs);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Apariencia del cursor
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::blinkCursor(bool enable)
{
    _cursorBlink = enable;
    _updateDisplayMode();
}

void LCD1602_I2C::showCursor(bool enable)
{
    _cursorVisible = enable;
    _updateDisplayMode();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Barra de progreso
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::displayProgressBar(uint8_t percent, uint8_t col,
                                     uint8_t row, uint8_t length)
{
    if (percent > 100) percent = 100;

    // Ajustar longitud si excede el display
    uint8_t maxLen = _cols - col;
    if (length > maxLen) length = maxLen;
    if (length == 0) return;

    // (Re)crear los caracteres personalizados de la barra
    _createProgressChars();

    // Calcular la cantidad de sub-segmentos (5 por cada carácter)
    uint16_t totalSubSegments = (uint16_t)length * 5;
    uint16_t filledSubSegments = ((uint16_t)percent * totalSubSegments) / 100;

    uint8_t fullChars  = filledSubSegments / 5;
    uint8_t partial    = filledSubSegments % 5;

    // Si está al 100% aseguramos que parcial sea 0 y fullChars = length
    if (percent == 100) {
        fullChars = length;
        partial = 0;
    }

    for (uint8_t i = 0; i < length; i++) {
        setCursor(col + i, row);
        if (i < fullChars) {
            _sendData(5);         // Carácter lleno
        } else if (i == fullChars) {
            _sendData(partial);   // Carácter parcial (0-4)
        } else {
            _sendData(0);         // Carácter vacío
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Texto centrado
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::printCenter(const char* str, uint8_t row)
{
    uint8_t len = strlen(str);
    uint8_t col = (len >= _cols) ? 0 : (_cols - len) / 2;
    setCursor(col, row);
    print(str);
}

// ═════════════════════════════════════════════════════════════════════════════
//  MÉTODOS PRIVADOS
// ═════════════════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────────────────
//  Comunicación I2C de bajo nivel
// ─────────────────────────────────────────────────────────────────────────────

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

void LCD1602_I2C::_writeNibble(uint8_t nibble, uint8_t rs)
{
    // Construir el byte para el PCF8574:
    //   bits 7-4 = nibble de datos (D7-D4)
    //   bit 2    = E
    //   bit 1    = RW (siempre 0, solo escritura)
    //   bit 0    = RS
    //   bit 3    = BL (backlight)
    uint8_t data = (nibble << 4) | (rs ? 0x01 : 0x00) | (_backlight ? 0x08 : 0x00);

    // E = 0
    _writeExpander(data & ~0x04);
    delayMicroseconds(1);

    // E = 1 (pulso)
    _writeExpander(data | 0x04);
    delayMicroseconds(1);

    // E = 0
    _writeExpander(data & ~0x04);
    delayMicroseconds(1);
}

void LCD1602_I2C::_writeByte(uint8_t data, uint8_t rs)
{
    _writeNibble(data >> 4, rs);   // Nibble alto primero
    _writeNibble(data & 0x0F, rs); // Nibble bajo después
}

// ─────────────────────────────────────────────────────────────────────────────
//  Comandos HD44780
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::_sendCommand(uint8_t cmd)
{
    _writeByte(cmd, 0);  // RS = 0 (command register)
    delayMicroseconds(50);

    // Los comandos Clear (0x01) y Home (0x02) necesitan más tiempo
    if (cmd == 0x01 || cmd == 0x02) {
        delay(2);
    }
}

void LCD1602_I2C::_sendData(uint8_t data)
{
    _writeByte(data, 1);  // RS = 1 (data register)
    delayMicroseconds(50);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Detección de dirección I2C
// ─────────────────────────────────────────────────────────────────────────────

bool LCD1602_I2C::_detectAddress(uint8_t addr)
{
    Wire.beginTransmission(addr);
    return (Wire.endTransmission() == 0);
}

void LCD1602_I2C::_updateDisplayMode()
{
    // Construye el comando Display On/Off con OR de bits:
    //   bit 2 = display ON (siempre 1)
    //   bit 1 = cursor visible
    //   bit 0 = cursor blink
    uint8_t cmd = 0x08;  // Display OFF base
    cmd |= 0x04;          // Display ON
    if (_cursorVisible) cmd |= 0x02;
    if (_cursorBlink)   cmd |= 0x01;
    _sendCommand(cmd);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Inicialización del LCD (modo 4 bits)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Secuencia según datasheet HD44780:
//    1. Esperar >15 ms tras encender
//    2. Enviar 0x30 tres veces (configuración 8 bits)
//    3. Enviar 0x20 (cambiar a modo 4 bits)
//    4. Enviar comandos en modo 4 bits: 0x28, 0x08, 0x01, 0x06, 0x0C
// ─────────────────────────────────────────────────────────────────────────────

bool LCD1602_I2C::_initLCD()
{
    delay(50);  // Estabilización tras power-on (>15 ms)

    uint8_t bl = _backlight ? 0x08 : 0x00;

    // ── Paso 1: Enviar 0x30 tres veces (modo 8 bits) ──
    // Cuando el LCD arranca está en modo 8 bits. Solo el nibble superior
    // (D7-D4 = 0011) es significativo porque D3-D0 están a 0 (GND).
    for (uint8_t i = 0; i < 3; i++) {
        // Escribir nibble 0x3 con RS=0, BL actual, E=0
        _writeExpander(0x30 | bl);
        delayMicroseconds(1);
        // E = 1
        _writeExpander(0x34 | bl);
        delayMicroseconds(1);
        // E = 0
        _writeExpander(0x30 | bl);

        if (i == 0) {
            delay(5);          // >4.1 ms la primera vez
        } else {
            delayMicroseconds(150);  // >100 us las siguientes
        }
    }

    // ── Paso 2: Cambiar a modo 4 bits (enviar 0x20) ──
    _writeExpander(0x20 | bl);
    delayMicroseconds(1);
    _writeExpander(0x24 | bl);
    delayMicroseconds(1);
    _writeExpander(0x20 | bl);
    delayMicroseconds(150);

    // ── Paso 3: Configurar en modo 4 bits ──
    // Ahora el LCD está en modo 4 bits; usamos _sendCommand() normal.
    _initialized = true;  // Permitir que _sendCommand funcione

    _sendCommand(0x28);  // Function Set: 4-bit, 2 líneas, 5×8 dots
    _sendCommand(0x08);  // Display OFF
    _sendCommand(0x01);  // Clear display
    delay(2);
    _sendCommand(0x06);  // Entry Mode: incrementar, sin shift
    _sendCommand(0x0C);  // Display ON, cursor OFF, blink OFF

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Caracteres personalizados para la barra de progreso
// ─────────────────────────────────────────────────────────────────────────────
//
//  Se definen 6 caracteres (índices 0-5) que representan los niveles
//  de llenado de izquierda a derecha en una cuadrícula de 5×8.
//
//  Bits:  4 3 2 1 0   (4 = columna izquierda, 0 = columna derecha)
//         x x x x x
//
//  Índice 0: ▯ vacío     (00000)
//  Índice 1: ▏ 1/5       (10000)
//  Índice 2: ▎ 2/5       (11000)
//  Índice 3: ▍ 3/5       (11100)
//  Índice 4: ▌ 4/5       (11110)
//  Índice 5: █ lleno     (11111)
// ─────────────────────────────────────────────────────────────────────────────

void LCD1602_I2C::_createProgressChars()
{
    // Datos de los 6 caracteres (cada uno: 8 filas de 5 bits)
    static const uint8_t PROGMEM charData[6][8] = {
        { 0b00000, 0b00000, 0b00000, 0b00000,   // 0: vacío
          0b00000, 0b00000, 0b00000, 0b00000 },
        { 0b10000, 0b10000, 0b10000, 0b10000,   // 1: 1/5
          0b10000, 0b10000, 0b10000, 0b10000 },
        { 0b11000, 0b11000, 0b11000, 0b11000,   // 2: 2/5
          0b11000, 0b11000, 0b11000, 0b11000 },
        { 0b11100, 0b11100, 0b11100, 0b11100,   // 3: 3/5
          0b11100, 0b11100, 0b11100, 0b11100 },
        { 0b11110, 0b11110, 0b11110, 0b11110,   // 4: 4/5
          0b11110, 0b11110, 0b11110, 0b11110 },
        { 0b11111, 0b11111, 0b11111, 0b11111,   // 5: lleno
          0b11111, 0b11111, 0b11111, 0b11111 }
    };

    for (uint8_t i = 0; i < 6; i++) {
        uint8_t buffer[8];
        memcpy_P(buffer, charData[i], 8);
        createChar(i, buffer);
    }
}
