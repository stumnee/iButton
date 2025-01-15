/*
 * iButton Reader - Arduino
 *
 * Reads iButton (DS1990A) key IDs using the OneWire protocol.
 *
 * Wiring:
 *   - iButton data pin -> Arduino digital pin 2
 *   - 4.7kΩ pull-up resistor between data pin and 5V
 *   - iButton ground -> Arduino GND
 *
 * Required library: OneWire (install via Arduino Library Manager)
 */

#include <OneWire.h>

const int IBUTTON_PIN = 2;
const int LED_PIN = 13;

OneWire ibutton(IBUTTON_PIN);

byte lastKey[8] = {0};
bool keyPresent = false;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("iButton Reader Ready");
  Serial.println("Touch an iButton to the reader...");
}

void loop() {
  byte id[8];

  if (!ibutton.search(id)) {
    ibutton.reset_search();
    if (keyPresent) {
      keyPresent = false;
      digitalWrite(LED_PIN, LOW);
      Serial.println("Key removed.\n");
    }
    delay(200);
    return;
  }

  // Check CRC
  if (OneWire::crc8(id, 7) != id[7]) {
    Serial.println("CRC error - bad read.");
    ibutton.reset_search();
    delay(200);
    return;
  }

  // Check if it's the same key still held on the reader
  if (memcmp(id, lastKey, 8) == 0 && keyPresent) {
    ibutton.reset_search();
    delay(200);
    return;
  }

  // New key detected
  memcpy(lastKey, id, 8);
  keyPresent = true;
  digitalWrite(LED_PIN, HIGH);

  Serial.print("Family: 0x");
  if (id[0] < 0x10) Serial.print("0");
  Serial.println(id[0], HEX);

  Serial.print("ID:     ");
  for (int i = 1; i < 7; i++) {
    if (id[i] < 0x10) Serial.print("0");
    Serial.print(id[i], HEX);
    if (i < 6) Serial.print(":");
  }
  Serial.println();

  Serial.print("Full:   ");
  for (int i = 0; i < 8; i++) {
    if (id[i] < 0x10) Serial.print("0");
    Serial.print(id[i], HEX);
  }
  Serial.println();
  Serial.println("---");

  ibutton.reset_search();
  delay(200);
}
