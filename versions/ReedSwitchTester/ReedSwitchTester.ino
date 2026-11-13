// ReedSwitchTester — Minimal test to understand MC-38 behavior
// Reads D2 (GPIO4) every 100ms and prints to Serial

#define REED_PIN 4   // D2

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== REED SWITCH TESTER ===");
  Serial.println("Testing 3 modes. Watch pin value when magnet near/far.\n");
}

void loop() {
  // MODE 1: INPUT_PULLUP — reed between D2 and GND
  pinMode(REED_PIN, INPUT_PULLUP);
  delay(10);
  int pullup = digitalRead(REED_PIN);

  // MODE 2: INPUT — reed between D2 and 3.3V
  pinMode(REED_PIN, INPUT);
  delay(10);
  int input33 = digitalRead(REED_PIN);

  // MODE 3: INPUT — reed between D2 and GND (floating)
  // already INPUT mode, same as above

  Serial.print("PULLUP(D2-GND)=");
  Serial.print(pullup);
  Serial.print(" | INPUT(D2-3.3V)=");
  Serial.print(input33);
  Serial.print(" | ");
  Serial.println(pullup == 0 ? "<<< CLOSED (magnet!)" : "    open");

  delay(100);
}
