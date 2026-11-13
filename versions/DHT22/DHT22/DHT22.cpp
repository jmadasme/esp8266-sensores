/*
 * DHT22.cpp - Implementación raw del protocolo single-bus para DHT22.
 *
 * Protocolo:
 *   1. Arduino pone DATA LOW ≥1ms, luego HIGH.
 *   2. Sensor responde: LOW 80µs → HIGH 80µs.
 *   3. 40 bits: cada bit → LOW 50µs, luego HIGH:
 *        - 26–28µs = bit 0
 *        - ~70µs   = bit 1
 *   4. Checksum: suma (byte[0] + byte[1] + byte[2] + byte[3]) & 0xFF == byte[4]
 *   5. Intervalo mínimo de 2 segundos entre lecturas.
 *
 * (c) 2026 - AG Creador de Contenido
 */

#include "DHT22.h"
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Constantes de temporización del protocolo single-bus (en microsegundos)
// ---------------------------------------------------------------------------
static const uint32_t START_LOW_US     = 1000UL;   // ≥1 ms para señal de START
static const uint32_t START_HIGH_US    = 30UL;     // Espera antes de respuesta
static const uint32_t RESPONSE_TIMEOUT = 100UL;    // Timeout esperando respuesta
static const uint32_t BIT_TIMEOUT      = 100UL;    // Timeout por bit (máx ~8ms sin interrupciones)
static const uint32_t BIT_HIGH_THRESH  = 40UL;     // Umbral 0/1 (~40µs)

// Rangos válidos del DHT22
static const float TEMP_MIN = -40.0f;
static const float TEMP_MAX = 80.0f;
static const float HUM_MIN  = 0.0f;
static const float HUM_MAX  = 100.0f;

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
DHT22::DHT22(uint8_t dataPin)
    : _dataPin(dataPin)
    , _temperature(NAN)
    , _humidity(NAN)
    , _lastReadValid(false)
    , _lastReadTime(0)
    , _errorCount(0)
    , _minInterval(2000)
{
    // Inicializar buffer
    for (uint8_t i = 0; i < 5; i++) {
        _data[i] = 0;
    }
}

// ---------------------------------------------------------------------------
// begin() — Configura pin y verifica comunicación básica
// ---------------------------------------------------------------------------
bool DHT22::begin() {
    pinMode(_dataPin, INPUT_PULLUP);
    delay(20);  // Estabilización del pin

    // Realizar una lectura de prueba para verificar comunicación real
    if (!read()) {
        return false;   // No hay sensor o no responde correctamente
    }

    return true;
}

// ---------------------------------------------------------------------------
// read() — Fuerza una lectura completa
// ---------------------------------------------------------------------------
bool DHT22::read() {
    // --- 1. Enviar señal de START ---
    if (!_sendStartSignal()) {
        _lastReadValid = false;
        _errorCount++;
        return false;
    }

    // --- 2. Leer 40 bits raw ---
    if (!_readRawData()) {
        _lastReadValid = false;
        _errorCount++;
        return false;
    }

    // --- 3. Verificar checksum ---
    if (!_verifyChecksum()) {
        _lastReadValid = false;
        _errorCount++;
        return false;
    }

    // --- 4. Parsear datos ---
    _parseData();

    // --- 5. Validar rangos ---
    if (!_validateRange()) {
        _lastReadValid = false;
        _errorCount++;
        return false;
    }

    _lastReadValid = true;
    _lastReadTime = millis();
    return true;
}

// ---------------------------------------------------------------------------
// readTemperature() - Con control de intervalo mínimo
// ---------------------------------------------------------------------------
float DHT22::readTemperature() {
    return readTemperature(false);
}

float DHT22::readTemperature(bool force) {
    if (force) {
        if (!read()) return NAN;
    } else {
        uint32_t now = millis();
        // Si ha pasado suficiente tiempo o nunca se ha leído, leer
        if (!_lastReadValid || (now - _lastReadTime >= _minInterval)) {
            if (!read()) return NAN;
        }
    }
    return _temperature;
}

// ---------------------------------------------------------------------------
// readTemperatureF() — Retorna temperatura en °F
// ---------------------------------------------------------------------------
float DHT22::readTemperatureF() {
    float tempC = readTemperature(false);
    if (isnan(tempC)) {
        return NAN;
    }
    return tempC * 9.0f / 5.0f + 32.0f;
}

float DHT22::readTemperatureF(bool force) {
    float tempC = readTemperature(force);
    if (isnan(tempC)) {
        return NAN;
    }
    return tempC * 9.0f / 5.0f + 32.0f;
}

// ---------------------------------------------------------------------------
// readHumidity() - Con control de intervalo mínimo
// ---------------------------------------------------------------------------
float DHT22::readHumidity() {
    return readHumidity(false);
}

float DHT22::readHumidity(bool force) {
    if (force) {
        if (!read()) return NAN;
    } else {
        uint32_t now = millis();
        if (!_lastReadValid || (now - _lastReadTime >= _minInterval)) {
            if (!read()) return NAN;
        }
    }
    return _humidity;
}

// ---------------------------------------------------------------------------
// Cálculo de Heat Index (fórmula Rothfusz)
// ---------------------------------------------------------------------------
float DHT22::computeHeatIndex(float tempC, float humidity) {
    // Si la temperatura es <= 27°C, usar fórmula simplificada (Steadman)
    // La fórmula de Steadman opera en °F, por lo que convertimos tempC a °F,
    // aplicamos la fórmula, y convertimos el resultado de vuelta a °C.
    if (tempC <= 27.0f) {
        float T_F = tempC * 9.0f / 5.0f + 32.0f;
        float HI_F = 0.5f * (T_F + 61.0f + ((T_F - 68.0f) * 1.2f) + (humidity * 0.094f));
        float HI_C = (HI_F - 32.0f) * 5.0f / 9.0f;
        return (HI_C < tempC) ? tempC : HI_C;
    }

    // Convertir a Fahrenheit para la fórmula Rothfusz
    float T = tempC * 9.0f / 5.0f + 32.0f;
    float R = humidity;

    // Regresión Rothfusz completa
    float HI_F = -42.379f
                 + 2.04901523f * T
                 + 10.14333127f * R
                 - 0.22475541f * T * R
                 - 6.83783e-3f * T * T
                 - 5.481717e-2f * R * R
                 + 1.22874e-3f * T * T * R
                 + 8.5282e-4f * T * R * R
                 - 1.99e-6f * T * T * R * R;

    // Convertir de vuelta a Celsius
    float HI_C = (HI_F - 32.0f) * 5.0f / 9.0f;

    // Si el HI calculado es menor que la temperatura ambiente, retornar temp
    if (HI_C < tempC) {
        return tempC;
    }

    return HI_C;
}

float DHT22::computeHeatIndexF(float tempF, float humidity) {
    float tempC = (tempF - 32.0f) * 5.0f / 9.0f;
    float hiC = computeHeatIndex(tempC, humidity);
    return hiC * 9.0f / 5.0f + 32.0f;
}

// ---------------------------------------------------------------------------
// Getters / Setters
// ---------------------------------------------------------------------------
uint32_t DHT22::getLastReadTime() {
    return _lastReadTime;
}

bool DHT22::isReadingValid() {
    return _lastReadValid;
}

uint8_t DHT22::getErrorCount() {
    return _errorCount;
}

void DHT22::resetErrorCount() {
    _errorCount = 0;
}

void DHT22::setMinInterval(uint16_t ms) {
    _minInterval = ms;
}

uint16_t DHT22::getMinInterval() {
    return _minInterval;
}

// ---------------------------------------------------------------------------
// PRIVADO: _sendStartSignal()
// ---------------------------------------------------------------------------
bool DHT22::_sendStartSignal() {
    // Deshabilitar interrupciones para temporización precisa
    noInterrupts();

    // 1. Poner pin como OUTPUT y LOW por ≥1 ms
    pinMode(_dataPin, OUTPUT);
    digitalWrite(_dataPin, LOW);
    delayMicroseconds(START_LOW_US);

    // 2. Liberar línea (HIGH) y esperar respuesta
    pinMode(_dataPin, INPUT_PULLUP);
    digitalWrite(_dataPin, HIGH);   // Activar pull-up explícitamente
    delayMicroseconds(START_HIGH_US);

    // 3. Esperar que el sensor baje la línea (respuesta LOW 80µs)
    uint32_t timeout = micros();
    while (digitalRead(_dataPin) == HIGH) {
        if ((micros() - timeout) > RESPONSE_TIMEOUT) {
            interrupts();
            return false;   // Timeout: sensor no responde
        }
    }

    // 4. Esperar que el sensor suba la línea (HIGH 80µs)
    timeout = micros();
    while (digitalRead(_dataPin) == LOW) {
        if ((micros() - timeout) > RESPONSE_TIMEOUT) {
            interrupts();
            return false;   // Timeout durante LOW de respuesta
        }
    }

    // 5. Esperar que termine el HIGH de respuesta
    timeout = micros();
    while (digitalRead(_dataPin) == HIGH) {
        if ((micros() - timeout) > RESPONSE_TIMEOUT) {
            interrupts();
            return false;   // Timeout durante HIGH de respuesta
        }
    }

    interrupts();
    return true;
}

// ---------------------------------------------------------------------------
// PRIVADO: _readRawData()
// ---------------------------------------------------------------------------
bool DHT22::_readRawData() {
    noInterrupts();

    // Leer 40 bits (5 bytes)
    for (uint8_t byteIdx = 0; byteIdx < 5; byteIdx++) {
        uint8_t result = 0;
        for (int8_t bitIdx = 7; bitIdx >= 0; bitIdx--) {
            // Esperar LOW (50µs) — inicio del bit
            uint32_t timeout = micros();
            while (digitalRead(_dataPin) == LOW) {
                if ((micros() - timeout) > BIT_TIMEOUT) {
                    interrupts();
                    return false;   // Timeout esperando LOW
                }
            }

            // Medir duración del HIGH
            timeout = micros();
            while (digitalRead(_dataPin) == HIGH) {
                if ((micros() - timeout) > BIT_TIMEOUT) {
                    interrupts();
                    return false;   // Timeout esperando HIGH
                }
            }

            uint32_t highDuration = micros() - timeout;

            // Si HIGH duró más del umbral → bit 1, sino bit 0
            if (highDuration > BIT_HIGH_THRESH) {
                result |= (1 << bitIdx);
            }
        }
        _data[byteIdx] = result;
    }

    interrupts();
    return true;
}

// ---------------------------------------------------------------------------
// PRIVADO: _verifyChecksum()
// ---------------------------------------------------------------------------
bool DHT22::_verifyChecksum() {
    uint8_t sum = _data[0] + _data[1] + _data[2] + _data[3];
    return (sum & 0xFF) == _data[4];
}

// ---------------------------------------------------------------------------
// PRIVADO: _parseData()
// ---------------------------------------------------------------------------
void DHT22::_parseData() {
    // Humedad: 16 bits, dividir entre 10 para obtener %
    uint16_t rawHum = ((uint16_t)_data[0] << 8) | _data[1];
    _humidity = rawHum / 10.0f;

    // Temperatura: 16 bits, dividir entre 10 para obtener °C
    // Bit 15 indica temperatura negativa
    uint16_t rawTemp = ((uint16_t)_data[2] << 8) | _data[3];
    if (rawTemp & 0x8000) {
        // Temperatura negativa
        rawTemp &= 0x7FFF;
        _temperature = -((float)rawTemp / 10.0f);
    } else {
        _temperature = rawTemp / 10.0f;
    }
}

// ---------------------------------------------------------------------------
// PRIVADO: _validateRange()
// ---------------------------------------------------------------------------
bool DHT22::_validateRange() {
    if (isnan(_temperature) || isnan(_humidity)) {
        return false;
    }
    if (_temperature < TEMP_MIN || _temperature > TEMP_MAX) {
        return false;
    }
    if (_humidity < HUM_MIN || _humidity > HUM_MAX) {
        return false;
    }
    return true;
}
