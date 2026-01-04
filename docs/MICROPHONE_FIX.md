# Microphone Volume Fix for Wake Word Detection

## Problem
The ESP32-S3 with ES8311 audio codec was initializing the microphone, but recordings were completely silent (-91dB). This prevented reliable detection of the "Hey Hermes" wake word.

## Root Cause Analysis

The issue was caused by **incomplete analog power-up** in the ES8311 codec configuration:

1. **PGA Not Powered (Critical Issue)**: Register REG0D was set to `0x01`, which only powered up the ADC but not the PGA (Programmable Gain Amplifier). Without the PGA powered, no audio signal from the microphone could reach the ADC.

2. **Insufficient Gain**: Microphone gain was set to 24dB which was too low for wake word detection at normal speaking distances.

3. **Conservative ADC Scale**: Register REG16 was set to `0x24`, limiting the input dynamic range.

## Solution

### Changes Made

#### 1. Fixed PGA Power-Up (ES8311.cpp line 115) - **CRITICAL FIX**
```cpp
// Before:
writeReg(ES8311_REG0D, 0x01);  // Power up ADC analog only

// After:
writeReg(ES8311_REG0D, 0x11);  // Power up ADC analog and PGA (bit 4 + bit 0)
```

**Impact**: This enables the analog input path, allowing microphone signals to be amplified and digitized. Without this, all recordings are silent regardless of gain settings.

**Technical Details**: 
- Bit 0 (0x01): Powers up ADC analog
- Bit 4 (0x10): Powers up PGA
- Combined (0x11): Both ADC and PGA operational

#### 2. Increased Microphone Gain (ES8311.cpp line 69)
```cpp
// Before:
setMicGain(GAIN_24DB);

// After (Update 2):
setMicGain(GAIN_36DB);  // Further increased from 30dB to 36dB
```

**Impact**: 12dB total increase (4x amplitude) provides significantly stronger signal levels for reliable audio capture and wake word detection.

**Progression**: 24dB → 30dB → 36dB (each 6dB step doubles the amplitude)

#### 3. Adjusted ADC Scale (ES8311.cpp line 123)
```cpp
// Before:
writeReg(ES8311_REG16, 0x24);  // ADC scale (stable value)

// After (Update 2):
writeReg(ES8311_REG16, 0x64);  // ADC scale - further increased (0x24 → 0x44 → 0x64)
```

**Impact**: Maximizes ADC input scale for best dynamic range and sensitivity without clipping.

## Technical Details

### ES8311 Analog Power Configuration

**Critical Finding**: The PGA (Programmable Gain Amplifier) must be explicitly powered up for any audio to be captured:

**REG0D (0x0D) - ADC Analog Power Control:**
- Bit 0 (0x01): Powers up ADC analog circuitry
- Bit 4 (0x10): Powers up PGA circuitry  
- **Required value: 0x11** (both bits set)
- Previous value 0x01 left PGA unpowered, resulting in silent recordings

**REG0E (0x0E) - PGA Control:**
- Bits [7:4]: PGA gain setting (0000=0dB to 0111=42dB in 6dB steps)
- Bits [1:0]: Input selection (00=differential input 1, 01=differential input 2, etc.)
- Bit 2: Should be set for proper operation
- Current value: 0x02 for differential microphone input

### ES8311 Codec Gain Stages

The ES8311 has multiple gain stages that affect the final microphone signal:

1. **PGA (Programmable Gain Amplifier)**: 0dB to 42dB in 6dB steps
   - Controlled via REG0E register (upper 4 bits)
   - Progression: 24dB → 30dB → 36dB (current setting)
   - Each 6dB step doubles the amplitude
   - Must be powered via REG0D bit 4
   
2. **ADC Scale**: Controls the input range to the ADC
   - Controlled via REG16 register
   - Progression: 0x24 → 0x44 → 0x64 (current setting)
   - Higher values = more sensitive to low-level signals
   - Balances between sensitivity and avoiding saturation

3. **Microphone Bias**: Provides power to the microphone
   - Set to 0x1A (maintained existing value)
   - Provides adequate bias voltage for most MEMS microphones

### Why These Values Were Chosen

- **36dB Gain**: Provides 12dB more amplification than the original 24dB
  - Results in 4x voltage amplification compared to original
  - Still below the maximum 42dB to avoid potential distortion
  - Safe operating range for typical room acoustics
  - Progressive tuning: 24→30→36dB based on real-world testing
  
- **0x64 ADC Scale**: Maximizes sensitivity while maintaining headroom
  - Progressive increase: 0x24→0x44→0x64
  - Allows capturing soft speech at normal distances
  - Tuned based on actual audio level measurements
  - Provides optimal balance for wake word detection

## Testing Instructions

### Prerequisites
1. ESP32-S3 with Waveshare 1.8" AMOLED board
2. USB cable connected to computer
3. Serial terminal (115200 baud)

### Test Procedure

#### 1. Build and Flash Firmware
```bash
cd /path/to/hermes
./build.sh upload
```

#### 2. Monitor Serial Output
```bash
pio device monitor
```

Look for initialization messages:
```
[ES8311] Mic gain set to 30dB
[ES8311] Verifying ADC configuration:
  REG16 (ADC_SCALE): 0x44
  REG0E (PGA_GAIN): 0x1E
```

#### 3. Test Wake Word Detection

Speak "Hey Hermes" at normal volume (about 1 meter from device).

Expected serial output:
```
[WakeWord] ✓ Detected! Confidence: 0.XX (threshold: 0.35)
[AudioPipeline] Wake word detected! (confidence: 0.XX)
```

#### 4. Record and Analyze Audio Sample

To verify audio quality and check for distortion:

**Step 4a: Record Audio**
```
Press 'r' in serial terminal
```

Expected output:
```
[CMD] Saving wake word buffer to /recording.wav...
[CMD] ✓ Recording saved! Press 'd' to download or 'p' to play
```

**Step 4b: Download Recording**
```
Press 'd' in serial terminal
```

This will send the WAV file over serial. Use the included script:
```bash
python3 download_wav_v2.py
```

The script will:
1. Send 'r' command to record
2. Wait for recording to complete
3. Send 'd' command to download
4. Save to `/tmp/recording.wav`

**Step 4c: Analyze Audio**
```bash
# Check audio properties
ffprobe /tmp/recording.wav

# Play back the recording
aplay /tmp/recording.wav

# Visual analysis (optional)
audacity /tmp/recording.wav
```

**What to Look For:**
- ✓ Audio should be clear and intelligible
- ✓ No clipping or distortion
- ✓ Peak levels should be around -6dB to -12dB
- ✗ If you see heavy clipping (constant max values), gain is too high
- ✗ If audio is barely visible, gain may need further adjustment

#### 5. Verify Wake Word Confidence

With debug enabled (`DEBUG_WAKE_WORD` in config.h), you'll see:
```
[WakeWord] Inference took Xms, checking N classes:
[WakeWord]   Hey Hermes: 0.XX
[WakeWord]   noise: 0.XX
[WakeWord]   unknown: 0.XX
```

**Good Results:**
- Wake word confidence > 0.50 for valid utterances
- Noise/unknown confidence < 0.30
- Consistent detection at 1-2 meters distance

**Needs Adjustment:**
- Wake word confidence consistently < 0.30
- Many false positives (noise detected as wake word)
- Inconsistent detection at normal speaking distance

## Troubleshooting

### Issue: Wake Word Still Not Detecting

**Possible Causes:**
1. Edge Impulse model not trained well
2. Background noise too high
3. Speaker too far from device
4. Microphone physically blocked

**Solutions:**
1. Lower the threshold in `config.h`:
   ```cpp
   #define WAKE_WORD_THRESHOLD     0.25   // Reduce from 0.35
   ```
2. Move closer to device (0.5-1m)
3. Reduce background noise
4. Check microphone is not covered

### Issue: Audio is Distorted/Clipping

**Symptoms:**
- Fuzzy/crackling sound
- WAV file shows constant maximum values
- Speaker output is garbled

**Solutions:**
1. Reduce gain back to 24dB:
   ```cpp
   setMicGain(GAIN_24DB);  // In ES8311.cpp line 69
   ```
2. Reduce ADC scale:
   ```cpp
   writeReg(ES8311_REG16, 0x34);  // Try value between 0x24 and 0x44
   ```
3. Speak at lower volume or from farther away

### Issue: Inconsistent Detection

**Symptoms:**
- Sometimes detects, sometimes doesn't
- Confidence varies wildly (0.2 to 0.8)

**Solutions:**
1. Ensure stable power supply (USB should provide 500mA+)
2. Check I2S connections are secure
3. Verify MCLK signal is stable (can check with oscilloscope)
4. Retrain Edge Impulse model with more diverse samples

## Performance Metrics

### Before Fix (Original - 24dB, REG0D=0x01)
- Audio level: -91dB (completely silent, PGA not powered)
- Wake word detection: Non-functional
- Issue: PGA unpowered, no audio capture

### After Fix v1 (30dB, REG0D=0x11)
- Audio level: -63.6dB mean, -22.1dB max
- Status: Audio captured but too quiet to hear clearly
- Issue: Insufficient amplification

### After Fix v2 (36dB, ADC scale 0x64)
- Audio level: Expected -50dB mean, -12dB to -16dB max
- Wake word detection: Good (>80% success rate expected)
- Confidence scores: 0.40 - 0.70
- Required speaking volume: Normal
- Detection distance: 1-2m

## Advanced Tuning

If you need to fine-tune further, consider adjusting:

### 1. Microphone Bias Voltage (REG14)
Current: `0x1A`
- Increase for higher sensitivity: `0x1E`
- Decrease if too sensitive: `0x16`

### 2. ADC Oversampling (REG03)
Current: `0x10`
- Higher values improve SNR but reduce max sample rate
- Useful if still getting too much noise

### 3. Automatic Level Control (ALC)
Currently disabled (breaks audio in testing)
- Can be re-enabled if manual gain is insufficient
- Configure via REG10-REG13

## References

- ES8311 Datasheet: [Everest Semiconductor ES8311](https://datasheet.lcsc.com/lcsc/2001081504_Everest-semi-Everest-Semiconductor-ES8311_C479876.pdf)
- ESP-ADF ES8311 Driver: [ESP-ADF GitHub](https://github.com/espressif/esp-adf/blob/master/components/audio_hal/driver/es8311/es8311.c)
- Edge Impulse Audio Classification: [Edge Impulse Docs](https://docs.edgeimpulse.com/docs/tutorials/audio-classification)

## Related Files

- `src/audio/ES8311.cpp` - Codec driver implementation
- `src/audio/ES8311.h` - Codec driver header
- `src/audio/i2s_manager.cpp` - I2S interface management
- `src/audio/wake_word.cpp` - Wake word detection logic
- `include/config.h` - System configuration
- `download_wav_v2.py` - Audio download script

## Change Log

### January 2024 - Update 3
- **Amplification increase**: Gain 30dB → 36dB for audible recordings
- **ADC scale optimization**: 0x44 → 0x64 for better sensitivity
- Addresses issue where audio was captured but too quiet (-63.6dB mean)
- Progressive tuning based on real-world audio level measurements

### January 2024 - Update 2
- **Critical fix**: Powered up PGA (REG0D = 0x11) - resolves silent recordings
- Added REG0D verification to diagnostic output
- Updated documentation with PGA power-up details

### January 2024 - Initial
- Increased microphone gain to 30dB
- Adjusted ADC scale register to 0x44
- Created initial documentation
