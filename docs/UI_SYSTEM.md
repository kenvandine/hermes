# UI System Documentation

Complete guide to the LVGL-based UI system for the ESP32 Intercom (368×448 portrait AMOLED).

## Overview

The UI system uses **LVGL 8.3** (Light and Versatile Graphics Library) optimized for the Waveshare ESP32-S3 1.8" AMOLED display. The design follows a screen-based architecture where each application state has a corresponding screen.

## Architecture

```
UIManager (Coordinator)
├── Screen Idle (Home/Dashboard)
├── Screen Listening (Voice command mode)
├── Screen Calling (Outgoing call)
├── Screen Ringing (Incoming call)
├── Screen Active (During call)
├── Screen Settings (Configuration)
└── Screen Error (Error messages)
```

### State → Screen Mapping

| AppState | Screen | Description |
|----------|--------|-------------|
| IDLE | screen_idle | Device list, status bar |
| LISTENING | screen_listening | Voice command help |
| CALLING | screen_calling | Outgoing call progress |
| RINGING | screen_ringing | Incoming call (Accept/Reject) |
| ACTIVE_CALL | screen_active | Call in progress, audio levels |
| HANGING_UP | screen_error | Brief "Ending call..." message |
| ERROR | screen_error | Error message display |

## Files Structure

```
src/ui/
├── ui_manager.h             # Main UI coordinator
├── ui_manager.cpp           # LVGL initialization, screen switching
└── screens/
    ├── screen_idle.h/cpp    # Home screen with device list
    ├── screen_listening.cpp # Voice command listening
    ├── screen_calling.cpp   # Outgoing call
    ├── screen_ringing.cpp   # Incoming call
    ├── screen_active.cpp    # Active call
    └── screen_settings.cpp  # Settings menu
```

## Display Configuration

### Hardware Specs
- **Display**: 1.8" AMOLED, 368×448 pixels
- **Orientation**: Portrait
- **Touch**: CST816S capacitive (I2C)
- **Colors**: 16-bit RGB565

### LVGL Configuration
- **Buffer**: 1/10 screen (16,486 pixels × 2 buffers)
- **Memory**: PSRAM (heap_caps_malloc)
- **Flush**: Arduino_GFX library
- **Input**: TouchLib for CST816S

## Screen Layouts

### 1. Idle Screen (Home)

**Purpose**: Main dashboard showing available devices

```
┌──────────────────────────┐ ─┐
│   Kitchen Intercom       │  │ Header (50px)
├──────────────────────────┤ ─┤
│  [Living Room]       ●   │  │
│  [Bedroom]           ●   │  │
│  [Office]            ○   │  │ Scrollable
│                          │  │ Device List
│                          │  │ (318px)
├──────────────────────────┤ ─┤
│  [⚙ Settings]            │  │
│  WiFi ████ | 12:34 PM    │  │ Footer (80px)
└──────────────────────────┘ ─┘
```

**Elements**:
- **Header**: Room name (24px font)
- **Device buttons**: 60px height, 15px spacing
- **Online indicator**: 12px green circle
- **Settings button**: Bottom left
- **Status bar**: WiFi signal, time

**Update**: `ScreenIdle::updateDeviceList()` called when devices discovered

### 2. Listening Screen

**Purpose**: Voice command mode after wake word

```
┌──────────────────────────┐
│                          │
│         🎤               │ Mic icon (48px)
│                          │
│    Listening...          │ Status (24px)
│                          │
│  Say a command:          │
│  • "Drop in on [room]"   │ Help text (14px)
│  • "Call [room]"         │
│  • "Hang up"             │
│  • "Cancel"              │
│                          │
│  [     Cancel     ]      │ Cancel button
└──────────────────────────┘
```

**Timeout**: Auto-returns to IDLE after 5 seconds (COMMAND_TIMEOUT_MS)

### 3. Calling Screen

**Purpose**: Outgoing call in progress

```
┌──────────────────────────┐
│   Calling...             │ Status (cyan)
│                          │
│      ┌────────┐          │
│      │  [🏠]  │          │ Room icon
│      └────────┘          │
│                          │
│   Living Room            │ Target room
│                          │
│   Waiting for answer...  │ Status message
│                          │
│  [     Cancel     ]      │ Cancel button
└──────────────────────────┘
```

**Dynamic**: Room name updated from CallSession

### 4. Ringing Screen

**Purpose**: Incoming call notification

```
┌──────────────────────────┐
│   Incoming Call          │ Header (yellow)
│                          │
│      ┌────────┐          │
│      │  [📞]  │          │ Call icon
│      └────────┘          │
│                          │
│   Living Room            │ Caller name
│   calling...             │
│                          │
│  [✓ Accept ]             │ Accept (green)
│                          │
│  [✗ Reject ]             │ Reject (red)
└──────────────────────────┘
```

**Actions**:
- Accept → Transitions to ACTIVE_CALL
- Reject → Returns to IDLE

### 5. Active Call Screen

**Purpose**: Call in progress with controls

```
┌──────────────────────────┐
│      ┌────────┐          │
│      │  [🏠]  │          │ Room icon
│      └────────┘          │
│   Living Room            │ Remote room
│                          │
│ Mic:  ████████           │ Mic level bar
│ Spk:  ████████           │ Speaker level bar
│                          │
│      00:45               │ Call duration
│                          │
│  [   Hang Up   ]         │ Hang up (red)
│                          │
│  [🔇] [🔊-] [🔊+]        │ Mute, Vol-, Vol+
└──────────────────────────┘
```

**Real-time Updates**:
- Duration: Every second via `ScreenActive::updateDuration()`
- Audio levels: ~10Hz via `ScreenActive::updateAudioLevels()`

### 6. Settings Screen

**Purpose**: Configuration options

```
┌──────────────────────────┐
│   Settings               │ Header
├──────────────────────────┤
│  Volume:            80%  │
│  [=========---]          │ Slider
│                          │
│  Brightness:        70%  │
│  [========----]          │ Slider
│                          │
│  Wake Word:         ON   │
│  [Toggle]                │ Switch
│                          │
│  [ ← Back ]              │ Back button
└──────────────────────────┘
```

**Persistent**: Settings saved to NVS on change

## UI Design Guidelines

### Colors (AMOLED Optimized)

Defined in `include/config.h`:

```cpp
#define UI_COLOR_BACKGROUND  0x0000  // True black (saves power)
#define UI_COLOR_PRIMARY     0x07E0  // Green
#define UI_COLOR_SECONDARY   0x7BEF  // Light gray
#define UI_COLOR_ACCENT      0xFD20  // Orange
#define UI_COLOR_TEXT        0xFFFF  // White
#define UI_COLOR_TEXT_DIM    0x8410  // Dim white
#define UI_COLOR_CALLING     0x07FF  // Cyan
#define UI_COLOR_RINGING     0xFFE0  // Yellow
#define UI_COLOR_ACTIVE      0x07E0  // Green
#define UI_COLOR_ERROR       0xF800  // Red
```

**AMOLED Power Saving**:
- Use true black (0x0000) for backgrounds → pixels OFF
- Avoid pure white large areas
- Prefer dark themes with bright accents

### Typography

```cpp
#define UI_FONT_SIZE_HEADER  24  // lv_font_montserrat_24
#define UI_FONT_SIZE_NORMAL  18  // lv_font_montserrat_18
#define UI_FONT_SIZE_SMALL   14  // lv_font_montserrat_14
```

LVGL built-in Montserrat fonts (lightweight, good on small displays).

### Layout Constants

```cpp
#define UI_MARGIN         10   // Screen margin
#define UI_BUTTON_HEIGHT  60   // Touch-friendly button height
#define UI_BUTTON_SPACING 15   // Space between buttons
#define UI_HEADER_HEIGHT  50   // Header bar
#define UI_FOOTER_HEIGHT  80   // Footer area
```

**Portrait 368×448 Considerations**:
- **Vertical space**: Plenty for scrolling lists
- **Horizontal space**: Limited, avoid multi-column layouts
- **Touch targets**: Minimum 50×50px for reliable touch
- **Text wrapping**: Enable for long room names

## Adding a New Screen

### 1. Create Screen Files

Create `src/ui/screens/screen_myscreen.cpp`:

```cpp
#include "../ui_manager.h"
#include "../../config.h"

namespace ScreenMyScreen {

static void btnExampleCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // Handle button press
}

lv_obj_t* create(void* uiManager) {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    // Create UI elements
    lv_obj_t* lblTitle = lv_label_create(screen);
    lv_label_set_text(lblTitle, "My Screen");
    lv_obj_set_style_text_font(lblTitle, &lv_font_montserrat_24, 0);
    lv_obj_align(lblTitle, LV_ALIGN_CENTER, 0, 0);

    return screen;
}

} // namespace ScreenMyScreen
```

### 2. Update UIManager

In `ui_manager.h`:
```cpp
lv_obj_t* screenMyScreen;
void createMyScreen();
```

In `ui_manager.cpp`:
```cpp
namespace ScreenMyScreen { lv_obj_t* create(void* uiManager); }

void UIManager::createMyScreen() {
    screenMyScreen = ScreenMyScreen::create(this);
}
```

In `UIManager::begin()`:
```cpp
createMyScreen();
```

### 3. Add State Mapping

In `onStateChanged()`:
```cpp
case AppState::MY_STATE:
    loadScreen(screenMyScreen);
    break;
```

## Button Callbacks

All button callbacks receive `UIManager*` as user data:

```cpp
static void myButtonCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);

    // Access components via UIManager
    if (ui && ui->callManager) {
        ui->callManager->initiateCall("Kitchen");
    }
}

// Register callback
lv_obj_add_event_cb(btn, myButtonCallback, LV_EVENT_CLICKED, uiManager);
```

## Dynamic Updates

### Device List

Called when devices discovered or status changes:

```cpp
void ScreenIdle::updateDeviceList(lv_obj_t* screen, DeviceRegistry* deviceRegistry) {
    // 1. Find device list container
    // 2. Clear existing buttons
    // 3. Create button for each online device
    // 4. Add online indicator (green circle)
}
```

### Call Information

Called every frame (~30Hz) during active call:

```cpp
void ScreenActive::updateDuration(unsigned long durationSec) {
    // Update duration label: "00:45"
}

void ScreenActive::updateAudioLevels(uint8_t micLevel, uint8_t spkLevel) {
    // Update progress bars: 0-100
}
```

### Status Bar

Called periodically to update WiFi, time, battery:

```cpp
void ScreenIdle::updateStatusBar(lv_obj_t* screen) {
    // Get WiFi RSSI
    // Get current time
    // Update status label
}
```

## LVGL Widgets Used

| Widget | Usage | Example |
|--------|-------|---------|
| `lv_label` | Text display | Room names, status |
| `lv_btn` | Buttons | Call, Accept, Reject |
| `lv_bar` | Progress bars | Audio levels |
| `lv_slider` | Sliders | Volume, brightness |
| `lv_switch` | Toggles | Wake word on/off |
| `lv_obj` | Containers | Headers, footers, panels |

## Touch Handling

**CST816S Capacitive Touch**:
- Single-touch only (no multi-touch in this implementation)
- Coordinates: 0-367 (X), 0-447 (Y)
- No calibration needed
- Interrupt-driven (GPIO 14)

**Touch Read**:
```cpp
if (touch->read()) {
    TP_Point t = touch->getPoint(0);
    data->point.x = t.x;
    data->point.y = t.y;
    data->state = LV_INDEV_STATE_PRESSED;
}
```

## Performance Optimization

### LVGL Timer

Update rate: **30Hz** (UI_UPDATE_RATE_HZ)

```cpp
void uiTask(void* parameter) {
    while (true) {
        uiManager->update();  // Calls lv_timer_handler()
        delay(1000 / UI_UPDATE_RATE_HZ);  // 33ms
    }
}
```

### Rendering

- **Partial refresh**: LVGL only redraws changed areas
- **Double buffering**: Eliminates tearing
- **PSRAM buffers**: Reduces heap fragmentation

### Memory Usage

**Typical LVGL Memory**:
- Display buffers: 2 × 16,486 × 2 = ~64KB
- LVGL internal: ~50KB
- Screen objects: ~20KB
- **Total**: ~135KB (well within 8MB PSRAM)

## Troubleshooting

### Display Not Updating

**Check**:
1. `lv_timer_handler()` called regularly
2. Display flush callback not blocking
3. Arduino_GFX initialized correctly

### Touch Not Working

**Check**:
1. I2C bus initialized (`touchBus->begin()`)
2. CST816S address correct (0x15)
3. Touch interrupt connected (GPIO 14)
4. Touch read callback registered

### Slow Performance

**Optimize**:
1. Reduce UI_UPDATE_RATE_HZ to 20Hz
2. Minimize full-screen redraws
3. Use smaller LVGL buffer (1/20 screen)
4. Simplify animations

### Text Not Visible

**Check**:
1. Text color not same as background
2. Font loaded correctly
3. Label size sufficient for text
4. Text not off-screen

## Customization Examples

### Change Color Scheme

In `config.h`:
```cpp
#define UI_COLOR_PRIMARY    0x001F  // Blue instead of green
#define UI_COLOR_BACKGROUND 0x1082  // Dark gray instead of black
```

### Add Animation

```cpp
// Fade in screen
lv_obj_set_style_opa(screen, LV_OPA_TRANSP, 0);
lv_anim_t a;
lv_anim_init(&a);
lv_anim_set_var(&a, screen);
lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
lv_anim_set_time(&a, 300);
lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_style_opa);
lv_anim_start(&a);
```

### Add Icons

Use LVGL symbols:
```cpp
LV_SYMBOL_HOME      // 🏠
LV_SYMBOL_CALL      // 📞
LV_SYMBOL_AUDIO     // 🎤
LV_SYMBOL_SETTINGS  // ⚙
LV_SYMBOL_OK        // ✓
LV_SYMBOL_CLOSE     // ✗
LV_SYMBOL_WIFI      // 📶
```

Or use custom images (convert to C array with LVGL image converter).

## Next Steps

- **Phase 7 UI Polish**: Add animations, gestures, better icons
- **Battery Status**: Add battery level indicator
- **Notifications**: Toast messages for events
- **Theming**: Support light/dark themes
- **Screensaver**: Prevent AMOLED burn-in

## Resources

- [LVGL Documentation](https://docs.lvgl.io/)
- [LVGL Examples](https://github.com/lvgl/lvgl/tree/master/examples)
- [Arduino_GFX](https://github.com/moononournation/Arduino_GFX)
- [TouchLib](https://github.com/lewisxhe/TouchLib)
