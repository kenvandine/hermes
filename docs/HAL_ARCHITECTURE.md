# Hardware Abstraction Layer (HAL) Architecture

## Overview

The HERMES project now supports multiple hardware devices through a Hardware Abstraction Layer (HAL). This allows the same codebase to run on different ESP32-based voice assistant hardware with minimal changes.

## Supported Devices

### Waveshare ESP32-S3-Touch-AMOLED-1.8" (Current)
- **Status**: Fully implemented and tested
- **Audio**: ES8311 codec, 16kHz mono
- **Display**: 1.8" AMOLED (368x448) with LVGL UI
- **Input**: FT3168 capacitive touch screen
- **Build Environment**: `waveshare`

### Nabu Casa Voice Preview Edition (Planned)
- **Status**: HAL stubs created, awaiting hardware
- **Audio**: XMOS XU316 + TI AIC3204, 48kHz stereo, dual microphones
- **Display**: 12-LED WS2812B ring (no screen)
- **Input**: Physical button, rotary encoder, mute switch
- **Build Environment**: `nabu_casa`

## HAL Structure

### Directory Organization

```
include/
├── hal/
│   ├── hal_audio.h          # Abstract audio interface
│   ├── hal_display.h        # Abstract display interface
│   ├── hal_controls.h       # Abstract input controls interface
│   ├── hal_factory.h        # Factory for creating device-specific HAL instances
│   ├── audio/
│   │   ├── hal_audio_waveshare.h
│   │   └── hal_audio_nabu_casa.h
│   ├── display/
│   │   ├── hal_display_waveshare.h
│   │   └── hal_display_nabu_casa.h
│   └── controls/
│       ├── hal_controls_waveshare.h
│       └── hal_controls_nabu_casa.h
├── devices/
│   ├── waveshare/
│   │   ├── device_config.h   # Device capabilities and config
│   │   └── device_pins.h     # Pin definitions
│   └── nabu_casa/
│       ├── device_config.h
│       └── device_pins.h

src/
├── hal/
│   ├── hal_factory.cpp
│   ├── audio/
│   │   ├── hal_audio_waveshare.cpp
│   │   └── hal_audio_nabu_casa.cpp
│   ├── display/
│   │   ├── hal_display_waveshare.cpp
│   │   └── hal_display_nabu_casa.cpp
│   └── controls/
│       ├── hal_controls_waveshare.cpp
│       └── hal_controls_nabu_casa.cpp
```

### Abstract Interfaces

#### HALAudio
```cpp
class HALAudio {
    virtual bool begin(uint32_t sampleRate) = 0;
    virtual size_t readMicrophone(int16_t* buffer, size_t sampleCount) = 0;
    virtual size_t writeSpeaker(const int16_t* buffer, size_t sampleCount) = 0;
    virtual void setVolume(uint8_t volume) = 0;
    virtual void setMicGain(uint8_t gainStep) = 0;
    // ... and more
};
```

#### HALDisplay
```cpp
class HALDisplay {
    virtual bool begin() = 0;
    virtual void showState(AppState state) = 0;
    virtual void showMessage(const String& message, uint32_t durationMs) = 0;
    virtual DisplayCapabilities getCapabilities() const = 0;
    // ... and more
};
```

#### HALControls
```cpp
class HALControls {
    typedef std::function<void(const ControlEventData&)> ControlCallback;

    virtual bool begin() = 0;
    virtual void update() = 0;
    virtual void registerCallback(ControlCallback callback) = 0;
    virtual bool hasTouchScreen() const = 0;
    virtual bool hasPhysicalControls() const = 0;
    // ... and more
};
```

### HAL Factory

The `HALFactory` class creates device-specific implementations based on compile-time build flags:

```cpp
// In your setup code:
auto audio = HALFactory::createAudio();
auto display = HALFactory::createDisplay();
auto controls = HALFactory::createControls();

audio->begin(16000);
display->begin();
controls->begin();
```

The factory automatically selects the correct implementation based on `DEVICE_WAVESHARE` or `DEVICE_NABU_CASA` build flags.

## Building for Different Devices

### Waveshare ESP32-S3 AMOLED
```bash
pio run -e waveshare
pio run -e waveshare --target upload
```

### Nabu Casa Voice PE (when ready)
```bash
pio run -e nabu_casa
pio run -e nabu_casa --target upload
```

## Build Flags

The device is selected via PlatformIO build flags in `platformio.ini`:

- **waveshare environment**: `-DDEVICE_WAVESHARE`
- **nabu_casa environment**: `-DDEVICE_NABU_CASA`

These flags control which HAL implementations are compiled and linked.

## Migration Status

### Phase 1: Foundation (Complete)
- [x] Create HAL interfaces
- [x] Create device configuration headers
- [x] Implement Waveshare HAL wrappers
- [x] Create Nabu Casa HAL stubs
- [x] Set up HAL factory
- [x] Configure PlatformIO build environments

### Phase 2: Integration (Planned)
- [ ] Refactor `AudioPipeline` to use `HALAudio`
- [ ] Refactor `UIManager` to use `HALDisplay` and `HALControls`
- [ ] Update `main.cpp` to instantiate HAL via factory
- [ ] Test Waveshare build with HAL integration

### Phase 3: Nabu Casa Implementation (Awaiting Hardware)
- [ ] Implement XMOS XU316 audio processor communication
- [ ] Implement TI AIC3204 codec driver
- [ ] Implement WS2812B LED ring driver
- [ ] Implement physical controls (button, rotary encoder, mute switch)
- [ ] Test Nabu Casa build on actual hardware

### Phase 4: Conditional Features (Future)
- [ ] Adapt features based on device capabilities
  - Full UI vs LED-only feedback
  - Touch vs physical controls
  - Mono vs stereo audio processing
  - Sample rate differences (16kHz vs 48kHz)

## Device Capabilities

Device capabilities are queried at runtime to enable/disable features:

```cpp
auto display = HALFactory::createDisplay();
DisplayCapabilities caps = display->getCapabilities();

if (caps.hasFullUI) {
    // Show LVGL UI
} else if (caps.hasLEDRing) {
    // Show LED patterns
}
```

## Example: Adding a New Device

To add support for a new device (e.g., "MyDevice"):

1. **Create configuration headers**:
   - `include/devices/mydevice/device_config.h` - Define capabilities
   - `include/devices/mydevice/device_pins.h` - Define pin mappings

2. **Implement HAL interfaces**:
   - `include/hal/audio/hal_audio_mydevice.h` and `.cpp`
   - `include/hal/display/hal_display_mydevice.h` and `.cpp`
   - `include/hal/controls/hal_controls_mydevice.h` and `.cpp`

3. **Update HAL factory** (`src/hal/hal_factory.cpp`):
   ```cpp
   #elif defined(DEVICE_MYDEVICE)
       #include "hal/audio/hal_audio_mydevice.h"
       // ... other includes
   ```

4. **Add PlatformIO environment** in `platformio.ini`:
   ```ini
   [env:mydevice]
   build_flags = -DDEVICE_MYDEVICE
   ```

5. **Build and test**:
   ```bash
   pio run -e mydevice
   ```

## Notes

- The HAL is currently in **foundation phase** - interfaces are defined but not yet fully integrated
- Waveshare device continues to use direct hardware access (ES8311, UIManager) for stability
- Full HAL integration will be completed in Phase 2
- Nabu Casa implementation awaits hardware availability

## References

- Waveshare ESP32-S3-Touch-AMOLED-1.8": https://www.waveshare.com/esp32-s3-touch-amoled-1.8.htm
- Nabu Casa Voice PE: https://www.home-assistant.io/voice-pe/
- PlatformIO Build Flags: https://docs.platformio.org/en/latest/projectconf/build_configurations.html
