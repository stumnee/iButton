/*
 * DS1971 (DS2430A) iButton EEPROM Reader/Writer - Arduino
 *
 * Reads and writes the 32-byte EEPROM and reads the 8-byte
 * application register from a DS1971/DS2430A iButton.
 *
 * DS1971 Write Process:
 *   1. Write data to scratchpad (0x0F)
 *   2. Read back scratchpad to verify (0xAA)
 *   3. Copy scratchpad to EEPROM (0x55 + 0xA5 validation key)
 *
 * Serial Commands (send via Serial Monitor, set to "Newline" line ending):
 *   r           - Read EEPROM and application register
 *   w AA HHHH.. - Write hex bytes starting at address AA
 *                  Example: w 00 48656C6C6F
 *                  Writes "Hello" starting at address 0x00
 *   w 00 *HH    - Fill entire EEPROM with byte HH
 *                  Example: w 00 *FF
 *                  Fills all 32 bytes with 0xFF
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

const byte CMD_READ_MEMORY    = 0xF0;
const byte CMD_READ_APP_REG   = 0xC3;
const byte CMD_WRITE_SCRATCH  = 0x0F;
const byte CMD_READ_SCRATCH   = 0xAA;
const byte CMD_COPY_SCRATCH   = 0x55;
const byte COPY_VALIDATION    = 0xA5;

const int EEPROM_SIZE = 32;
const int APP_REG_SIZE = 8;

OneWire ibutton(IBUTTON_PIN);

byte currentID[8] = {0};
bool keyPresent = false;

// --- Utility ---

void printHex(byte val) {
  if (val < 0x10) Serial.print("0");
  Serial.print(val, HEX);
}

void printID(byte *id) {
  Serial.print("ROM ID:   ");
  for (int i = 0; i < 8; i++) printHex(id[i]);
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

void printHexDump(byte *data, int len, int baseAddr) {
  for (int i = 0; i < len; i += 8) {
    printHex(baseAddr + i);
    Serial.print(": ");
    for (int j = 0; j < 8 && (i + j) < len; j++) {
      printHex(data[i + j]);
      Serial.print(" ");
    }
    Serial.print(" |");
    for (int j = 0; j < 8 && (i + j) < len; j++) {
      byte c = data[i + j];
      Serial.print((c >= 0x20 && c <= 0x7E) ? (char)c : '.');
    }
    Serial.println("|");
  }
}

int hexCharToVal(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

int parseHexByte(const char *str) {
  int hi = hexCharToVal(str[0]);
  int lo = hexCharToVal(str[1]);
  if (hi < 0 || lo < 0) return -1;
  return (hi << 4) | lo;
}

// --- OneWire Memory Operations ---

bool readEEPROM(byte *buffer) {
  if (!ibutton.reset()) return false;
  ibutton.select(currentID);
  ibutton.write(CMD_READ_MEMORY);
  ibutton.write(0x00);
  for (int i = 0; i < EEPROM_SIZE; i++) {
    buffer[i] = ibutton.read();
  }
  return true;
}

bool readAppRegister(byte *buffer) {
  if (!ibutton.reset()) return false;
  ibutton.select(currentID);
  ibutton.write(CMD_READ_APP_REG);
  ibutton.write(0x00);
  for (int i = 0; i < APP_REG_SIZE; i++) {
    buffer[i] = ibutton.read();
  }
  return true;
}

bool writeScratchpad(byte addr, byte *data, int len) {
  if (!ibutton.reset()) return false;
  ibutton.select(currentID);
  ibutton.write(CMD_WRITE_SCRATCH);
  ibutton.write(addr);
  for (int i = 0; i < len; i++) {
    ibutton.write(data[i]);
  }
  return true;
}

bool verifyScratchpad(byte addr, byte *expected, int len) {
  if (!ibutton.reset()) return false;
  ibutton.select(currentID);
  ibutton.write(CMD_READ_SCRATCH);
  ibutton.write(addr);
  for (int i = 0; i < len; i++) {
    byte val = ibutton.read();
    if (val != expected[i]) {
      Serial.print("Verify failed at offset ");
      Serial.print(i);
      Serial.print(": expected ");
      printHex(expected[i]);
      Serial.print(", got ");
      printHex(val);
      Serial.println();
      return false;
    }
  }
  return true;
}

bool copyScratchpadToEEPROM() {
  if (!ibutton.reset()) return false;
  ibutton.select(currentID);
  ibutton.write(CMD_COPY_SCRATCH);
  ibutton.write(COPY_VALIDATION);
  delay(30);  // DS1971 needs up to 30ms for EEPROM write
  return true;
}

bool writeEEPROM(byte addr, byte *data, int len) {
  Serial.print("Writing ");
  Serial.print(len);
  Serial.print(" byte(s) at address 0x");
  printHex(addr);
  Serial.println("...");

  // Step 1: Write to scratchpad
  if (!writeScratchpad(addr, data, len)) {
    Serial.println("ERROR: Failed to write scratchpad.");
    return false;
  }

  // Step 2: Verify scratchpad
  if (!verifyScratchpad(addr, data, len)) {
    Serial.println("ERROR: Scratchpad verification failed. Write aborted.");
    return false;
  }
  Serial.println("Scratchpad verified OK.");

  // Step 3: Copy scratchpad to EEPROM
  if (!copyScratchpadToEEPROM()) {
    Serial.println("ERROR: Failed to copy scratchpad to EEPROM.");
    return false;
  }

  Serial.println("EEPROM write complete.");
  return true;
}

// --- Read/Dump Command ---

void cmdRead() {
  if (!keyPresent) {
    Serial.println("No iButton present. Touch one to the reader first.");
    return;
  }

  Serial.println();
  printID(currentID);
  Serial.println();

  byte eeprom[EEPROM_SIZE];
  if (readEEPROM(eeprom)) {
    Serial.println("EEPROM (32 bytes):");
    printHexDump(eeprom, EEPROM_SIZE, 0x00);
  } else {
    Serial.println("ERROR: Failed to read EEPROM.");
  }
  Serial.println();

  byte appReg[APP_REG_SIZE];
  if (readAppRegister(appReg)) {
    Serial.println("Application Register (8 bytes):");
    printHexDump(appReg, APP_REG_SIZE, 0x00);
  } else {
    Serial.println("ERROR: Failed to read Application Register.");
  }
  Serial.println();
}

// --- Write Command ---

void cmdWrite(const char *args) {
  if (!keyPresent) {
    Serial.println("No iButton present. Touch one to the reader first.");
    return;
  }

  if (currentID[0] != 0x14) {
    Serial.println("ERROR: Device is not a DS1971 (family 0x14). Write aborted.");
    return;
  }

  // Skip leading spaces
  while (*args == ' ') args++;

  // Parse address
  if (strlen(args) < 2) {
    Serial.println("Usage: w AA HHHH..  or  w AA *HH");
    return;
  }
  int addr = parseHexByte(args);
  if (addr < 0 || addr >= EEPROM_SIZE) {
    Serial.println("ERROR: Invalid address (00-1F).");
    return;
  }
  args += 2;
  while (*args == ' ') args++;

  byte writeData[EEPROM_SIZE];
  int writeLen = 0;

  // Check for fill mode: *HH
  if (*args == '*') {
    args++;
    if (strlen(args) < 2) {
      Serial.println("Usage: w 00 *HH  (fill all 32 bytes with HH)");
      return;
    }
    int fillVal = parseHexByte(args);
    if (fillVal < 0) {
      Serial.println("ERROR: Invalid fill byte.");
      return;
    }
    writeLen = EEPROM_SIZE - addr;
    for (int i = 0; i < writeLen; i++) {
      writeData[i] = (byte)fillVal;
    }
  } else {
    // Parse hex string
    int maxLen = EEPROM_SIZE - addr;
    while (*args && writeLen < maxLen) {
      if (*args == ' ') { args++; continue; }
      if (strlen(args) < 2) {
        Serial.println("ERROR: Incomplete hex byte.");
        return;
      }
      int val = parseHexByte(args);
      if (val < 0) {
        Serial.print("ERROR: Invalid hex at '");
        Serial.print(args[0]);
        Serial.print(args[1]);
        Serial.println("'.");
        return;
      }
      writeData[writeLen++] = (byte)val;
      args += 2;
    }
  }

  if (writeLen == 0) {
    Serial.println("Usage: w AA HHHH..  or  w AA *HH");
    return;
  }

  // Show what we're about to write
  Serial.print("Data:  ");
  for (int i = 0; i < writeLen; i++) {
    printHex(writeData[i]);
    Serial.print(" ");
  }
  Serial.println();

  if (!writeEEPROM((byte)addr, writeData, writeLen)) {
    return;
  }

  // Read back and display
  Serial.println();
  Serial.println("Read-back verification:");
  byte verify[EEPROM_SIZE];
  if (readEEPROM(verify)) {
    printHexDump(verify, EEPROM_SIZE, 0x00);
  }
  Serial.println();
}

// --- Serial Command Handler ---

void processCommand(const char *cmd) {
  if (cmd[0] == 'r' || cmd[0] == 'R') {
    cmdRead();
  } else if (cmd[0] == 'w' || cmd[0] == 'W') {
    cmdWrite(cmd + 1);
  } else {
    Serial.println("Commands:");
    Serial.println("  r             Read EEPROM + app register");
    Serial.println("  w AA HHHH..   Write hex bytes at address AA");
    Serial.println("  w 00 *FF      Fill EEPROM with byte FF");
  }
}

// --- Main ---

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("DS1971 iButton EEPROM Reader/Writer");
  Serial.println("Commands: r = read, w AA HH.. = write, w 00 *FF = fill");
  Serial.println("Set line ending to 'Newline' in Serial Monitor.");
  Serial.println("Touch an iButton to the reader...");
  Serial.println();
}

char serialBuf[80];
int serialPos = 0;

void loop() {
  // Detect iButton presence
  byte id[8];
  bool found = false;

  if (ibutton.search(id)) {
    if (OneWire::crc8(id, 7) == id[7]) {
      found = true;
      if (!keyPresent || memcmp(id, currentID, 8) != 0) {
        memcpy(currentID, id, 8);
        keyPresent = true;
        digitalWrite(LED_PIN, HIGH);

        Serial.print("iButton detected: ");
        for (int i = 0; i < 8; i++) printHex(id[i]);
        Serial.print(" (family 0x");
        printHex(id[0]);
        Serial.println(")");

        if (id[0] != 0x14) {
          Serial.println("WARNING: Not a DS1971 (0x14). EEPROM commands may not work.");
        }

        // Auto-read on touch
        cmdRead();
      }
    }
  }

  ibutton.reset_search();

  if (!found && keyPresent) {
    keyPresent = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println("Key removed.\n");
  }

  // Process serial input
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialPos > 0) {
        serialBuf[serialPos] = '\0';
        processCommand(serialBuf);
        serialPos = 0;
      }
    } else if (serialPos < (int)sizeof(serialBuf) - 1) {
      serialBuf[serialPos++] = c;
    }
  }

  delay(200);
}
