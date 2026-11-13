/*
 * HeatIndex - Lectura de temperatura, humedad e índice de calor con DHT22.
 *
 * Conexiones:
 *   DHT22 DATA → Pin digital 2
 *   DHT22 VDD  → 5V
 *   DHT22 GND  → GND
 *
 * Muestra por puerto serie cada 2 segundos:
 *   - Temperatura en °C y °F
 *   - Humedad relativa en %
 *   - Índice de calor (Heat Index) en °C y °F
 *
 * El Heat Index se calcula usando la fórmula Rothfusz cuando T > 27°C.
 * Para temperaturas moderadas se usa una fórmula simplificada (Steadman).
 *
 * (c) 2026 - AG Creador de Contenido
 */

#include <DHT22.h>

// Crear objeto DHT22 en pin digital 2
DHT22 dht(2);

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(10);
    }

    Serial.println(F("=== DHT22 Heat Index ==="));
    Serial.println(F("Inicializando sensor..."));

    if (!dht.begin()) {
        Serial.println(F("ERROR: No se pudo inicializar el sensor DHT22."));
        Serial.println(F("Verifique las conexiones y el pull-up."));
    } else {
        Serial.println(F("Sensor DHT22 inicializado correctamente."));
    }

    Serial.println();
    Serial.println(F("Temp(C)\tTemp(F)\tHum(%)"));
    Serial.println(F("HI(C)\tHI(F)\tSensacion"));
    Serial.println(F("----------------------------------------------------------"));
}

void loop() {
    // Leer temperatura y humedad
    float tempC = dht.readTemperature();
    float tempF = dht.readTemperatureF();
    float humidity = dht.readHumidity();

    // Verificar validez
    if (isnan(tempC) || isnan(humidity)) {
        Serial.print(F("ERROR: Lectura invalida ("));
        Serial.print(dht.getErrorCount());
        Serial.println(F(" errores)"));
        delay(2000);
        return;
    }

    // Calcular Heat Index en °C y °F
    float hiC = dht.computeHeatIndex(tempC, humidity);
    float hiF = dht.computeHeatIndexF(tempF, humidity);

    // Mostrar datos
    Serial.print(tempC, 1);
    Serial.print(F(" C\t"));
    Serial.print(tempF, 1);
    Serial.print(F(" F\t"));
    Serial.print(humidity, 1);
    Serial.print(F(" %\t"));

    Serial.print(hiC, 1);
    Serial.print(F(" C\t"));
    Serial.print(hiF, 1);
    Serial.print(F(" F\t"));

    // Mensaje descriptivo según el HI
    if (hiC >= 54.0f) {
        Serial.println(F("!PELIGRO EXTREMO! Golpe de calor inminente."));
    } else if (hiC >= 41.0f) {
        Serial.println(F("!PELIGRO! Calambres/agotamiento por calor probable."));
    } else if (hiC >= 32.0f) {
        Serial.println(F("Precaucion: posible agotamiento con actividad prolongada."));
    } else if (hiC >= 27.0f) {
        Serial.println(F("Precaucion: posible fatiga con exposicion prolongada."));
    } else {
        Serial.println(F("Sin riesgo por calor."));
    }

    // Esperar 2 segundos entre lecturas
    delay(2000);
}
