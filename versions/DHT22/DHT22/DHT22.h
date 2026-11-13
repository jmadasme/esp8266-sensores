/*
 * DHT22.h - Librería para sensor DHT22 (AM2302) de temperatura y humedad.
 *
 * Implementación raw del protocolo single-bus propietario.
 * NO utiliza librerías de Adafruit ni terceros.
 *
 * Conexiones:
 *   DATA → Pin digital (configurable en constructor)
 *   VDD  → 5V
 *   GND  → GND
 *   Pull-up externo 4.7kΩ–10kΩ entre VDD y DATA (si el módulo no lo incluye)
 *
 * (c) 2026 - AG Creador de Contenido
 */

#ifndef DHT22_H
#define DHT22_H

#include <Arduino.h>

class DHT22 {
public:
    /**
     * Constructor.
     * @param dataPin Número del pin digital conectado a DATA del sensor.
     */
    explicit DHT22(uint8_t dataPin);

    /**
     * Inicializa el pin y prepara la comunicación.
     * @return true si el sensor responde, false en caso contrario.
     */
    bool begin();

    /**
     * Realiza una lectura completa (forzada, ignora intervalo mínimo).
     * @return true si la lectura fue exitosa y el checksum válido.
     */
    bool read();

    /**
     * Retorna la última temperatura leída en grados Celsius.
     * Si ha pasado suficiente tiempo desde la última lectura (> minInterval),
     * realiza una nueva lectura automáticamente.
     * @return Temperatura en °C, o NAN si no hay datos válidos.
     */
    float readTemperature();

    /**
     * Retorna la temperatura en °C, con opción de forzar lectura.
     * @param force true fuerza una nueva lectura ignorando el intervalo mínimo.
     * @return Temperatura en °C, o NAN si no hay datos válidos.
     */
    float readTemperature(bool force);

    /**
     * Retorna la última temperatura leída en grados Fahrenheit.
     * Si ha pasado suficiente tiempo desde la última lectura (> minInterval),
     * realiza una nueva lectura automáticamente.
     * @return Temperatura en °F, o NAN si no hay datos válidos.
     */
    float readTemperatureF();

    /**
     * Retorna la temperatura en °F, con opción de forzar lectura.
     * @param force true fuerza una nueva lectura ignorando el intervalo mínimo.
     * @return Temperatura en °F, o NAN si no hay datos válidos.
     */
    float readTemperatureF(bool force);

    /**
     * Retorna la última humedad relativa leída en porcentaje.
     * Si ha pasado suficiente tiempo desde la última lectura (> minInterval),
     * realiza una nueva lectura automáticamente.
     * @return Humedad en % (0–100), o NAN si no hay datos válidos.
     */
    float readHumidity();

    /**
     * Retorna la humedad en %, con opción de forzar lectura.
     * @param force true fuerza una nueva lectura ignorando el intervalo mínimo.
     * @return Humedad en %, o NAN si no hay datos válidos.
     */
    float readHumidity(bool force);

    /**
     * Calcula el Heat Index (índice de calor) en grados Celsius
     * usando la fórmula de Rothfusz cuando T > 27°C.
     * @param tempC Temperatura ambiente en °C.
     * @param humidity Humedad relativa en %.
     * @return Índice de calor en °C.
     */
    float computeHeatIndex(float tempC, float humidity);

    /**
     * Calcula el Heat Index (índice de calor) en grados Fahrenheit
     * usando la fórmula de Rothfusz cuando T > 80°F.
     * @param tempF Temperatura ambiente en °F.
     * @param humidity Humedad relativa en %.
     * @return Índice de calor en °F.
     */
    float computeHeatIndexF(float tempF, float humidity);

    /**
     * Retorna el timestamp (millis) de la última lectura exitosa.
     */
    uint32_t getLastReadTime();

    /**
     * Indica si los últimos datos leídos son válidos.
     * @return true si la última lectura fue exitosa y los datos son coherentes.
     */
    bool isReadingValid();

    /**
     * Retorna el contador acumulado de errores de comunicación.
     */
    uint8_t getErrorCount();

    /**
     * Reinicia el contador de errores a cero.
     */
    void resetErrorCount();

    /**
     * Configura el intervalo mínimo entre lecturas.
     * @param ms Intervalo en milisegundos (default: 2000).
     */
    void setMinInterval(uint16_t ms);

    /**
     * Retorna el intervalo mínimo configurado en milisegundos.
     */
    uint16_t getMinInterval();

private:
    uint8_t _dataPin;            // Pin de datos
    float   _temperature;        // Última temperatura válida en °C
    float   _humidity;           // Última humedad válida en %
    bool    _lastReadValid;      // Flag de validez de última lectura
    uint32_t _lastReadTime;      // millis() de la última lectura exitosa
    uint8_t  _errorCount;        // Contador de errores
    uint16_t _minInterval;       // Intervalo mínimo entre lecturas (ms)
    uint8_t  _data[5];           // Buffer de 40 bits (5 bytes) raw

    /**
     * Envía la señal de START y espera la respuesta del sensor.
     * @return true si el sensor respondió correctamente.
     */
    bool _sendStartSignal();

    /**
     * Lee 40 bits (5 bytes) del sensor mediante el protocolo single-bus.
     * @return true si se leyeron 40 bits sin timeout.
     */
    bool _readRawData();

    /**
     * Verifica el checksum de los 40 bits recibidos.
     * @return true si checksum(byte[0]+byte[1]+byte[2]+byte[3]) == byte[4].
     */
    bool _verifyChecksum();

    /**
     * Convierte los bytes raw a temperatura y humedad.
     */
    void _parseData();

    /**
     * Verifica que la temperatura y humedad estén dentro de rangos válidos.
     * @return true si los valores son físicamente posibles.
     */
    bool _validateRange();
};

#endif // DHT22_H
