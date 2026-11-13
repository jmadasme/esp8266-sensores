/*
 * BasicRead - Ejemplo básico de lectura de temperatura y humedad con DHT22.
 *
 * Conexiones:
 *   DHT22 DATA → Pin digital 2
 *   DHT22 VDD  → 5V
 *   DHT22 GND  → GND
 *
 * Muestra por puerto serie cada 2 segundos:
 *   - Temperatura en °C y °F
 *   - Humedad relativa en %
 *
 * (c) 2026 - AG Creador de Contenido
 */

#include <DHT22.h>

// Crear objeto DHT22 en pin digital 2
DHT22 dht(2);

void setup() {
    Serial.begin(9600);
    while (!Serial) {
        delay(10);  // Esperar a que el puerto serie esté listo (solo para placas con USB nativo)
    }

    Serial.println(F("=== DHT22 BasicRead ==="));
    Serial.println(F("Inicializando sensor..."));

    if (!dht.begin()) {
        Serial.println(F("ERROR: No se pudo inicializar el sensor DHT22."));
        Serial.println(F("Verifique las conexiones y el pull-up."));
    } else {
        Serial.println(F("Sensor DHT22 inicializado correctamente."));
    }

    Serial.println();
    Serial.println(F("Temperatura(C)\tTemperatura(F)\tHumedad(%)"));
    Serial.println(F("----------------------------------------------"));
}

void loop() {
    // Leer temperatura (auto: respeta intervalo mínimo de 2s)
    float tempC = dht.readTemperature();
    float tempF = dht.readTemperatureF();
    float humidity = dht.readHumidity();

    // Verificar si los datos son válidos
    if (isnan(tempC) || isnan(tempF) || isnan(humidity)) {
        Serial.print(F("ERROR: Lectura invalida ("));
        Serial.print(dht.getErrorCount());
        Serial.println(F(" errores acumulados)"));
    } else {
        // Mostrar datos formateados
        Serial.print(tempC, 1);
        Serial.print(F(" C\t\t"));
        Serial.print(tempF, 1);
        Serial.print(F(" F\t\t"));
        Serial.print(humidity, 1);
        Serial.println(F(" %"));
    }

    // Esperar 2 segundos entre lecturas
    delay(2000);
}
