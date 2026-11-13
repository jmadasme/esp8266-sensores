/**
 * DHTMonitor.ino - Monitor ambiental con DHT11 y LCD1602 I2C
 *
 * Lee temperatura y humedad de un sensor DHT11 (o DHT22) y las
 * muestra en el LCD cada 2 segundos. Incluye indicadores visuales
 * como barra de progreso para la humedad y animación de ventilador.
 *
 * --- MODO SIMULADO ---
 * Por defecto, este ejemplo funciona SIN sensor físico, generando
 * datos simulados para demostración. Para usar un DHT real:
 *   1. Instala la librería "DHT sensor library" de Adafruit
 *   2. Conecta el DHT11 al pin digital definido abajo
 *   3. Cambia USE_REAL_DHT a 1
 *
 * Conexiones:
 *   LCD I2C VCC → 5V
 *   LCD I2C GND → GND
 *   LCD I2C SDA → A4
 *   LCD I2C SCL → A5
 *   DHT11 OUT  → pin digital 7  (solo si USE_REAL_DHT = 1)
 *   DHT11 VCC  → 5V
 *   DHT11 GND  → GND
 *
 * Dependencias:
 *   - Wire.h
 *   - LCD1602_I2C.h
 *   - DHT sensor library (solo si USE_REAL_DHT = 1)
 */

#include <Wire.h>
#include <LCD1602_I2C.h>

// ─── Configuración ───────────────────────────────────────────────────────────

// Cambiar a 1 para usar un sensor DHT real
#define USE_REAL_DHT 0

#if USE_REAL_DHT
    #include <DHT.h>
    #define DHT_PIN 7
    #define DHT_TYPE DHT11   // DHT11 o DHT22
    DHT dht(DHT_PIN, DHT_TYPE);
#endif

LCD1602_I2C lcd(0x27, 16, 2);

// Variables de simulación
float simTemp    = 25.0f;
float simHum     = 60.0f;
int   simTrend   = 1;   // 1 = subiendo, -1 = bajando

// ─── Caracteres personalizados ──────────────────────────────────────────────

// Termómetro (índice 6)
const uint8_t thermometer[8] PROGMEM = {
    0b00100,
    0b01010,
    0b01010,
    0b01110,
    0b01110,
    0b11111,
    0b11111,
    0b01110
};

// Gota de agua / humedad (índice 7)
const uint8_t waterDrop[8] PROGMEM = {
    0b00100,
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00000
};

// ─────────────────────────────────────────────────────────────────────────────

void setup()
{
    Serial.begin(9600);
    Serial.println(F("DHTMonitor Example"));

    if (!lcd.beginAuto()) {
        Serial.println(F("ERROR: LCD no detectado"));
        while (1) delay(1000);
    }

    lcd.backlightOn();

    // Crear caracteres personalizados
    uint8_t thermoBuf[8], dropBuf[8];
    memcpy_P(thermoBuf, thermometer, 8);
    memcpy_P(dropBuf, waterDrop, 8);
    lcd.createChar(6, thermoBuf);
    lcd.createChar(7, dropBuf);

    // Mensaje de inicio
    lcd.clear();
    lcd.printCenter("DHT Monitor", 0);
    lcd.printCenter("Iniciando...", 1);
    delay(1500);
    lcd.clear();

#if USE_REAL_DHT
    dht.begin();
    Serial.println(F("Modo REAL: DHT sensor activado"));
#else
    Serial.println(F("Modo SIMULADO: generando datos ficticios"));
#endif
}

// ─────────────────────────────────────────────────────────────────────────────

void loop()
{
    float temperature, humidity;

#if USE_REAL_DHT
    // Leer sensor real
    humidity    = dht.readHumidity();
    temperature = dht.readTemperature();

    // Verificar si la lectura falló
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println(F("Error leyendo DHT"));
        lcd.clear();
        lcd.printCenter("ERROR DHT", 0);
        lcd.printCenter("Revisar sensor", 1);
        delay(2000);
        return;
    }
#else
    // Generar datos simulados con variación realista
    simulateDHT(temperature, humidity);
#endif

    // ─── Mostrar en LCD ────────────────────────────────────────────

    lcd.clear();

    // Fila 0: Temperatura
    lcd.displayChar(6, 0, 0);        // Icono termómetro
    lcd.setCursor(1, 0);
    lcd.print("Temp: ");
    lcd.print(temperature, 1);
    lcd.print("C");

    // Fila 1: Humedad
    lcd.displayChar(7, 0, 1);        // Icono gota
    lcd.setCursor(1, 1);
    lcd.print("Hum:  ");
    lcd.print(humidity, 1);
    lcd.print("%");

    // ─── Barra de humedad gráfica ──────────────────────────────────
    // Mostrar una mini-barra de 8 caracteres con el nivel de humedad
    if (humidity >= 0.0f && humidity <= 100.0f) {
        lcd.displayProgressBar((uint8_t)humidity, 9, 1, 7);
    }

    // ─── Mostrar en Serial ─────────────────────────────────────────
    Serial.print(F("Temperature: "));
    Serial.print(temperature, 1);
    Serial.print(F(" °C  |  Humidity: "));
    Serial.print(humidity, 1);
    Serial.println(F(" %"));

    delay(2000);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Simulación de datos DHT
// ─────────────────────────────────────────────────────────────────────────────

void simulateDHT(float& temp, float& hum)
{
    // Variación aleatoria suave
    float deltaTemp = (random(-10, 11)) * 0.1f;   // -1.0 a +1.0 °C
    float deltaHum  = (random(-15, 16)) * 0.1f;   // -1.5 a +1.5 %

    simTemp += deltaTemp;
    simHum  += deltaHum;

    // Mantener dentro de rangos realistas
    if (simTemp < 18.0f) simTemp = 18.0f;
    if (simTemp > 35.0f) simTemp = 35.0f;
    if (simHum < 30.0f)  simHum  = 30.0f;
    if (simHum > 90.0f)  simHum  = 90.0f;

    temp = simTemp;
    hum  = simHum;
}
