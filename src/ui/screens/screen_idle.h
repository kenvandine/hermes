#ifndef SCREEN_IDLE_H
#define SCREEN_IDLE_H

#include <lvgl.h>
#include "../../call/device_registry.h"

/**
 * Idle/Home Screen
 *
 * Displays:
 * - Header with room name
 * - Scrollable list of available devices
 * - Device status indicators (online/offline)
 * - Settings button
 * - Status bar (WiFi, time, battery)
 *
 * Layout (368x448):
 * ┌──────────────────────┐
 * │   Kitchen Intercom   │ ← Header (50px)
 * ├──────────────────────┤
 * │  [Living Room]   ●   │ ← Device buttons
 * │  [Bedroom]       ●   │   (60px each)
 * │  [Office]        ○   │
 * │                      │
 * │                      │ ← Scrollable
 * ├──────────────────────┤
 * │  [⚙ Settings]        │ ← Footer (80px)
 * │  WiFi ████  12:34 PM │
 * └──────────────────────┘
 */
namespace ScreenIdle {
    /**
     * Create idle screen
     */
    lv_obj_t* create(void* uiManager);

    /**
     * Update device list
     */
    void updateDeviceList(lv_obj_t* screen, DeviceRegistry* deviceRegistry);

    /**
     * Update status bar (WiFi, time, battery)
     */
    void updateStatusBar(lv_obj_t* screen);
}

#endif // SCREEN_IDLE_H
