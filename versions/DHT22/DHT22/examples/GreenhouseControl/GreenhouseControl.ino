/*
 * GreenhouseControl - Control automatizado de invernadero con DHT22.
 *
 * Conexiones:
 *   DHT22 DATA   → Pin digital 2
 *   DHT22 VDD    → 5V
 *   DHT22 GND    → GND
 *   Extractor    → Pin digital 8 (activo HIGH, via relé/transistor)
 *   Nebulizador  → Pin digital 9 (activo HIGH, via relé/transistor)
 *   LED Verde    → Pin digital 10 (indicador de estado normal)
 *   LED Rojo     → Pin digital 11 (indicador de alarma/error)
 *
 * Lógica de control:
 *   - Extractor ON  si humedad > 70%  (histéresis: OFF si humedad < 65%)
 *   - Nebulizador ON si temperatura > 30°C (histéresis: OFF si temp < 28°C)
 *   - LED Verde: estado normal
 *   - LED Rojo: parpadea si hay errores de comunicación
 *
 * (c) 2026 - AG Creador de Contenido
 */

#include <DHT22.h>

// Pines
const uint8_t PIN_DHT22     = 2;
const uint8_t PIN_EXTRACTOR = 8;
const uint8_t PIN_NEBULIZER = 9;
const uint8_t PIN_LED_GREEN = 10;
const uint8_t PIN_LED_RED   = 11;

// Umbrales con histéresis
const float HUM_THRESHOLD_ON  = 70.0f;   // Encender extractor si hum > 70%
const float HUM_THRESHOLD_OFF = 65.0f;   // Apagar extractor si hum < 65%
const float TEMP_THRESHOLD_ON = 30.0f;   // Encender nebulizador si temp > 30°C
const float TEMP_THRESHOLD_OFF = 28.0f;  // Apagar nebulizador si temp < 28°C

// Intervalos
const uint16_t READ_INTERVAL_MS  = 2000;  // Lectura cada 2s
const uint16_t BLINK_INTERVAL_MS = 300;   // Parpadeo LED rojo

// Estados
bool extractorState   = false;
bool nebulizerState   = false;
bool lastErrorState   = false;
uint32_t lastBlink    = 0;
bool ledRedOn         = false;

// Objeto DHT22
DHT22 dht(PIN_DHT22);

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(10);
    }

    // Configurar pines de salida
    pinMode(PIN_EXTRACTOR, OUTPUT);
    pinMode(PIN_NEBULIZER, OUTPUT);
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_RED,   OUTPUT);

    // Estado inicial: todo apagado
    digitalWrite(PIN_EXTRACTOR, LOW);
    digitalWrite(PIN_NEBULIZER, LOW);
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED,   LOW);

    Serial.println(F("=== DHT22 Greenhouse Control ==="));
    Serial.println(F("Inicializando sensor..."));

    if (!dht.begin()) {
        Serial.println(F("ERROR: No se pudo inicializar el sensor DHT22."));
        Serial.println(F("Verifique las conexiones y el pull-up."));
    } else {
        Serial.println(F("Sensor DHT22 listo."));
    }

    Serial.println(F("------------------------------------------------"));
    Serial.println(F("Sistema de control de invernadero iniciado."));
    Serial.println(F("Extractor:  ON si hum > 70%, OFF si hum < 65%"));
    Serial.println(F("Nebulizador: ON si temp > 30C, OFF si temp < 28C"));
    Serial.println(F("------------------------------------------------"));
    Serial.println();

    // Breve delay para estabilización
    delay(1000);
}

void loop() {
    // Leer temperatura y humedad
    float tempC    = dht.readTemperature();
    float humidity = dht.readHumidity();

    bool readingError = isnan(tempC) || isnan(humidity);

    // =====================================================
    //  CONTROL DE ACTUADORES (solo si la lectura es válida)
    // =====================================================
    if (!readingError) {
        // --- Control del extractor (humedad) ---
        if (humidity > HUM_THRESHOLD_ON && !extractorState) {
            extractorState = true;
            digitalWrite(PIN_EXTRACTOR, HIGH);
            Serial.println(F("[ACCION] Extractor ENCENDIDO (humedad alta)"));
        } else if (humidity < HUM_THRESHOLD_OFF && extractorState) {
            extractorState = false;
            digitalWrite(PIN_EXTRACTOR, LOW);
            Serial.println(F("[ACCION] Extractor APAGADO (humedad normal)"));
        }

        // --- Control del nebulizador (temperatura) ---
        if (tempC > TEMP_THRESHOLD_ON && !nebulizerState) {
            nebulizerState = true;
            digitalWrite(PIN_NEBULIZER, HIGH);
            Serial.println(F("[ACCION] Nebulizador ENCENDIDO (temperatura alta)"));
        } else if (tempC < TEMP_THRESHOLD_OFF && nebulizerState) {
            nebulizerState = false;
            digitalWrite(PIN_NEBULIZER, LOW);
            Serial.println(F("[ACCION] Nebulizador APAGADO (temperatura normal)"));
        }
    }

    // =====================================================
    //  INDICADORES LED
    // =====================================================
    if (readingError) {
        // Error: LED rojo parpadea, LED verde apagado
        digitalWrite(PIN_LED_GREEN, LOW);

        uint32_t now = millis();
        if (now - lastBlink >= BLINK_INTERVAL_MS) {
            lastBlink = now;
            ledRedOn = !ledRedOn;
            digitalWrite(PIN_LED_RED, ledRedOn ? HIGH : LOW);
        }

        if (!lastErrorState) {
            Serial.print(F("[ALERTA] Error de lectura ("));
            Serial.print(dht.getErrorCount());
            Serial.println(F(" errores acumulados)"));
        }
        lastErrorState = true;

    } else {
        // Sin error: LED verde fijo, LED rojo apagado
        digitalWrite(PIN_LED_GREEN, HIGH);
        digitalWrite(PIN_LED_RED, LOW);
        lastErrorState = false;
    }

    // =====================================================
    //  MOSTRAR DATOS POR SERIAL
    // =====================================================
    Serial.print(F("[MONITOR] "));
    if (readingError) {
        Serial.print(F("Datos: N/A"));
    } else {
        Serial.print(F("Temp: "));
        Serial.print(tempC, 1);
        Serial.print(F(" C, Hum: "));
        Serial.print(humidity, 1);
        Serial.print(F(" %, HI: "));
        Serial.print(dht.computeHeatIndex(tempC, humidity), 1);
        Serial.print(F(" C"));
    }

    Serial.print(F(" | Extractor: "));
    Serial.print(extractorState ? F("ON") : F("OFF"));
    Serial.print(F(" | Nebulizador: "));
    Serial.print(nebulizerState ? F("ON") : F("OFF"));
    Serial.print(F(" | Errores: "));
    Serial.println(dht.getErrorCount());

    // Esperar antes de la siguiente lectura
    delay(READ_INTERVAL_MS);
}
