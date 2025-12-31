# Hardware Setup Guide - Waveshare ESP32-S3 1.8" AMOLED

This guide covers the hardware setup for the ESP32 Multi-Room Intercom using the **Waveshare ESP32-S3 1.8" AMOLED Touch Display** (368x448).

## Hardware Overview

### Waveshare ESP32-S3 1.8" AMOLED Board
- **MCU**: ESP32-S3-WROOM-1 (16MB Flash, 8MB PSRAM)
- **Display**: 1.8" AMOLED, 368x448 resolution
- **Display Controller**: RM67162 (QSPI interface)
- **Touch**: CST816S capacitive touch controller (I2C)
- **Built-in**: USB-C, battery charging circuit, GPIO breakout

**Specifications**:
- Resolution: 368×448 pixels (portrait)
- Colors: 65K RGB
- Viewing angle: Full angle
- Interface: QSPI (display), I2C (touch)
- Power: 3.3V~5V via USB-C or GPIO
- Dimensions: 39mm × 65mm

## Pin Assignments

### Display Pins (RM67162 - QSPI)
These pins are **fixed** on the Waveshare board:

| Pin | Function | GPIO |
|-----|----------|------|
| CS | Chip Select | 10 |
| DC | Data/Command | 11 |
| RST | Reset | 13 |
| SDA0 | QSPI Data 0 | 9 |
| SDA1 | QSPI Data 1 | 8 |
| SDA2 | QSPI Data 2 | 7 |
| SDA3 | QSPI Data 3 | 6 |
| SCL | QSPI Clock | 12 |
| BL | Backlight (PWM) | 38 |

### Touch Pins (CST816S - I2C)
These pins are **fixed** on the Waveshare board:

| Pin | Function | GPIO |
|-----|----------|------|
| SDA | I2C Data | 15 |
| SCL | I2C Clock | 16 |
| RST | Touch Reset | 21 |
| INT | Touch Interrupt | 14 |

### Available GPIOs for External Components

After accounting for display and touch, these GPIOs are available:

**Available GPIOs**: 1, 2, 3, 4, 5, 17, 18, 35, 36, 37, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48

**Reserved/Used**:
- 6-16: Display and touch
- 19-20: USB (don't use)
- 26-32: Not available on ESP32-S3

## External Audio Components

You need to connect external I2S audio components for the intercom functionality.

### Required Components

1. **I2S Microphone**: INMP441 (recommended) or similar
2. **I2S Amplifier + Speaker**: MAX98357A (recommended) or similar
3. **Speaker**: 3W-5W, 4Ω or 8Ω
4. **Jumper wires**: Female-to-female for breadboard

### I2S Microphone Wiring (INMP441)

**INMP441 → Waveshare ESP32-S3**

| INMP441 Pin | Function | ESP32-S3 GPIO | Notes |
|-------------|----------|---------------|-------|
| VDD | Power | 3.3V | Use 3.3V rail |
| GND | Ground | GND | Common ground |
| SD | Serial Data | GPIO 42 | Audio data output |
| WS | Word Select | GPIO 1 | Left/Right clock |
| SCK | Bit Clock | GPIO 2 | Serial clock |
| L/R | Channel Select | GND | GND=Left, VDD=Right |

**Wiring Notes**:
- Connect L/R to GND for left channel (mono)
- Use short wires (<15cm) to minimize noise
- Keep away from power wires

### I2S Amplifier Wiring (MAX98357A)

**MAX98357A → Waveshare ESP32-S3**

| MAX98357A Pin | Function | ESP32-S3 GPIO | Notes |
|---------------|----------|---------------|-------|
| VIN | Power | 5V or 3.3V | 5V for louder output |
| GND | Ground | GND | Common ground |
| DIN | Serial Data | GPIO 3 | Audio data input |
| BCLK | Bit Clock | GPIO 4 | Serial clock |
| LRC | Word Select | GPIO 5 | Left/Right clock |
| GAIN | Volume | GND or VDD | See table below |
| SD | Shutdown | VDD | VDD=On, GND=Off |

**Speaker Connection**:
- Connect speaker to "+" and "-" terminals on MAX98357A
- Use 4Ω or 8Ω speaker, 3W-5W power rating
- Polarity matters for phase, but either way works

**GAIN Settings**:
| GAIN Pin | Gain (dB) | Volume Level |
|----------|-----------|--------------|
| GND | 9 dB | Quiet |
| Float | 12 dB | Medium (default) |
| VDD | 15 dB | Loud |

**Recommendation**: Leave GAIN floating (not connected) for medium volume.

## Complete Wiring Diagram

```
Waveshare ESP32-S3 1.8" AMOLED
┌─────────────────────────────────┐
│                                 │
│   [1.8" AMOLED Display]         │
│                                 │
├─────────────────────────────────┤
│  ESP32-S3 GPIO Breakout:        │
│                                 │
│  3.3V ────┬──────┬──────────────┤ 3.3V Rail
│  5V ──────┼──────┼──────────────┤ 5V Rail (optional for MAX98357A)
│  GND ─────┼──────┼──────────────┤ Common Ground
│           │      │              │
│  GPIO 42 ─┼──────┤ (MIC SD)     │ ← INMP441 SD
│  GPIO 1 ──┼──────┤ (MIC WS)     │ ← INMP441 WS
│  GPIO 2 ──┼──────┤ (MIC SCK)    │ ← INMP441 SCK
│           │      │              │
│  GPIO 3 ──┼──────┼──────────────┤ → MAX98357A DIN
│  GPIO 4 ──┼──────┼──────────────┤ → MAX98357A BCLK
│  GPIO 5 ──┼──────┼──────────────┤ → MAX98357A LRC
│           │      │              │
└───────────┴──────┴──────────────┘
            │      │
      INMP441    MAX98357A + Speaker
```

## Assembly Steps

### 1. Prepare the Breadboard (Recommended for Testing)

Use a breadboard for initial testing before soldering:

1. Place Waveshare board on breadboard (or use jumper wires)
2. Place INMP441 microphone module on breadboard
3. Place MAX98357A amplifier module on breadboard
4. Use breadboard power rails for 3.3V and GND distribution

### 2. Connect Power Rails

1. Connect Waveshare **3.3V** to breadboard **+ rail**
2. Connect Waveshare **GND** to breadboard **- rail**
3. (Optional) Connect Waveshare **5V** to separate **+ rail** for MAX98357A

### 3. Wire I2S Microphone (INMP441)

```
INMP441 → Breadboard → ESP32-S3
VDD → + rail (3.3V)
GND → - rail
SD → GPIO 42
WS → GPIO 1
SCK → GPIO 2
L/R → - rail (GND)
```

### 4. Wire I2S Amplifier (MAX98357A)

```
MAX98357A → Breadboard → ESP32-S3
VIN → + rail (3.3V or 5V)
GND → - rail
DIN → GPIO 3
BCLK → GPIO 4
LRC → GPIO 5
SD → + rail (3.3V) [Always on]
GAIN → Float (not connected)
```

### 5. Connect Speaker

```
Speaker → MAX98357A
Red (+) → + terminal
Black (-) → - terminal
```

## Power Supply Options

### Option 1: USB-C Power (Recommended for Testing)
- Connect USB-C cable to Waveshare board
- Provides 5V power
- Easiest for development and testing

### Option 2: Battery Power (Portable)
- Waveshare board has JST battery connector
- Use 3.7V LiPo battery (500mAh - 2000mAh)
- Built-in charging circuit charges via USB-C
- **Important**: Ensure battery has protection circuit

### Option 3: External 5V Supply
- Connect to 5V and GND pins on GPIO breakout
- Use regulated 5V supply (1A minimum)
- Good for permanent installation

## Enclosure Considerations

### Minimum Enclosure Size
- **Internal dimensions**: 45mm × 70mm × 20mm (W × H × D)
- Includes space for Waveshare board + breadboard + audio modules

### Cutouts Required
1. **Display**: 39mm × 65mm front panel cutout
2. **Speaker**: 25mm diameter hole (rear or side)
3. **Microphone**: 5mm hole (top or front)
4. **USB-C**: 9mm × 4mm slot (bottom or side)
5. **Power LED**: 3mm hole (optional)

### Acoustic Considerations
- **Microphone**: Place near top edge, away from speaker
- **Speaker**: Rear-facing or front-facing grill
- **Isolation**: Use foam between mic and speaker to reduce feedback
- **Mounting**: Use rubber standoffs to reduce vibration

## Testing the Hardware

### 1. Display Test
```cpp
// In setup()
// Display should show boot screen
// Touch should respond to finger
```

### 2. Microphone Test (Loopback)
```cpp
// In config.h
audioPipeline->setMode(AudioMode::LOOPBACK);
```
- Speak into microphone
- Should hear your voice through speaker (with delay)
- Adjust volume if needed

### 3. Network Test
- Device should connect to WiFi
- Check serial output for IP address
- Should appear in Home Assistant (if configured)

## Troubleshooting

### Display Issues

**Black screen / No display**:
- Check USB-C power connection
- Verify code uploaded successfully
- Press reset button on board
- Check DISPLAY_WIDTH/HEIGHT in config.h (should be 368×448)

**Display garbled**:
- Wrong display rotation in config.h
- Incorrect library (should use Arduino_GFX with RM67162)
- Flash corruption - re-upload firmware

### Touch Issues

**Touch not responding**:
- CST816S I2C address wrong (should be 0x15)
- Touch pins not connected (built-in on Waveshare)
- Reset board

**Touch inaccurate**:
- Capacitive touch doesn't need calibration
- Check for electrical interference near display
- Update touch library

### Audio Issues

**No microphone input**:
- Check I2S mic wiring (SD, WS, SCK)
- Verify INMP441 has 3.3V power
- Check serial output for I2S init errors
- Test with loopback mode

**No speaker output**:
- Check MAX98357A wiring
- Verify SD pin is HIGH (connected to 3.3V)
- Check speaker connection polarity
- Increase GAIN setting (connect to VDD)
- Check volume setting in code

**Audio distortion**:
- Lower volume setting
- Use 5V for MAX98357A instead of 3.3V
- Check for loose wiring
- Add capacitor across power (100µF)

**Echo/Feedback**:
- Increase distance between mic and speaker
- Add acoustic foam isolation
- Lower speaker volume
- Adjust microphone sensitivity

## Pin Usage Summary

| GPIO | Function | Component |
|------|----------|-----------|
| 1 | I2S Mic WS | INMP441 |
| 2 | I2S Mic SCK | INMP441 |
| 3 | I2S Speaker DIN | MAX98357A |
| 4 | I2S Speaker BCLK | MAX98357A |
| 5 | I2S Speaker LRC | MAX98357A |
| 6-13 | Display QSPI | RM67162 (built-in) |
| 14 | Touch INT | CST816S (built-in) |
| 15 | Touch SDA | CST816S (built-in) |
| 16 | Touch SCL | CST816S (built-in) |
| 21 | Touch RST | CST816S (built-in) |
| 38 | Display BL | RM67162 (built-in) |
| 42 | I2S Mic SD | INMP441 |

## Bill of Materials (BOM)

| Item | Part Number | Qty | Est. Cost |
|------|-------------|-----|-----------|
| Waveshare ESP32-S3 1.8" AMOLED | ESP32-S3-AMOLED-1.8 | 1 | $20-25 |
| I2S MEMS Microphone | INMP441 | 1 | $3-5 |
| I2S Audio Amplifier | MAX98357A | 1 | $3-5 |
| Speaker 4Ω 3W | Generic | 1 | $2-4 |
| USB-C Cable | Generic | 1 | $2-3 |
| Jumper Wires (F-F) | Generic | 20 | $2-3 |
| Breadboard (optional) | Generic | 1 | $3-5 |
| **Total** | | | **$35-50** |

**Optional**:
- LiPo Battery 3.7V 1000mAh: $5-10
- Custom PCB: $5-15 (if ordering 5-10 pieces)
- Enclosure: $5-15

## Next Steps

1. **Assemble hardware** following wiring diagram
2. **Upload firmware** via PlatformIO: `pio run -t upload`
3. **Configure WiFi** (captive portal on first boot)
4. **Test audio** using loopback mode
5. **Set room name** via touch screen or serial
6. **Train wake word model** (see `docs/edge_impulse_guide.md`)
7. **Enjoy hands-free intercom!**

## References

- [Waveshare ESP32-S3 1.8" AMOLED Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-AMOLED-1.8)
- [INMP441 Datasheet](https://invensense.tdk.com/products/digital/inmp441/)
- [MAX98357A Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98357A-MAX98357B.pdf)
- [ESP32-S3 Pinout](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html)
