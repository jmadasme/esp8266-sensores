/**
 * I2CScanner.ino - Escáner de bus I2C con salida en LCD y Serial
 *
 * Escanea todo el rango de direcciones I2C (1-127) y muestra los
 * resultados tanto en el monitor Serial como en el LCD (si está
 * conectado). Ideal para diagnosticar la dirección de tu módulo
 * LCD1602 u otros dispositivos I2C.
 *
 * Conexiones:
 *   LCD I2C (opcional) → Arduino Uno
 *   Cualquier dispositivo I2C → SDA/SCL
 *
 * Dependencias:
 *   - Wire.h
 *   - LCD1602_I2C.h (solo si hay LCD conectado)
 */

#include <Wire.h>
#include <LCD1602_I2C.h>

// Crear objeto LCD (se usará solo si se detecta en el bus)
LCD1602_I2C lcd(0x27, 16, 2);

bool lcdAvailable = false;

// ─────────────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(9600);
    Serial.println(F("╔══════════════════════════════════╗"));
    Serial.println(F("║     I2C Scanner v1.0             ║"));
    Serial.println(F("╚══════════════════════════════════╝"));
    Serial.println();

    Wire.begin();

    // Intentar inicializar el LCD (si está presente)
    lcdAvailable = lcd.beginAuto();
    if (lcdAvailable) {
        lcd.backlightOn();
        lcd.clear();
        lcd.printCenter("I2C Scanner", 0);
        lcd.printCenter("Escaneando...", 1);
        delay(1000);
    } else {
        Serial.println(F("LCD no detectado. Solo salida Serial."));
    }
}

// ─────────────────────────────────────────────────────────────────────────────

void scanI2CBus()
{
    uint8_t count = 0;

    Serial.println(F("Escaneando bus I2C..."));
    Serial.println(F("Direcciones detectadas:"));

    if (lcdAvailable) {
        lcd.clear();
        lcd.printCenter("Escaneando...", 0);
    }

    for (uint8_t addr = 1; addr < 128; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            // Dispositivo encontrado
            count++;
            Serial.print(F("  0x"));
            if (addr < 16) Serial.print(F("0"));
            Serial.print(addr, HEX);
            Serial.print(F("  ("));

            // Identificar dispositivos conocidos
            if (addr == 0x27) Serial.print(F("PCF8574T / LCD"));
            else if (addr == 0x3F) Serial.print(F("PCF8574AT / LCD"));
            else if (addr >= 0x68 && addr <= 0x6F) Serial.print(F("AT24Cxx / RTC"));
            else if (addr == 0x76 || addr == 0x77) Serial.print(F("BMP280"));
            else if (addr == 0x5C) Serial.print(F("AM2320"));
            else if (addr == 0x48) Serial.print(F("PCF8591"));
            else Serial.print(F("desconocido"));

            Serial.println(F(")"));
        }
    }

    Serial.println();
    if (count == 0) {
        Serial.println(F("Ningun dispositivo I2C encontrado."));
    } else {
        Serial.print(F("Total: "));
        Serial.print(count);
        Serial.println(F(" dispositivo(s)"));
    }

    // Mostrar en LCD
    if (lcdAvailable) {
        lcd.clear();
        lcd.printCenter("I2C Scan OK", 0);

        if (count == 0) {
            lcd.printCenter("Sin dispositivos", 1);
        } else {
            lcd.setCursor(0, 1);
            lcd.print("Encontrados: ");
            lcd.print(count);
        }
    }

    Serial.println(F("────────────────────────────────────"));
}

// ─────────────────────────────────────────────────────────────────────────────

void loop()
{
    scanI2CBus();

    Serial.println(F("Proximo escaneo en 5 segundos..."));
    Serial.println();
    delay(5000);
}
