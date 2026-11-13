/**
 * LCD1602_I2C.h - Controlador para LCD1602 con interfaz I2C (PCF8574)
 *
 * Librería para Arduino que permite controlar un display LCD 16×2
 * mediante el expansor I2C PCF8574T/PCF8574AT.
 *
 * Características:
 *   - Detección automática de dirección I2C (0x27 / 0x3F / escaneo completo)
 *   - Control de retroiluminación por software
 *   - Caracteres personalizados (hasta 8, formato 5×8)
 *   - Desplazamiento de texto (scroll horizontal)
 *   - Barra de progreso animada con caracteres custom
 *   - Texto centrado automático
 */

#ifndef LCD1602_I2C_H
#define LCD1602_I2C_H

#include <Arduino.h>
#include <Wire.h>

/**
 * @class LCD1602_I2C
 * @brief Controlador para LCD 16×2 con interfaz I2C (PCF8574)
 *
 * Proporciona una interfaz completa para manejar un display LCD 1602
 * a través del bus I2C usando el expansor PCF8574. Soporta modos
 * de 4 bits, caracteres personalizados, retroiluminación, scroll,
 * barra de progreso y centrado de texto.
 */
class LCD1602_I2C {
public:
    /**
     * @brief Constructor
     * @param addr Dirección I2C del módulo (típicamente 0x27 o 0x3F)
     * @param cols Número de columnas del display (por defecto 16)
     * @param rows Número de filas del display (por defecto 2)
     */
    LCD1602_I2C(uint8_t addr, uint8_t cols = 16, uint8_t rows = 2);

    // ──────────────────────────────────────────────
    //  Inicialización
    // ──────────────────────────────────────────────

    /**
     * @brief Inicializa el LCD usando la dirección del constructor
     * @return true si el módulo responde, false en caso contrario
     */
    bool begin();

    /**
     * @brief Inicializa el LCD con una dirección específica
     * @param addr Dirección I2C a usar
     * @return true si el módulo responde, false en caso contrario
     */
    bool begin(uint8_t addr);

    /**
     * @brief Escanea el bus I2C y se conecta a la primera dirección
     *        de LCD que encuentre (prioriza 0x27 y 0x3F)
     * @return true si se detectó algún módulo compatible
     */
    bool beginAuto();

    // ──────────────────────────────────────────────
    //  Información del dispositivo
    // ──────────────────────────────────────────────

    /**
     * @brief Retorna la dirección I2C activa del módulo
     * @return Dirección I2C en formato uint8_t
     */
    uint8_t getAddress();

    /**
     * @brief Retorna el número de columnas del display
     * @return Número de columnas
     */
    uint8_t getCols();

    /**
     * @brief Retorna el número de filas del display
     * @return Número de filas
     */
    uint8_t getRows();

    // ──────────────────────────────────────────────
    //  Control de cursor y pantalla
    // ──────────────────────────────────────────────

    /**
     * @brief Posiciona el cursor en una columna y fila
     * @param col Columna (0 a cols-1)
     * @param row Fila (0 a rows-1)
     */
    void setCursor(uint8_t col, uint8_t row);

    /**
     * @brief Lleva el cursor a la posición (0, 0)
     */
    void home();

    /**
     * @brief Limpia toda la pantalla y lleva el cursor a home
     */
    void clear();

    // ──────────────────────────────────────────────
    //  Salida de texto
    // ──────────────────────────────────────────────

    /**
     * @brief Muestra una cadena de texto en la posición actual
     * @param str Cadena terminada en null
     */
    void print(const char* str);

    /**
     * @brief Muestra un número entero en la posición actual
     * @param value Valor entero a imprimir
     */
    void print(int value);

    /**
     * @brief Muestra un número decimal en la posición actual
     * @param value Valor flotante a imprimir
     * @param decimals Cantidad de decimales (por defecto 2)
     */
    void print(float value, uint8_t decimals = 2);

    // ──────────────────────────────────────────────
    //  Retroiluminación
    // ──────────────────────────────────────────────

    /**
     * @brief Enciende la retroiluminación
     */
    void backlightOn();

    /**
     * @brief Apaga la retroiluminación
     */
    void backlightOff();

    /**
     * @brief Controla el estado de la retroiluminación
     * @param on true para encender, false para apagar
     */
    void setBacklight(bool on);

    /**
     * @brief Retorna el estado de la retroiluminación
     * @return true si está encendida, false si está apagada
     */
    bool isBacklightOn();

    // ──────────────────────────────────────────────
    //  Caracteres personalizados
    // ──────────────────────────────────────────────

    /**
     * @brief Define un carácter personalizado en CGRAM
     * @param index Índice del carácter (0-7)
     * @param data Arreglo de 8 bytes con el patrón 5×8
     *
     * @note Después de crear caracteres, la dirección CGRAM se modifica.
     *       Se debe llamar a setCursor() o home() antes de imprimir texto.
     */
    void createChar(uint8_t index, const uint8_t* data);

    /**
     * @brief Muestra un carácter personalizado en una posición
     * @param index Índice del carácter (0-7)
     * @param col Columna destino
     * @param row Fila destino
     */
    void displayChar(uint8_t index, uint8_t col, uint8_t row);

    // ──────────────────────────────────────────────
    //  Desplazamiento (scroll)
    // ──────────────────────────────────────────────

    /**
     * @brief Desplaza el contenido completo a la izquierda
     * @param cols Número de columnas a desplazar
     * @param delayMs Tiempo de espera entre cada desplazamiento (ms)
     */
    void scrollLeft(uint8_t cols, uint16_t delayMs);

    /**
     * @brief Desplaza el contenido completo a la derecha
     * @param cols Número de columnas a desplazar
     * @param delayMs Tiempo de espera entre cada desplazamiento (ms)
     */
    void scrollRight(uint8_t cols, uint16_t delayMs);

    /**
     * @brief Desplaza un texto largo automáticamente
     * @param text Texto a desplazar
     * @param delayMs Tiempo entre desplazamientos (ms, por defecto 300)
     *
     * Si el texto es más corto que el ancho del display, lo muestra centrado.
     */
    void scrollText(const char* text, uint16_t delayMs = 300);

    // ──────────────────────────────────────────────
    //  Apariencia del cursor
    // ──────────────────────────────────────────────

    /**
     * @brief Activa o desactiva el parpadeo del cursor
     * @param enable true para activar parpadeo
     */
    void blinkCursor(bool enable);

    /**
     * @brief Muestra u oculta el cursor subrayado
     * @param enable true para mostrar cursor
     */
    void showCursor(bool enable);

    // ──────────────────────────────────────────────
    //  Efectos visuales
    // ──────────────────────────────────────────────

    /**
     * @brief Muestra una barra de progreso animada
     * @param percent Porcentaje (0-100)
     * @param col Columna de inicio
     * @param row Fila donde se muestra
     * @param length Longitud en caracteres de la barra
     *
     * @note Utiliza los caracteres personalizados en los índices 0-5.
     *       Cualquier carácter definido previamente en esos índices
     *       será sobrescrito.
     */
    void displayProgressBar(uint8_t percent, uint8_t col, uint8_t row, uint8_t length);

    // ──────────────────────────────────────────────
    //  Utilidades
    // ──────────────────────────────────────────────

    /**
     * @brief Muestra una cadena centrada en una fila
     * @param str Cadena a mostrar
     * @param row Fila donde centrar (0 o 1)
     */
    void printCenter(const char* str, uint8_t row);

private:
    uint8_t _addr;             ///< Dirección I2C del PCF8574
    uint8_t _cols;             ///< Columnas del display
    uint8_t _rows;             ///< Filas del display
    bool    _backlight;        ///< Estado de la retroiluminación
    bool    _initialized;      ///< Flag de inicialización completada
    bool    _cursorVisible;    ///< Estado del cursor subrayado
    bool    _cursorBlink;      ///< Estado del parpadeo del cursor

    // ──────────────────────────────────────────────
    //  Comunicación I2C de bajo nivel
    // ──────────────────────────────────────────────

    /**
     * @brief Escribe un byte en el expansor PCF8574
     * @param data Byte a escribir
     * @return true si la transmisión I2C fue exitosa, false en caso de error
     */
    bool _writeExpander(uint8_t data);

    /**
     * @brief Envía un nibble (4 bits) en el bus I2C con pulso E
     * @param nibble Valor de 4 bits a colocar en D4-D7
     * @param rs Nivel del pin RS (0=comando, 1=dato)
     */
    void _writeNibble(uint8_t nibble, uint8_t rs);

    /**
     * @brief Envía un byte completo (2 nibbles, high-first)
     * @param data Byte a enviar
     * @param rs Nivel del pin RS
     */
    void _writeByte(uint8_t data, uint8_t rs);

    // ──────────────────────────────────────────────
    //  Comandos HD44780
    // ──────────────────────────────────────────────

    /**
     * @brief Envía un comando al LCD (RS=0)
     * @param cmd Código de comando
     */
    void _sendCommand(uint8_t cmd);

    /**
     * @brief Envía un dato (carácter) al LCD (RS=1)
     * @param data Byte de datos
     */
    void _sendData(uint8_t data);

    // ──────────────────────────────────────────────
    //  Utilidades internas
    // ──────────────────────────────────────────────

    /**
     * @brief Verifica si un dispositivo responde en una dirección I2C
     * @param addr Dirección I2C a probar
     * @return true si hay respuesta
     */
    bool _detectAddress(uint8_t addr);

    /**
     * @brief Reconstruye el comando Display On/Off combinando
     *        los flags _cursorVisible y _cursorBlink mediante OR de bits
     *
     * Comando = 0x08 | 0x04 | (_cursorVisible ? 0x02 : 0) | (_cursorBlink ? 0x01 : 0)
     */
    void _updateDisplayMode();

    /**
     * @brief Secuencia de inicialización del HD44780 en modo 4 bits
     * @return true si la inicialización fue exitosa
     */
    bool _initLCD();

    /**
     * @brief Crea los 6 caracteres personalizados para la barra de progreso
     *
     * Los caracteres se almacenan en CGRAM en los índices 0-5.
     *   - Índice 0: vacío (0/5)
     *   - Índice 1: 1/5
     *   - Índice 2: 2/5
     *   - Índice 3: 3/5
     *   - Índice 4: 4/5
     *   - Índice 5: lleno (5/5)
     */
    void _createProgressChars();
};

#endif // LCD1602_I2C_H
