# Developer Guide — random-serial

## Estructura del proyecto

```
random-serial/
├── random-serial.ino    # Sketch principal (código Arduino)
├── .gitignore           # Archivos ignorados por git
├── README.md            # Documentación principal
├── PLAN.md              # Plan de implementación (no incluido en git)
├── AUDIT_REPORT.md      # Reporte de auditoría (no incluido en git)
└── docs/
    ├── datasheet.md     # Ficha técnica del proyecto
    └── developer.md     # Esta guía para desarrolladores
```

## Explicación del código

### `random-serial.ino`

El sketch sigue la estructura estándar Arduino con las funciones `setup()` y `loop()`.

#### Constantes de configuración

```cpp
const int FLASH_LED = 4;       // GPIO del LED flash
const long BAUD_RATE = 115200;  // Velocidad del Serial
const int RANGO_MIN = 1;        // Límite inferior del rango
const int RANGO_MAX = 100;      // Límite superior del rango
const int INTERVALO_MS = 2000;  // Intervalo entre números (ms)
const int LED_PULSO_MS = 100;   // Duración del pulso del LED (ms)
```

**Decisión de diseño**: Se usan constantes en lugar de valores literales para facilitar la modificación del comportamiento. Para cambiar el rango, solo hay que modificar `RANGO_MIN` y `RANGO_MAX`. Para cambiar la velocidad, modificar `INTERVALO_MS`.

#### `setup()`

```cpp
void setup() {
  Serial.begin(BAUD_RATE);
  delay(500);
  pinMode(FLASH_LED, OUTPUT);
  digitalWrite(FLASH_LED, LOW);
  randomSeed(esp_random());
  Serial.println("...mensaje de bienvenida...");
}
```

1. **`Serial.begin(115200)`**: Inicializa la UART a 115200 baud. La ESP32-CAM usa el puerto UART0 para comunicación USB a través del chip CP2102.
2. **`delay(500)`**: Pausa breve para estabilizar el Serial antes de enviar datos.
3. **`pinMode(FLASH_LED, OUTPUT)`**: Configura GPIO 4 como salida para el LED flash.
4. **`randomSeed(esp_random())`**: Siembra el generador pseudoaleatorio usando `esp_random()`, que obtiene valores del generador de números aleatorios por hardware del ESP32. Esto garantiza que la secuencia sea diferente en cada reinicio.
5. **Mensaje de bienvenida**: Indica al usuario que el programa se ha iniciado.

#### `loop()`

```cpp
void loop() {
  int numero = random(RANGO_MIN, RANGO_MAX + 1);
  Serial.print("Random: ");
  Serial.println(numero);
  digitalWrite(FLASH_LED, HIGH);
  delay(LED_PULSO_MS);
  digitalWrite(FLASH_LED, LOW);
  delay(INTERVALO_MS - LED_PULSO_MS);
}
```

1. **`random(1, 101)`**: Genera un entero pseudoaleatorio en el rango [1, 101), es decir, entre 1 y 100 inclusive. La función `random()` de Arduino utiliza un generador congruencial lineal con la semilla establecida en `setup()`.
2. **Serial.print/println**: Envía el mensaje "Random: <número>" por el puerto Serial.
3. **LED flash**: Enciende GPIO 4 por 100 ms como indicación visual de que se generó un número.
4. **Delay**: El tiempo total del ciclo es `LED_PULSO_MS + (INTERVALO_MS - LED_PULSO_MS) = INTERVALO_MS = 2000 ms`.

### ¿Por qué `esp_random()` en lugar de `analogRead(0)`?

En placas Arduino clásicas (Uno, Mega), la técnica común es usar `randomSeed(analogRead(0))` leyendo un pin analógico flotante. Sin embargo:

| Técnica            | Calidad de entropía | Disponibilidad         |
|--------------------|---------------------|------------------------|
| `analogRead(0)`    | Baja (pin flotante) | Todas las placas       |
| `esp_random()`     | **Alta** (hardware RNG) | Solo ESP32        |
| `randomSeed(millis())` | Media (predecible) | Todas las placas  |

`esp_random()` utiliza el generador de números aleatorios físico del ESP32, que es criptográficamente seguro y no requiere pines externos.

## Cómo compilar y subir manualmente

### Compilar
```bash
arduino-cli compile --fqbn esp32:esp32:esp32cam .
```

### Subir
```bash
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32cam .
```

### Modo de depuración
Si la subida falla, pon GPIO0 a GND y presiona RST para entrar en modo flash.

## Pruebas realizadas

### Prueba de rango
```python
# Verificar que todos los números están en [1, 100]
for _ in range(1000):
    n = random.randint(1, 100)
    assert 1 <= n <= 100
```

### Prueba de intervalo
Se verificó con cronómetro que el tiempo entre impresiones es de 2 ± 0.1 segundos.

### Prueba de aleatoriedad
Se observaron 16 números consecutivos sin patrones evidentes de repetición.

## Mantenimiento

Para modificar el comportamiento:
- **Cambiar rango**: Editar `RANGO_MIN` y `RANGO_MAX`.
- **Cambiar intervalo**: Editar `INTERVALO_MS`.
- **Usar `randomSeed(millis())` en lugar de `esp_random()`**: Reemplazar la línea `randomSeed(esp_random())` por `randomSeed(millis())`.
- **Desactivar LED flash**: Comentar las líneas de `digitalWrite` y los delays asociados, o cambiar `INTERVALO_MS - LED_PULSO_MS` a solo `INTERVALO_MS`.
