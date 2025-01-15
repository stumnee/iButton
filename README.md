# iButton Reader - Arduino

Read iButton (DS1990A) key IDs using an Arduino and the OneWire protocol. Prints the family code, unique ID, and full 8-byte ROM to the serial monitor.

## Hardware

- Arduino (Uno, Nano, Mega, etc.)
- iButton key (DS1990A or compatible 1-Wire device)
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
5. Touch an iButton to the reader.

### Example Output

```
iButton Reader Ready
Touch an iButton to the reader...
Family: 0x01
ID:     A3:B2:00:12:4F:C8
Full:   01A3B200124FC8E7
---
Key removed.
```

## Behavior

- The onboard LED (pin 13) lights up while a key is held on the reader.
- Duplicate reads are suppressed — the same key is only printed once per touch.
- CRC validation ensures only valid reads are displayed.

## License

MIT
