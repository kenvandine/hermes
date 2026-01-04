# Multi-Device Support Implementation

## Summary

The HERMES project now has a foundation for supporting multiple hardware devices through a Hardware Abstraction Layer (HAL). This feature branch adds the infrastructure needed to build for different ESP32-based voice assistant hardware.

## What Was Completed

### Phase 1: HAL Foundation ✅

1. **Device Configuration Headers**
   - `include/devices/waveshare/device_config.h` - Waveshare ESP32-S3 AMOLED capabilities
   - `include/devices/waveshare/device_pins.h` - Pin mappings for Waveshare
   - `include/devices/nabu_casa/device_config.h` - Nabu Casa Voice PE capabilities
   - `include/devices/nabu_casa/device_pins.h` - Pin mappings for Nabu Casa (placeholders)

2. **HAL Interface Definitions**
   - `include/hal/hal_audio.h` - Abstract audio interface
   - `include/hal/hal_display.h` - Abstract display interface
   - `include/hal/hal_controls.h` - Abstract input controls interface
   - `include/hal/hal_factory.h` - Factory for creating device-specific implementations

3. **Waveshare HAL Implementation (Stubs)**
   - `include/hal/audio/hal_audio_waveshare.h` + `.cpp`
   - `include/hal/display/hal_display_waveshare.h` + `.cpp`
   - `include/hal/controls/hal_controls_waveshare.h` + `.cpp`

4. **Nabu Casa HAL Implementation (Stubs)**
   - `include/hal/audio/hal_audio_nabu_casa.h` + `.cpp`
   - `include/hal/display/hal_display_nabu_casa.h` + `.cpp`
   - `include/hal/controls/hal_controls_nabu_casa.h` + `.cpp`

5. **HAL Factory**
   - `src/hal/hal_factory.cpp` - Device selection via compile-time flags

6. **PlatformIO Build Environments**
   - `[env:waveshare]` - Waveshare ESP32-S3 build with `-DDEVICE_WAVESHARE`
   - `[env:nabu_casa]` - Nabu Casa Voice PE build with `-DDEVICE_NABU_CASA`
   - Default environment set to `waveshare`

7. **Documentation**
   - `docs/HAL_ARCHITECTURE.md` - Comprehensive HAL design documentation
   - Migration roadmap for full HAL integration

## Project Structure Changes

```
include/
├── hal/
│   ├── hal_audio.h
│   ├── hal_display.h
│   ├── hal_controls.h
│   ├── hal_factory.h
│   ├── audio/
│   │   ├── hal_audio_waveshare.h
│   │   └── hal_audio_nabu_casa.h
│   ├── display/
│   │   ├── hal_display_waveshare.h
│   │   └── hal_display_nabu_casa.h
│   └── controls/
│       ├── hal_controls_waveshare.h
│       └── hal_controls_nabu_casa.h
└── devices/
    ├── waveshare/
    │   ├── device_config.h
    │   └── device_pins.h
    └── nabu_casa/
        ├── device_config.h
        └── device_pins.h

src/hal/
├── hal_factory.cpp
├── audio/
│   ├── hal_audio_waveshare.cpp
│   └── hal_audio_nabu_casa.cpp
├── display/
│   ├── hal_display_waveshare.cpp
│   └── hal_display_nabu_casa.cpp
└── controls/
    ├── hal_controls_waveshare.cpp
    └── hal_controls_nabu_casa.cpp

docs/
├── HAL_ARCHITECTURE.md
└── MULTI_DEVICE_SUPPORT.md (this file)
```

## Building for Different Devices

### Waveshare ESP32-S3 AMOLED (Default)
```bash
# Using virtual environment
.venv/bin/pio run -e waveshare
.venv/bin/pio run -e waveshare --target upload

# Or globally installed
pio run -e waveshare
```

### Nabu Casa Voice PE (When Ready)
```bash
.venv/bin/pio run -e nabu_casa
.venv/bin/pio run -e nabu_casa --target upload
```

## Current Status

### Working ✅
- ✅ HAL interfaces defined
- ✅ Device configurations created
- ✅ Build system configured for multi-device support
- ✅ Waveshare build compiles successfully with HAL foundation
- ✅ HAL stub implementations created for both devices
- ✅ Documentation complete

### Pending 🔄
- ⏳ Full HAL integration into `AudioPipeline` and `UIManager`
- ⏳ Nabu Casa hardware implementation (awaiting hardware)
- ⏳ Device capability-based feature adaptation

## Important Notes

### HAL Integration Status

The HAL is currently in **foundation phase** - the interfaces and structure are in place, but **not yet integrated** into the existing codebase. The Waveshare device continues to use direct hardware access for stability:

- **Audio**: `AudioPipeline` still creates `ES8311` and `I2SManager` directly
- **Display**: `UIManager` still uses `Arduino_GFX` and touch controller directly
- **Controls**: Touch events still handled by `UIManager`

This is intentional - it allows the project to build successfully while providing the foundation for future integration.

### Why Stub Implementations?

The HAL wrapper implementations are currently stubs with TODO comments because:
1. Full integration requires refactoring `AudioPipeline` and `UIManager`
2. These managers need dependency injection to receive HAL instances
3. Breaking the working Waveshare implementation during refactoring is risky
4. The stub approach provides a clean migration path

### Next Steps for Full Integration

To complete HAL integration (Phase 2):

1. **Refactor AudioPipeline**:
   ```cpp
   // Instead of:
   AudioPipeline::AudioPipeline() {
       codec_ = new ES8311();
       i2s_ = new I2SManager();
   }

   // Change to:
   AudioPipeline::AudioPipeline(HALAudio* audio)
       : audio_(audio) {
   }
   ```

2. **Refactor UIManager**:
   ```cpp
   // Instead of:
   UIManager::UIManager(...) {
       // Create display driver directly
   }

   // Change to:
   UIManager::UIManager(HALDisplay* display, HALControls* controls, ...)
       : display_(display), controls_(controls) {
   }
   ```

3. **Update main.cpp**:
   ```cpp
   // Create HAL instances
   auto audio = HALFactory::createAudio();
   auto display = HALFactory::createDisplay();
   auto controls = HALFactory::createControls();

   // Pass to managers
   audioPipeline = new AudioPipeline(audio.get());
   uiManager = new UIManager(display.get(), controls.get(), ...);
   ```

## Testing

### Waveshare Build Test
```bash
cd /home/ken/src/github/kenvandine/hermes
.venv/bin/pio run -e waveshare
```

**Result**: ✅ SUCCESS
- Build completes without errors
- Binary size: 1.93 MB (57.7% of flash)
- RAM usage: 122 KB (37.5%)

### Nabu Casa Build Test
Not yet tested (hardware-specific implementation pending)

## Branch Information

- **Branch**: `feature/multi-device-support`
- **Parent**: `main`
- **Status**: Ready for review
- **Breaking Changes**: None (HAL is additive, not replacing existing code)

## Migration Checklist for Full Integration

- [ ] Refactor `AudioPipeline` to accept `HALAudio*` in constructor
- [ ] Refactor `UIManager` to accept `HALDisplay*` and `HALControls*`
- [ ] Update `main.cpp` to use `HALFactory::create*()` methods
- [ ] Implement conditional features based on device capabilities
- [ ] Test Waveshare build with full HAL integration
- [ ] Implement Nabu Casa hardware drivers
- [ ] Test Nabu Casa build on actual hardware
- [ ] Update documentation with working examples

## Contributing

To add support for a new device:
1. Create device config headers in `include/devices/yourdevice/`
2. Implement HAL interfaces in `include/hal/*/hal_*_yourdevice.h`
3. Add device case to `HALFactory`
4. Add PlatformIO environment with `-DDEVICE_YOURDEVICE`
5. Test build: `.venv/bin/pio run -e yourdevice`

## References

- `docs/HAL_ARCHITECTURE.md` - Detailed architecture documentation
- Waveshare ESP32-S3: https://www.waveshare.com/esp32-s3-touch-amoled-1.8.htm
- Nabu Casa Voice PE: https://www.home-assistant.io/voice-pe/
