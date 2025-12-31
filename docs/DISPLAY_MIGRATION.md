# Display Migration: Waveshare ESP32-S3 1.8" AMOLED

This document summarizes the changes made to optimize the ESP32 Intercom System for the **Waveshare ESP32-S3 1.8" AMOLED Touch Display** (368×448).

## Hardware Changes

### Original Configuration
- **Display**: Generic 4.3" TFT LCD (480×272 landscape)
- **Controller**: Various (via TFT_eSPI library)
- **Touch**: Resistive touch (SPI)
- **ESP32 Board**: Separate ESP32-S3 DevKit
- **Orientation**: Landscape

### New Configuration
- **Display**: Waveshare ESP32-S3 1.8" AMOLED (368×448 portrait)
- **Controller**: RM67162 AMOLED (QSPI interface)
- **Touch**: CST816S capacitive touch (I2C)
- **ESP32 Board**: Integrated on Waveshare board
- **Orientation**: Portrait

## Code Changes

### 1. Pin Configuration (`include/config.h`)

#### Display Pins (Updated)
```cpp
// RM67162 AMOLED controller (QSPI)
#define TFT_CS      10
#define TFT_DC      11
#define TFT_RST     13
#define TFT_SDA0    9
#define TFT_SDA1    8
#define TFT_SDA2    7
#define TFT_SDA3    6
#define TFT_SCL     12
#define TFT_BL      38
```

#### Touch Pins (Updated)
```cpp
// CST816S capacitive touch (I2C)
#define TOUCH_SDA   15
#define TOUCH_SCL   16
#define TOUCH_RST   21
#define TOUCH_IRQ   14
```

#### Audio Pins (Relocated)
Due to display/touch using GPIOs 6-16, audio pins were moved:

**Before**:
```cpp
#define I2S_MIC_SCK_PIN     26
#define I2S_MIC_WS_PIN      25
#define I2S_MIC_SD_PIN      33
#define I2S_SPK_SCK_PIN     14  // CONFLICT with touch!
#define I2S_SPK_WS_PIN      12  // CONFLICT with display!
#define I2S_SPK_SD_PIN      13  // CONFLICT with display!
```

**After**:
```cpp
#define I2S_MIC_SCK_PIN     2
#define I2S_MIC_WS_PIN      1
#define I2S_MIC_SD_PIN      42
#define I2S_SPK_SCK_PIN     4
#define I2S_SPK_WS_PIN      5
#define I2S_SPK_SD_PIN      3
```

### 2. Display Configuration (`include/config.h`)

#### Resolution (Updated)
```cpp
#define DISPLAY_WIDTH       368    // Was: 480
#define DISPLAY_HEIGHT      448    // Was: 272
#define DISPLAY_ROTATION    0      // Portrait (was: 1 for landscape)
```

#### UI Layout Constants (Added)
Optimized for smaller, portrait display:
```cpp
#define UI_MARGIN           10
#define UI_BUTTON_HEIGHT    60
#define UI_BUTTON_SPACING   15
#define UI_HEADER_HEIGHT    50
#define UI_FOOTER_HEIGHT    80
#define UI_ICON_SIZE        48
#define UI_FONT_SIZE_HEADER 24
#define UI_FONT_SIZE_NORMAL 18
#define UI_FONT_SIZE_SMALL  14
```

#### AMOLED Color Optimization (Added)
Using true black to save AMOLED power:
```cpp
#define UI_COLOR_BACKGROUND 0x0000  // True black
#define UI_COLOR_PRIMARY    0x07E0  // Green
#define UI_COLOR_TEXT       0xFFFF  // White
// ... other colors
```

#### Touch Configuration (Updated)
```cpp
#define TOUCH_I2C_ADDRESS         0x15    // CST816S I2C address
#define TOUCH_CALIBRATION_ENABLED false   // Capacitive doesn't need calibration
```

### 3. Library Dependencies (`platformio.ini`)

#### Before:
```ini
bodmer/TFT_eSPI@^2.5.43  # Generic TFT library
```

#### After:
```ini
moononournation/GFX Library for Arduino@^1.4.7  # Supports RM67162
https://github.com/lewisxhe/SensorLib.git       # CST816S touch
```

**Why**: RM67162 AMOLED controller uses QSPI interface not supported by TFT_eSPI.

## UI Layout Implications

### Screen Real Estate Comparison

| Aspect | Old Display | New Display | Impact |
|--------|-------------|-------------|--------|
| Resolution | 480×272 | 368×448 | 35% more pixels vertically |
| Orientation | Landscape | Portrait | Better for lists, worse for wide content |
| Width | 480px | 368px | 23% narrower |
| Height | 272px | 448px | 65% taller |
| Aspect Ratio | 16:9 | 9:11 | Portrait vs landscape |

### UI Adaptation Recommendations

#### Ideal for Portrait Layout:
✅ **Vertical lists** (device list, call history)
✅ **Stacked buttons** (Accept/Reject, Hangup)
✅ **Status indicators** (calling, active call)
✅ **Scrollable content** (more vertical space)

#### Challenges:
⚠️ **Wide text** (room names, status messages may wrap)
⚠️ **Horizontal buttons** (less horizontal space)
⚠️ **Multi-column layouts** (narrow width limits columns)

### Suggested UI Layouts

#### Idle Screen (Home)
```
┌──────────────────────┐
│   Kitchen Intercom   │ ← Header (50px)
├──────────────────────┤
│                      │
│  [Living Room]   ●  │ ← Device buttons (60px each)
│  [Bedroom]       ●  │
│  [Office]        ○  │
│  [Garage]        ○  │
│                      │
│                      │ ← Scrollable area
│                      │
│                      │
├──────────────────────┤
│  [Settings]          │ ← Footer (80px)
│  WiFi: ████ | 12:34  │
└──────────────────────┘
```

#### Active Call Screen
```
┌──────────────────────┐
│   Calling...         │ ← Header
├──────────────────────┤
│                      │
│     ┌────────┐       │
│     │        │       │
│     │  [🏠]  │       │ ← Large room icon
│     │        │       │
│     └────────┘       │
│                      │
│   Living Room        │ ← Room name
│                      │
│   ●●●●●●●●           │ ← Audio levels
│                      │
│   00:45              │ ← Call duration
│                      │
├──────────────────────┤
│   [  Hang Up  ]      │ ← Large button
├──────────────────────┤
│   Mute | Vol: 80%    │ ← Footer
└──────────────────────┘
```

## Performance Considerations

### AMOLED Power Optimization

**True Black Backgrounds**:
- AMOLED pixels are OFF when displaying black (0x0000)
- Saves significant power vs always-on LCD backlight
- UI uses `UI_COLOR_BACKGROUND = 0x0000`

**Brightness**:
- Default brightness reduced to 180 (from 200)
- AMOLED doesn't need high brightness in most conditions
- Further reduction possible in dark rooms

**Static Content**:
- Minimize frequent full-screen updates
- Use partial updates where possible
- AMOLED has fast response time (no ghosting)

### Memory Usage

**Smaller Display = Less VRAM**:
- Old: 480×272 = 130,560 pixels
- New: 368×448 = 164,864 pixels
- **+26% more pixels** but still manageable with PSRAM

**LVGL Buffer**:
- Recommend 1/10 screen buffer: ~16KB
- ESP32-S3 has 8MB PSRAM - plenty of room

## Migration Checklist

For developers adapting this code:

- [x] Update pin definitions in `include/config.h`
- [x] Update display resolution (368×448)
- [x] Change display orientation to portrait (0)
- [x] Update library dependencies (Arduino_GFX, SensorLib)
- [x] Add UI layout constants for portrait design
- [x] Optimize colors for AMOLED (true black)
- [x] Relocate I2S audio pins (avoid 6-16)
- [x] Update hardware documentation
- [x] Create wiring diagram for new board
- [ ] Implement UI screens (Phase 7 - future)
- [ ] Test touch calibration (capacitive)
- [ ] Optimize for battery life (AMOLED low power)

## Testing Recommendations

### Display Testing
1. **Basic display test**: Show solid colors (R, G, B, W, Black)
2. **Touch test**: Draw circles at touch points
3. **Text rendering**: Various sizes and colors
4. **Rotation test**: Verify portrait orientation
5. **Brightness test**: PWM backlight control

### Audio Testing
1. **Loopback test**: Mic → Speaker (verify new pin assignments)
2. **Network audio**: UDP audio streaming
3. **Interference test**: Audio during display updates
4. **Volume test**: MAX98357A GAIN settings

### Integration Testing
1. **Wake word**: Edge Impulse model with new hardware
2. **Touch + audio**: Ensure no electrical interference
3. **Battery test**: Power consumption with AMOLED
4. **Thermal test**: ESP32-S3 doesn't overheat in enclosure

## Known Issues & Solutions

### Issue 1: QSPI Display Initialization
**Problem**: RM67162 requires specific initialization sequence
**Solution**: Use moononournation/GFX Library with RM67162 support

### Issue 2: Touch I2C Communication
**Problem**: CST816S may not respond immediately after power-on
**Solution**: Add delay after touch reset, retry I2C init

### Issue 3: Audio Pin Conflicts
**Problem**: Original I2S pins conflict with display
**Solution**: Relocated to GPIOs 1-5, 42 (verified available)

### Issue 4: Narrow Display Width
**Problem**: Long room names may not fit horizontally
**Solution**: Use text wrapping or abbreviations, vertical scrolling

## Future Enhancements

### Display-Specific Features
1. **AMOLED Burn-In Prevention**:
   - Screensaver after 5 min idle
   - Shift UI elements slightly
   - Reduce brightness when idle

2. **Gesture Support**:
   - CST816S supports gestures (swipe, double-tap)
   - Could enable: swipe to scroll devices, double-tap to call

3. **Always-On Display**:
   - Low-power mode showing time and status
   - Very low power with mostly-black AMOLED

4. **Portrait-Optimized Animations**:
   - Vertical slide transitions
   - Bottom-to-top call notifications

## Conclusion

The migration to Waveshare ESP32-S3 1.8" AMOLED provides:

**Advantages**:
- ✅ All-in-one integrated board (simpler assembly)
- ✅ Higher pixel density (better text clarity)
- ✅ AMOLED power efficiency (true black saves power)
- ✅ Capacitive touch (better accuracy, no calibration)
- ✅ Portrait orientation (better for lists)
- ✅ Built-in battery support
- ✅ Smaller form factor (easier to mount)

**Trade-offs**:
- ⚠️ Smaller screen (less content visible at once)
- ⚠️ Different libraries needed (RM67162 not in TFT_eSPI)
- ⚠️ Pin constraints (fewer available GPIOs)
- ⚠️ Portrait layout needs different UI design

**Overall**: The Waveshare ESP32-S3 1.8" AMOLED is well-suited for this intercom project, offering a compact, integrated solution with excellent display quality and touch responsiveness.
