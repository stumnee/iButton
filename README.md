# DS1971 iButton EEPROM Reader - Arduino

Read the full memory contents of a DS1971 (DS2430A) iButton using an Arduino and the OneWire protocol. Dumps the 32-byte EEPROM and 8-byte application register in a hex+ASCII format.

## Hardware

- Arduino (Uno, Nano, Mega, etc.)
- DS1971 / DS2430A iButton
- iButton probe or reader holder
- 4.7kΩ resistor (pull-up)
- Jumper wires

## Wiring

```
Arduino Pin 2  ──┬── iButton data contact (center)
                  │
               4.7kΩ
                  │
Arduino 5V   ────┘

Arduino GND  ──────── iButton ground (outer ring)
```

| Arduino Pin | Connection             |
|-------------|------------------------|
| D2          | iButton data (center)  |
| 5V          | 4.7kΩ pull-up to D2    |
| GND         | iButton ground (ring)  |

## Dependencies

- [OneWire](https://www.pjrc.com/teensy/td_libs_OneWire.html) library

Install via Arduino IDE: **Sketch → Include Library → Manage Libraries → search "OneWire"**

## Usage

1. Wire the circuit as shown above.
2. Open `ibutton_reader/ibutton_reader.ino` in the Arduino IDE.
3. Upload to your Arduino.
4. Open the Serial Monitor at **9600 baud**.
5. Touch a DS1971 iButton to the reader.

### Example Output

```
DS1971 iButton EEPROM Reader
Touch an iButton to the reader...

=============================
ROM ID:   1400000123ABCD1E
Family:   0x14
Serial:   00:00:01:23:AB:CD

EEPROM (32 bytes):
00: 48 65 6C 6C 6F 20 57 6F  |Hello Wo|
08: 72 6C 64 21 00 00 00 00  |rld!....|
10: FF FF FF FF FF FF FF FF  |........|
18: FF FF FF FF FF FF FF FF  |........|

Application Register (8 bytes):
00: 01 02 03 04 05 06 07 08  |........|
=============================

Key removed.
```

## DS1971 Memory Map

| Region               | Size     | Command |
|----------------------|----------|---------|
| EEPROM               | 32 bytes | 0xF0    |
| Application Register | 8 bytes  | 0xC3    |

## Behavior

- The onboard LED (pin 13) lights up while a key is held on the reader.
- Duplicate reads are suppressed — the same key is only printed once per touch.
- CRC validation ensures only valid ROM reads are displayed.
- A warning is shown if the family code is not 0x14 (DS1971).

## License

MIT
