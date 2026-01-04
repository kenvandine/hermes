# HERMES Audio Hardware Test

This test program helps you verify that your microphone and speaker are working correctly.

## Hardware Connections

### Microphone (INMP441 or compatible I2S MEMS microphone)

| INMP441 Pin | ESP32-S3 GPIO | Description |
|-------------|---------------|-------------|
| SCK (BCLK)  | GPIO 1        | Bit Clock |
| WS (LRCLK)  | GPIO 2        | Word Select |
| SD (DOUT)   | GPIO 42       | Serial Data |
| VDD         | 3.3V          | Power |
| GND         | GND           | Ground |
| L/R         | GND           | Left channel |

### Speaker Amplifier (ES8311 or MAX98357A)

**Note:** The Waveshare board has an onboard ES8311 codec. If using external MAX98357A:

| Amplifier Pin | ESP32-S3 GPIO | Description |
|---------------|---------------|-------------|
| BCLK (SCK)    | GPIO 9        | Bit Clock |
| LRCLK (WS)    | GPIO 45       | Word Select |
| DIN (SD)      | GPIO 10       | Serial Data |
| VDD           | 3.3V or 5V    | Power |
| GND           | GND           | Ground |
| GAIN          | GND or NC     | Gain setting |

Connect a 4-8Ω speaker to the amplifier output.

## Building and Uploading

```bash
# Build the test program
.venv/bin/pio run -e test_audio

# Upload to device
.venv/bin/pio run -e test_audio --target upload

# Monitor serial output
.venv/bin/pio device monitor
```

Or use the shorthand if `pio` is in your PATH:
```bash
pio run -e test_audio --target upload && pio device monitor
```

## Running Tests

After uploading, open the serial monitor (115200 baud). You'll see a menu:

```
========================================
  HERMES Audio Test Menu
========================================
1 - Test Microphone (show audio levels)
2 - Test Speaker (play 440 Hz tone)
3 - Test Loopback (mic → speaker)
4 - Run All Tests
5 - Show Menu
========================================
```

### Test 1: Microphone

- Reads audio from the microphone for 5 seconds
- Displays audio level in real-time as a bar graph
- **What to expect**:
  - Quiet room: Small bar, low RMS values
  - Speaking/music: Larger bar, higher RMS values
- **Troubleshooting**:
  - No bar movement: Check microphone wiring
  - Always max level: Check for short circuit or incorrect pins

### Test 2: Speaker

- Plays a 440 Hz (A4 musical note) tone for 1 second
- **What to expect**:
  - Clear, steady tone from speaker
  - No distortion or crackling
- **Troubleshooting**:
  - No sound: Check speaker wiring and amplifier power
  - Distorted sound: Check volume/gain settings, ensure proper power supply
  - Crackling: Check for loose connections

### Test 3: Loopback

- Records from microphone and immediately plays to speaker
- Runs for 10 seconds
- **What to expect**:
  - Hear your voice with ~50-100ms delay
  - Some echo/feedback is normal
- **Troubleshooting**:
  - No loopback: One or both devices not working
  - Excessive feedback: Reduce speaker volume or increase distance between mic and speaker

### Test 4: Run All Tests

Runs tests 1, 2, and 3 in sequence with 1-second pauses between them.

## Expected Output Examples

### Microphone Test
```
========================================
  MICROPHONE TEST
========================================
Reading microphone for 5 seconds...
Speak into the microphone!

Level: [████████              ] 842
Level: [█████████████         ] 1305
Level: [██████                ] 623
Level: [███                   ] 287

✓ Microphone test complete
```

### Speaker Test
```
========================================
  SPEAKER TEST
========================================
Playing 440 Hz tone for 1000 ms...

✓ Speaker test complete
```

## Troubleshooting

### No Audio from Microphone

1. **Check wiring**:
   - Verify all connections match the pin table above
   - Ensure GND and VDD are connected
   - Check that L/R pin is connected to GND for left channel

2. **Check power**:
   - INMP441 requires 3.3V (not 5V!)
   - Measure voltage at VDD pin

3. **Check serial output**:
   - Look for "[MIC] ✓ Microphone initialized" message
   - If you see errors, note the error code

### No Audio from Speaker

1. **Check wiring**:
   - Verify all connections match the pin table above
   - Ensure speaker is properly connected to amplifier output

2. **Check power**:
   - MAX98357A can use 3.3V or 5V (5V gives more power)
   - ES8311 uses 3.3V
   - Check if amplifier LED is lit (if present)

3. **Check volume**:
   - Some amplifiers have gain settings
   - Try adjusting GAIN pin (connect to VDD for higher gain)

4. **Check serial output**:
   - Look for "[SPK] ✓ Speaker initialized" message
   - If you see errors, note the error code

### I2S Error Codes

Common ESP32 I2S error codes:
- `257` (0x101): ESP_ERR_INVALID_ARG - Check pin numbers
- `261` (0x105): ESP_ERR_INVALID_STATE - I2S already initialized
- `4354` (0x1102): ESP_ERR_NO_MEM - Not enough memory

### Audio Quality Issues

1. **Crackling/Popping**:
   - Increase DMA buffer size (edit `DMA_BUF_LEN` in test_audio.cpp)
   - Check for electrical interference
   - Use shorter wires
   - Add bypass capacitors near power pins

2. **Low Volume**:
   - Increase gain setting on amplifier
   - Check speaker impedance (4-8Ω recommended)
   - Verify power supply can deliver enough current

3. **Distortion**:
   - Reduce volume/gain
   - Check for clipping (RMS values > 30000)
   - Ensure clean power supply

## Hardware Specifications

### Sample Rate
- 16 kHz (16,000 samples/second)
- Good for voice communication
- Can be changed in test_audio.cpp

### Bit Depth
- 16-bit signed samples
- Range: -32768 to +32767

### DMA Settings
- 8 buffers of 512 samples each
- Total buffer time: ~256ms

## Next Steps

After successful audio testing:

1. **Return to main firmware**: Upload the main HERMES firmware
   ```bash
   ./build.sh upload
   ```

2. **Enable audio features**: The main firmware currently has audio disabled
   - Remove `-DDISABLE_AUDIO_TEMP` from platformio.ini
   - Rebuild the main firmware

3. **Test with actual calls**: Try the intercom functionality

## Additional Notes

- This test program bypasses all other HERMES features (MQTT, UI, etc.)
- It's a minimal test to verify hardware only
- The main firmware uses the same I2S configuration
- If this test works, audio should work in the main firmware too
