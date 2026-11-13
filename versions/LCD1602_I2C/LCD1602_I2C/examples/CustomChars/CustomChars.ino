/**
 * CustomChars.ino - Caracteres personalizados y barra de progreso
 *
 * Demuestra la creación de símbolos personalizados (corazón, flecha,
 * carga, notas musicales) y la barra de progreso animada.
 *
 * Conexiones:
 *   LCD I2C → Arduino Uno
 *   VCC     → 5V
 *   GND     → GND
 *   SDA     → A4
 *   SCL     → A5
 *
 * Dependencias:
 *   - Wire.h
 *   - LCD1602_I2C.h
 */

#include <Wire.h>
#include <LCD1602_I2C.h>

LCD1602_I2C lcd(0x27, 16, 2);

// ─── Definición de caracteres personalizados ──────────────────────────────
// Cada carácter: 8 filas × 5 bits (bit 4 = columna izquierda)

// Corazón (índice 6)
const uint8_t heart[8] PROGMEM = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000
};

// Flecha derecha (índice 7)
const uint8_t arrowRight[8] PROGMEM = {
    0b00000,
    0b00100,
    0b00110,
    0b11111,
    0b11111,
    0b00110,
    0b00100,
    0b00000
};

// Carga girando - frame 0 (índice 0) — esquinas superior izquierda
const uint8_t load0[8] PROGMEM = {
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000,
    0b00000
};

// Para los frames de carga usaremos los índices 0-3 como animación
// y los redefiniremos en el loop, o mejor: usar displayChar con
// caracteres predefinidos. Pero como createChar(6, ...) y (7, ...)
// ya están ocupados con corazón y flecha, usamos los índices que
// queramos. La barra de progreso usará 0-5, así que:

//   Índices 6, 7 → corazón y flecha (se mantienen)
//   Índices 0-5  → barra de progreso (se sobrescriben al usarla)

// ─────────────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(9600);
    Serial.println(F("CustomChars Example"));

    if (!lcd.beginAuto()) {
        Serial.println(F("ERROR: LCD no detectado"));
        while (1) delay(1000);
    }

    lcd.backlightOn();
    lcd.clear();

    // Crear caracteres personalizados en índices 6 y 7
    uint8_t heartBuf[8], arrowBuf[8];
    memcpy_P(heartBuf, heart, 8);
    memcpy_P(arrowBuf, arrowRight, 8);
    lcd.createChar(6, heartBuf);
    lcd.createChar(7, arrowBuf);

    // Mostrar título
    lcd.printCenter("CustomChars", 0);
    delay(1000);
    lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────

void loop()
{
    // ─── Parte 1: Mostrar caracteres personalizados estáticos ────────
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Corazon:");
    lcd.displayChar(6, 9, 0);      // Corazón en col 9

    lcd.setCursor(0, 1);
    lcd.print("Flecha:");
    lcd.displayChar(7, 8, 1);      // Flecha en col 8

    delay(2000);

    // ─── Parte 2: Animación simple de carga ─────────────────────────
    lcd.clear();
    lcd.printCenter("Cargando...", 0);
    lcd.setCursor(0, 1);

    // Caracteres de carga: 4 frames en índices 0-3
    const uint8_t spinFrames[4][8] PROGMEM = {
        { 0b00000, 0b00000, 0b00000, 0b00000,
          0b00000, 0b00000, 0b00000, 0b00000 },   // frame 0: vacío
        { 0b10000, 0b10000, 0b10000, 0b10000,
          0b10000, 0b10000, 0b10000, 0b10000 },   // frame 1: columna izq
        { 0b11111, 0b00000, 0b00000, 0b00000,
          0b00000, 0b00000, 0b00000, 0b00000 },   // frame 2: fila superior
        { 0b00000, 0b00000, 0b00000, 0b00000,
          0b00000, 0b00000, 0b00000, 0b11111 }    // frame 3: fila inferior
    };

    for (uint8_t f = 0; f < 4; f++) {
        uint8_t buf[8];
        memcpy_P(buf, spinFrames[f], 8);
        lcd.createChar(0, buf);    // Reusar índice 0
        lcd.displayChar(0, 7, 1);  // Mostrar en (7,1)
        delay(300);
    }

    delay(1500);

    // ─── Parte 3: Barra de progreso animada ─────────────────────────
    // NOTA: displayProgressBar() sobrescribe los índices 0-5
    // Los caracteres 6 (corazón) y 7 (flecha) se conservan.

    lcd.clear();
    lcd.printCenter("Progreso:", 0);

    // Animar barra de 0% a 100%
    for (uint8_t p = 0; p <= 100; p += 5) {
        lcd.displayProgressBar(p, 0, 1, 16);
        delay(100);
    }

    delay(1000);

    // ─── Parte 4: Combinar corazones con barra ──────────────────────
    lcd.clear();
    lcd.printCenter("Demo completa!", 0);

    // Corazón al inicio y final de la fila 1
    lcd.displayChar(6, 0, 1);      // Corazón al inicio
    lcd.displayChar(6, 15, 1);     // Corazón al final

    // Barra de progreso en medio
    lcd.displayProgressBar(75, 2, 1, 12);

    delay(3000);
}
