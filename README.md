# DS1971 iButton EEPROM Reader/Writer - Arduino

Read and write the full memory of a DS1971 (DS2430A) iButton using an Arduino and the OneWire protocol. Dumps the 32-byte EEPROM and 8-byte application register in hex+ASCII format, and supports writing arbitrary data to the EEPROM via serial commands.

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
5. **Set line ending to "Newline"** in the Serial Monitor dropdown.
6. Touch a DS1971 iButton to the reader — it auto-reads on contact.

## Serial Commands

| Command          | Description                                  |
|------------------|----------------------------------------------|
| `r`              | Read EEPROM and application register         |
| `w AA HHHH..`    | Write hex bytes starting at address AA       |
| `w AA *HH`       | Fill EEPROM from address AA to end with byte |

### Write Examples

```
w 00 48656C6C6F       Write "Hello" at address 0x00
w 10 DEADBEEF         Write 4 bytes at address 0x10
w 00 *FF              Fill entire EEPROM with 0xFF (erase)
w 00 *00              Fill entire EEPROM with 0x00
```

## Write Process

The DS1971 uses a safe three-step write:

1. **Write scratchpad** (0x0F) — data goes to a temporary buffer
2. **Verify scratchpad** (0xAA) — read back and compare before committing
3. **Copy to EEPROM** (0x55 + 0xA5) — commit verified data to permanent storage

If verification fails, the write is aborted and EEPROM is unchanged.

### Example Output

```
DS1971 iButton EEPROM Reader/Writer
Commands: r = read, w AA HH.. = write, w 00 *FF = fill
Touch an iButton to the reader...

iButton detected: 1400000123ABCD1E (family 0x14)

ROM ID:   1400000123ABCD1E
Family:   0x14
Serial:   00:00:01:23:AB:CD

EEPROM (32 bytes):
00: FF FF FF FF FF FF FF FF  |........|
08: FF FF FF FF FF FF FF FF  |........|
10: FF FF FF FF FF FF FF FF  |........|
18: FF FF FF FF FF FF FF FF  |........|

Application Register (8 bytes):
00: 00 00 00 00 00 00 00 00  |........|

> w 00 48656C6C6F
Writing 5 byte(s) at address 0x00...
Data:  48 65 6C 6C 6F
Scratchpad verified OK.
EEPROM write complete.

Read-back verification:
00: 48 65 6C 6C 6F FF FF FF  |Hello...|
08: FF FF FF FF FF FF FF FF  |........|
10: FF FF FF FF FF FF FF FF  |........|
18: FF FF FF FF FF FF FF FF  |........|
```

## DS1971 Memory Map

| Region               | Size     | Read Cmd | Write Cmd       |
|----------------------|----------|----------|-----------------|
| EEPROM               | 32 bytes | 0xF0     | 0x0F → 0x55     |
| Application Register | 8 bytes  | 0xC3     | 0x99 → 0x5A (OTP) |

**Note:** The application register is **one-time programmable** (OTP). Once written, it cannot be changed. This sketch does not include app register writing to prevent accidental irreversible writes.

## Behavior

- Auto-reads EEPROM when an iButton is touched to the reader.
- The onboard LED (pin 13) lights up while a key is present.
- Duplicate reads are suppressed — the same key is only dumped once per touch.
- Write commands verify data via scratchpad before committing to EEPROM.
- Family code is checked — a warning is shown for non-DS1971 devices.

## License

MIT
