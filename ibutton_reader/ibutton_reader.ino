/*
 * DS1971 (DS2430A) iButton EEPROM Reader - Arduino
 *
 * Reads the full 32-byte EEPROM and 8-byte application register
 * from a DS1971/DS2430A iButton using the OneWire protocol.
 *
 * DS1971 Memory Map:
 *   EEPROM:             32 bytes (0x00 - 0x1F)
 *   Application Register: 8 bytes
 *
 * DS1971 Commands:
 *   0xF0 - Read Memory (followed by start address)
 *   0xC3 - Read Application Register (followed by start address)
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

const byte CMD_READ_MEMORY = 0xF0;
const byte CMD_READ_APP_REG = 0xC3;

const int EEPROM_SIZE = 32;
const int APP_REG_SIZE = 8;

OneWire ibutton(IBUTTON_PIN);

byte lastKey[8] = {0};
bool keyPresent = false;

void printHex(byte val) {
  if (val < 0x10) Serial.print("0");
  Serial.print(val, HEX);
}

void printID(byte *id) {
  Serial.print("ROM ID:   ");
  for (int i = 0; i < 8; i++) {
    printHex(id[i]);
  }
  Serial.println();

  Serial.print("Family:   0x");
  printHex(id[0]);
  Serial.println();

  Serial.print("Serial:   ");
  for (int i = 1; i < 7; i++) {
    printHex(id[i]);
    if (i < 6) Serial.print(":");
  }
  Serial.println();
}

bool readEEPROM(byte *id, byte *buffer) {
  if (!ibutton.reset()) return false;
  ibutton.select(id);
  ibutton.write(CMD_READ_MEMORY);
  ibutton.write(0x00);  // start address

  for (int i = 0; i < EEPROM_SIZE; i++) {
    buffer[i] = ibutton.read();
  }
  return true;
}

bool readAppRegister(byte *id, byte *buffer) {
  if (!ibutton.reset()) return false;
  ibutton.select(id);
  ibutton.write(CMD_READ_APP_REG);
  ibutton.write(0x00);  // start address

  for (int i = 0; i < APP_REG_SIZE; i++) {
    buffer[i] = ibutton.read();
  }
  return true;
}

void printHexDump(byte *data, int len, int baseAddr) {
  for (int i = 0; i < len; i += 8) {
    // Address
    printHex(baseAddr + i);
    Serial.print(": ");

    // Hex bytes
    for (int j = 0; j < 8 && (i + j) < len; j++) {
      printHex(data[i + j]);
      Serial.print(" ");
    }

    // ASCII
    Serial.print(" |");
    for (int j = 0; j < 8 && (i + j) < len; j++) {
      byte c = data[i + j];
      Serial.print((c >= 0x20 && c <= 0x7E) ? (char)c : '.');
    }
    Serial.println("|");
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  Serial.println("DS1971 iButton EEPROM Reader");
  Serial.println("Touch an iButton to the reader...");
  Serial.println();
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

  // Validate CRC
  if (OneWire::crc8(id, 7) != id[7]) {
    Serial.println("CRC error - bad read.");
    ibutton.reset_search();
    delay(200);
    return;
  }

  // Skip if same key still on reader
  if (memcmp(id, lastKey, 8) == 0 && keyPresent) {
    ibutton.reset_search();
    delay(200);
    return;
  }

  memcpy(lastKey, id, 8);
  keyPresent = true;
  digitalWrite(LED_PIN, HIGH);

  Serial.println("=============================");
  printID(id);
  Serial.println();

  // Check family code (DS1971/DS2430A = 0x14)
  if (id[0] != 0x14) {
    Serial.print("WARNING: Family 0x");
    printHex(id[0]);
    Serial.println(" is not DS1971 (expected 0x14).");
    Serial.println("EEPROM read may fail or return invalid data.");
    Serial.println();
  }

  // Read EEPROM
  byte eeprom[EEPROM_SIZE];
  if (readEEPROM(id, eeprom)) {
    Serial.println("EEPROM (32 bytes):");
    printHexDump(eeprom, EEPROM_SIZE, 0x00);
  } else {
    Serial.println("ERROR: Failed to read EEPROM.");
  }
  Serial.println();

  // Read Application Register
  byte appReg[APP_REG_SIZE];
  if (readAppRegister(id, appReg)) {
    Serial.println("Application Register (8 bytes):");
    printHexDump(appReg, APP_REG_SIZE, 0x00);
  } else {
    Serial.println("ERROR: Failed to read Application Register.");
  }

  Serial.println("=============================");
  Serial.println();

  ibutton.reset_search();
  delay(200);
}
