#include "screen_idle.h"
#include "config.h"
#include "../ui_manager.h"
#include <WiFi.h>

// UI element tags for updates
#define TAG_DEVICE_LIST      1
#define TAG_STATUS_BAR       2
#define TAG_ROOM_NAME        3

// Button callback for device selection
static void deviceButtonCallback(lv_event_t* e) {
    const char* roomName = (const char*)lv_event_get_user_data(e);
    UIManager* ui = (UIManager*)lv_obj_get_user_data(lv_scr_act());

    if (ui && roomName) {
        // TODO: Initiate call to selected room
        Serial.printf("[UI] Device selected: %s\n", roomName);
    }
}

// Settings button callback
static void settingsButtonCallback(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    Serial.printf("[UI] Settings button callback invoked! Event code: %d\n", code);
    
    if (code == LV_EVENT_CLICKED || code == LV_EVENT_PRESSED) {
        UIManager* ui = (UIManager*)lv_event_get_user_data(e);
        Serial.println("[UI] Settings button pressed - attempting to show settings");
        if (ui) {
            Serial.println("[UI] UIManager pointer valid, calling showSettings()");
            ui->showSettings();
        } else {
            Serial.println("[UI] ERROR: UIManager pointer is NULL!");
        }
    }
}

lv_obj_t* ScreenIdle::create(void* uiManager) {
    Serial.printf("[UI] ScreenIdle::create called with uiManager=%p\n", uiManager);
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), 0);
    lv_obj_set_user_data(screen, uiManager);

    // ========================================
    // Header Section
    // ========================================
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, DISPLAY_WIDTH, UI_HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);

    // Room name label
    lv_obj_t* lblRoomName = lv_label_create(header);
    lv_label_set_text(lblRoomName, "HERMES");  // Will be updated with room name
    lv_obj_set_style_text_font(lblRoomName, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblRoomName, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_center(lblRoomName);
    lv_obj_set_user_data(lblRoomName, (void*)TAG_ROOM_NAME);

    // ========================================
    // Device List Section (Scrollable)
    // ========================================
    lv_obj_t* deviceListContainer = lv_obj_create(screen);
    lv_obj_set_size(deviceListContainer, DISPLAY_WIDTH,
                    DISPLAY_HEIGHT - UI_HEADER_HEIGHT - UI_FOOTER_HEIGHT);
    lv_obj_set_pos(deviceListContainer, 0, UI_HEADER_HEIGHT);
    lv_obj_set_style_bg_color(deviceListContainer, lv_color_hex(UI_COLOR_BACKGROUND), 0);
    lv_obj_set_style_border_width(deviceListContainer, 0, 0);
    lv_obj_set_style_pad_all(deviceListContainer, UI_MARGIN, 0);
    lv_obj_set_scroll_dir(deviceListContainer, LV_DIR_VER);
    lv_obj_set_user_data(deviceListContainer, (void*)TAG_DEVICE_LIST);

    // Placeholder text (will be replaced by device buttons)
    lv_obj_t* lblPlaceholder = lv_label_create(deviceListContainer);
    lv_label_set_text(lblPlaceholder, "Discovering devices...\n\nMake sure other HERMES\ndevices are powered on\nand connected to WiFi.");
    lv_obj_set_style_text_font(lblPlaceholder, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblPlaceholder, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_set_style_text_align(lblPlaceholder, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lblPlaceholder);

    // ========================================
    // Footer Section
    // ========================================
    lv_obj_t* footer = lv_obj_create(screen);
    lv_obj_set_size(footer, DISPLAY_WIDTH, UI_FOOTER_HEIGHT);
    lv_obj_set_pos(footer, 0, DISPLAY_HEIGHT - UI_FOOTER_HEIGHT);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0x1A1A1A), 0);  // Dark gray
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, UI_MARGIN, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling on footer
    lv_obj_move_foreground(footer);  // Ensure footer is on top

    // Settings button
    Serial.printf("[UI] Creating Settings button with uiManager=%p\n", uiManager);
    lv_obj_t* btnSettings = lv_btn_create(footer);
    lv_obj_set_size(btnSettings, 120, 50);
    lv_obj_align(btnSettings, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(btnSettings, lv_color_hex(UI_COLOR_SECONDARY), 0);
    lv_obj_clear_flag(btnSettings, LV_OBJ_FLAG_SCROLLABLE);  // Ensure button doesn't scroll
    lv_obj_add_flag(btnSettings, LV_OBJ_FLAG_CLICKABLE);  // Explicitly make clickable
    lv_obj_move_foreground(btnSettings);  // Bring to front
    
    Serial.println("[UI] Adding event callbacks to Settings button");
    lv_obj_add_event_cb(btnSettings, settingsButtonCallback, LV_EVENT_CLICKED, uiManager);
    lv_obj_add_event_cb(btnSettings, settingsButtonCallback, LV_EVENT_PRESSED, uiManager);  // Also try PRESSED
    Serial.printf("[UI] Settings button created at %p\n", btnSettings);

    lv_obj_t* lblSettings = lv_label_create(btnSettings);
    lv_label_set_text(lblSettings, LV_SYMBOL_SETTINGS " Settings");
    lv_obj_set_style_text_font(lblSettings, &lv_font_montserrat_14, 0);
    lv_obj_center(lblSettings);

    // Status bar (WiFi, time, battery)
    lv_obj_t* lblStatus = lv_label_create(footer);
    lv_label_set_text(lblStatus, "WiFi " LV_SYMBOL_WIFI " | 12:34");
    lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblStatus, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_align(lblStatus, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_user_data(lblStatus, (void*)TAG_STATUS_BAR);

    Serial.printf("[UI] ScreenIdle::create complete - returning screen=%p\n", screen);
    return screen;
}

void ScreenIdle::updateDeviceList(lv_obj_t* screen, DeviceRegistry* deviceRegistry) {
    if (!screen || !deviceRegistry) return;

    // Find device list container
    lv_obj_t* deviceListContainer = nullptr;
    uint32_t childCount = lv_obj_get_child_cnt(screen);

    for (uint32_t i = 0; i < childCount; i++) {
        lv_obj_t* child = lv_obj_get_child(screen, i);
        if (lv_obj_get_user_data(child) == (void*)TAG_DEVICE_LIST) {
            deviceListContainer = child;
            break;
        }
    }

    if (!deviceListContainer) return;

    // Clear existing device buttons
    lv_obj_clean(deviceListContainer);

    // Get online devices
    auto devices = deviceRegistry->getOnlineDevices();

    if (devices.empty()) {
        // Show placeholder
        lv_obj_t* lblPlaceholder = lv_label_create(deviceListContainer);
        lv_label_set_text(lblPlaceholder, "No devices found\n\nMake sure other HERMES\ndevices are powered on.");
        lv_obj_set_style_text_font(lblPlaceholder, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(lblPlaceholder, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
        lv_obj_set_style_text_align(lblPlaceholder, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(lblPlaceholder);
        return;
    }

    // Create button for each device
    int y = UI_MARGIN;
    for (auto device : devices) {
        // Device button
        lv_obj_t* btn = lv_btn_create(deviceListContainer);
        lv_obj_set_size(btn, DISPLAY_WIDTH - 3 * UI_MARGIN, UI_BUTTON_HEIGHT);
        lv_obj_set_pos(btn, 0, y);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x2A2A2A), 0);  // Dark gray

        // Copy room name to persistent memory for callback
        char* roomNameCopy = (char*)malloc(device->roomName.length() + 1);
        strcpy(roomNameCopy, device->roomName.c_str());
        lv_obj_add_event_cb(btn, deviceButtonCallback, LV_EVENT_CLICKED, roomNameCopy);

        // Room name label
        lv_obj_t* lblRoom = lv_label_create(btn);
        lv_label_set_text(lblRoom, device->roomName.c_str());
        lv_obj_set_style_text_font(lblRoom, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(lblRoom, lv_color_hex(UI_COLOR_TEXT), 0);
        lv_obj_align(lblRoom, LV_ALIGN_LEFT_MID, UI_MARGIN, 0);

        // Online indicator (green dot)
        lv_obj_t* indicator = lv_obj_create(btn);
        lv_obj_set_size(indicator, 12, 12);
        lv_obj_set_style_radius(indicator, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(indicator, lv_color_hex(UI_COLOR_ACTIVE), 0);
        lv_obj_set_style_border_width(indicator, 0, 0);
        lv_obj_align(indicator, LV_ALIGN_RIGHT_MID, -UI_MARGIN, 0);

        y += UI_BUTTON_HEIGHT + UI_BUTTON_SPACING;
    }
}

void ScreenIdle::updateStatusBar(lv_obj_t* screen) {
    if (!screen) return;

    // Find status bar label
    uint32_t childCount = lv_obj_get_child_cnt(screen);

    for (uint32_t i = 0; i < childCount; i++) {
        lv_obj_t* child = lv_obj_get_child(screen, i);
        // Recursively search in footer
        uint32_t subChildCount = lv_obj_get_child_cnt(child);
        for (uint32_t j = 0; j < subChildCount; j++) {
            lv_obj_t* subChild = lv_obj_get_child(child, j);
            if (lv_obj_get_user_data(subChild) == (void*)TAG_STATUS_BAR) {
                // Update status text
                char statusText[64];

                // Get WiFi RSSI
                int rssi = WiFi.RSSI();
                const char* wifiIcon = (rssi > -60) ? LV_SYMBOL_WIFI : LV_SYMBOL_WARNING;

                // Get current time
                struct tm timeinfo;
                if (getLocalTime(&timeinfo)) {
                    snprintf(statusText, sizeof(statusText),
                             "%s %s | %02d:%02d",
                             "WiFi", wifiIcon, timeinfo.tm_hour, timeinfo.tm_min);
                } else {
                    snprintf(statusText, sizeof(statusText),
                             "%s %s", "WiFi", wifiIcon);
                }

                lv_label_set_text(subChild, statusText);
                return;
            }
        }
    }
}
