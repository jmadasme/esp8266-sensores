/**
 * HelloWorld.ino - Ejemplo básico de uso del LCD1602 con I2C
 *
 * Muestra "Hola Mundo" estático en la primera línea y un contador
 * en la segunda, demostrando inicialización automática y control
 * básico de retroiluminación.
 *
 * Conexiones:
 *   LCD I2C → Arduino Uno
 *   VCC     → 5V
 *   GND     → GND
 *   SDA     → A4
 *   SCL     → A5
 *
 * Dependencias:
 *   - Wire.h (incluida en Arduino IDE)
 *   - LCD1602_I2C.h (esta librería)
 */

#include <Wire.h>
#include <LCD1602_I2C.h>

// Crear objeto LCD con dirección por defecto 0x27
// (Si tu módulo usa 0x3F, cambia el valor o usa beginAuto())
LCD1602_I2C lcd(0x27, 16, 2);

// ─────────────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(9600);
    Serial.println(F("HelloWorld Example"));

    // Inicializar con detección automática de dirección
    if (!lcd.beginAuto()) {
        Serial.println(F("ERROR: No se detectó el LCD. Revisa conexiones."));
        while (1) {  // Detener ejecución
            delay(1000);
        }
    }

    // Encender retroiluminación
    lcd.backlightOn();

    // Mensaje de bienvenida
    lcd.clear();
    lcd.printCenter("Hola Mundo!", 0);
    lcd.setCursor(0, 1);
    lcd.print("Init OK");
    delay(2000);

    lcd.clear();
    lcd.printCenter("LCD1602 I2C", 0);
    lcd.printCenter("Library v1.0", 1);
    delay(1500);
    lcd.clear();
}

// ─────────────────────────────────────────────────────────────────────────────

void loop()
{
    // Mostrar un contador en la segunda línea
    static int counter = 0;

    lcd.printCenter("Hola Mundo!", 0);
    lcd.setCursor(0, 1);
    lcd.print("Contador: ");
    lcd.print(counter);
    counter++;

    // Alternar retroiluminación cada 10 ciclos para demostrar control
    if (counter % 10 == 0) {
        lcd.setBacklight(!lcd.isBacklightOn());
    }

    delay(1000);
}
