/**
 * ScrollingText.ino - Desplazamiento automático de texto largo
 *
 * Demuestra el scroll horizontal de textos que exceden el ancho del
 * display (16 caracteres) usando scrollText() y también muestra
 * los métodos scrollLeft() / scrollRight() para desplazar el
 * contenido completo de la pantalla.
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

// Texto más largo que el display (16 caracteres)
const char* mensajeLargo =
    "Bienvenido a la libreria LCD1602_I2C para Arduino! "
    "Este texto se desplaza automaticamente... ";

const char* mensajeCorto = "Hola!";

// ─────────────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(9600);
    Serial.println(F("ScrollingText Example"));

    if (!lcd.beginAuto()) {
        Serial.println(F("ERROR: LCD no detectado"));
        while (1) delay(1000);
    }

    lcd.backlightOn();
    lcd.clear();

    lcd.printCenter("Scroll Demo", 0);
    delay(1500);
    lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────

void loop()
{
    // ─── Demostración 1: scrollText con texto largo ────────────────
    lcd.clear();
    lcd.printCenter("Scroll texto:", 0);
    lcd.scrollText(mensajeLargo, 250);

    delay(1000);

    // ─── Demostración 2: scrollText con texto corto (centrado) ────
    lcd.clear();
    lcd.printCenter("Texto corto:", 0);
    lcd.scrollText(mensajeCorto, 300);

    delay(1000);

    // ─── Demostración 3: scrollLeft/scrollRight ────────────────────
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("<<< Desplazando >>>");
    lcd.setCursor(0, 1);
    lcd.print("  linea 2  ");

    delay(1000);

    // Scroll a la izquierda 5 posiciones
    lcd.scrollLeft(5, 300);
    delay(500);

    // Scroll a la derecha 5 posiciones (vuelve al inicio)
    lcd.scrollRight(5, 300);
    delay(1000);

    // ─── Demostración 4: Scroll continuo manual ────────────────────
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ing. Sistemas");
    lcd.setCursor(0, 1);
    lcd.print("UNAH - 2025");

    // Hacer scroll continuo 8 veces
    for (uint8_t i = 0; i < 8; i++) {
        lcd.scrollLeft(1, 200);
    }
    // Volver al inicio
    for (uint8_t i = 0; i < 8; i++) {
        lcd.scrollRight(1, 200);
    }

    delay(2000);
}
