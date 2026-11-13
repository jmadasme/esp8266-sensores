/**
 * random-serial.ino - Generador de Números Aleatorios + Saludo Morse
 * 
 * Placa: ESP32-CAM AI-Thinker (esp32:esp32:esp32cam)
 * 
 * Comportamiento:
 *   1. Saluda "HOLA" en código Morse con el LED flash (GPIO 4)
 *   2. Luego imprime números aleatorios 1-100 por Serial cada 2s
 * 
 * Pines:
 *   - GPIO 4: LED Flash integrado (OUTPUT)
 *   - TX/RX:  Comunicación Serial USB
 */

const int FLASH_LED = 4;
const long BAUD_RATE = 115200;
const int RANGO_MIN = 1;
const int RANGO_MAX = 100;
const int INTERVALO_MS = 2000;

// === Timing Morse ===
const int DOT_MS = 150;
const int DASH_MS = 450;
const int GAP_MS = 150;

// Tabla Morse
struct MorseChar {
  char letter;
  const char* code;
};

const MorseChar MORSE[] = {
  {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},
  {'E', "."},    {'F', "..-."}, {'G', "--."},  {'H', "...."},
  {'I', ".."},   {'J', ".---"}, {'K', "-.-"},  {'L', ".-.."},
  {'M', "--"},   {'N', "-."},   {'O', "---"},  {'P', ".--."},
  {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
  {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"},
  {'Y', "-.--"}, {'Z', "--.."},
  {'0', "-----"},{'1', ".----"},{'2', "..---"},{'3', "...--"},
  {'4', "....-"},{'5', "....."},{'6', "-...."},{'7', "--..."},
  {'8', "---.."},{'9', "----."},
};
const int MORSE_COUNT = sizeof(MORSE) / sizeof(MORSE[0]);

void flashSymbol(char s) {
  int dur = (s == '.') ? DOT_MS : DASH_MS;
  digitalWrite(FLASH_LED, HIGH);
  delay(dur);
  digitalWrite(FLASH_LED, LOW);
  delay(GAP_MS);
}

const char* getMorse(char c) {
  c = toupper(c);
  for (int i = 0; i < MORSE_COUNT; i++)
    if (MORSE[i].letter == c) return MORSE[i].code;
  return NULL;
}

void flashChar(char c) {
  const char* code = getMorse(c);
  if (!code) { delay(DASH_MS); return; }
  for (int i = 0; code[i]; i++) flashSymbol(code[i]);
  delay(DASH_MS - GAP_MS);
}

void flashWord(const char* w) {
  for (int i = 0; w[i]; i++) {
    if (w[i] == ' ') delay(DASH_MS * 2);
    else flashChar(w[i]);
  }
}

void flashNumber(int n) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%d", n);
  flashWord(buf);
  delay(DASH_MS * 2);
}

void setup() {
  Serial.begin(BAUD_RATE);
  delay(500);

  pinMode(FLASH_LED, OUTPUT);
  digitalWrite(FLASH_LED, LOW);

  randomSeed(esp_random());

  // 3 parpadeos inicio
  for (int i = 0; i < 3; i++) {
    digitalWrite(FLASH_LED, HIGH); delay(100);
    digitalWrite(FLASH_LED, LOW);  delay(200);
  }
  delay(500);

  // "HOLA" en Morse por LED
  flashWord("HOLA");
  delay(1000);

  // Salida Serial
  Serial.println("=======================================");
  Serial.println(" Generador de Numeros Aleatorios");
  Serial.println(" ESP32-CAM AI-Thinker");
  Serial.println(" Rango: 1-100 | Intervalo: 2s");
  Serial.println(" LED: saludo Morse HOLA + numeros");
  Serial.println("=======================================");
}

void loop() {
  int numero = random(RANGO_MIN, RANGO_MAX + 1);

  Serial.print("Random: ");
  Serial.println(numero);

  flashNumber(numero);
  delay(INTERVALO_MS);
}
