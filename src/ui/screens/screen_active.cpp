#include "../ui_manager.h"
#include "config.h"

/**
 * Active Call Screen
 *
 * Shown during an active call.
 *
 * Layout:
 * ┌──────────────────────┐
 * │                      │
 * │     ┌────────┐       │
 * │     │  [🏠]  │       │ ← Room icon
 * │     └────────┘       │
 * │                      │
 * │   Living Room        │ ← Remote room
 * │                      │
 * │   ████████           │ ← Mic audio level
 * │   ████████           │ ← Speaker audio level
 * │                      │
 * │   00:45              │ ← Call duration
 * │                      │
 * │   [  Hang Up  ]      │ ← Hang up (red)
 * │                      │
 * │   [🔇] [🔊-] [🔊+]   │ ← Mute, Vol down, Vol up
 * └──────────────────────┘
 */

namespace ScreenActive {

static lv_obj_t* lblDuration = nullptr;
static lv_obj_t* barMicLevel = nullptr;
static lv_obj_t* barSpkLevel = nullptr;

static void btnHangupCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // Hang up call
}

static void btnMuteCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // Toggle mute
}

static void btnVolDownCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // Decrease volume
}

static void btnVolUpCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // Increase volume
}

lv_obj_t* create(void* uiManager) {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    // Room icon
    lv_obj_t* lblIcon = lv_label_create(screen);
    lv_label_set_text(lblIcon, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(lblIcon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lblIcon, lv_color_hex(UI_COLOR_ACTIVE), 0);
    lv_obj_align(lblIcon, LV_ALIGN_TOP_MID, 0, 40);

    // Remote room name
    lv_obj_t* lblRoom = lv_label_create(screen);
    lv_label_set_text(lblRoom, "Unknown");
    lv_obj_set_style_text_font(lblRoom, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblRoom, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_align(lblRoom, LV_ALIGN_TOP_MID, 0, 110);

    // Microphone level label
    lv_obj_t* lblMic = lv_label_create(screen);
    lv_label_set_text(lblMic, "Mic:");
    lv_obj_set_style_text_font(lblMic, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblMic, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_align(lblMic, LV_ALIGN_LEFT_MID, UI_MARGIN, -30);

    // Microphone level bar
    barMicLevel = lv_bar_create(screen);
    lv_obj_set_size(barMicLevel, DISPLAY_WIDTH - 80, 20);
    lv_obj_align(barMicLevel, LV_ALIGN_LEFT_MID, 50, -30);
    lv_obj_set_style_bg_color(barMicLevel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_color(barMicLevel, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_INDICATOR);
    lv_bar_set_range(barMicLevel, 0, 100);
    lv_bar_set_value(barMicLevel, 0, LV_ANIM_OFF);

    // Speaker level label
    lv_obj_t* lblSpk = lv_label_create(screen);
    lv_label_set_text(lblSpk, "Spk:");
    lv_obj_set_style_text_font(lblSpk, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblSpk, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_align(lblSpk, LV_ALIGN_LEFT_MID, UI_MARGIN, 0);

    // Speaker level bar
    barSpkLevel = lv_bar_create(screen);
    lv_obj_set_size(barSpkLevel, DISPLAY_WIDTH - 80, 20);
    lv_obj_align(barSpkLevel, LV_ALIGN_LEFT_MID, 50, 0);
    lv_obj_set_style_bg_color(barSpkLevel, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_color(barSpkLevel, lv_color_hex(UI_COLOR_PRIMARY), LV_PART_INDICATOR);
    lv_bar_set_range(barSpkLevel, 0, 100);
    lv_bar_set_value(barSpkLevel, 0, LV_ANIM_OFF);

    // Call duration
    lblDuration = lv_label_create(screen);
    lv_label_set_text(lblDuration, "00:00");
    lv_obj_set_style_text_font(lblDuration, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblDuration, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_align(lblDuration, LV_ALIGN_CENTER, 0, 50);

    // Hang up button
    lv_obj_t* btnHangup = lv_btn_create(screen);
    lv_obj_set_size(btnHangup, DISPLAY_WIDTH - 2 * UI_MARGIN, UI_BUTTON_HEIGHT);
    lv_obj_align(btnHangup, LV_ALIGN_BOTTOM_MID, 0, -UI_MARGIN - 50);
    lv_obj_set_style_bg_color(btnHangup, lv_color_hex(UI_COLOR_ERROR), 0);
    lv_obj_add_event_cb(btnHangup, btnHangupCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblHangup = lv_label_create(btnHangup);
    lv_label_set_text(lblHangup, LV_SYMBOL_CALL " Hang Up");
    lv_obj_set_style_text_font(lblHangup, &lv_font_montserrat_18, 0);
    lv_obj_center(lblHangup);

    // Control buttons row (Mute, Vol-, Vol+)
    int btnWidth = (DISPLAY_WIDTH - 4 * UI_MARGIN) / 3;

    // Mute button
    lv_obj_t* btnMute = lv_btn_create(screen);
    lv_obj_set_size(btnMute, btnWidth, 40);
    lv_obj_align(btnMute, LV_ALIGN_BOTTOM_LEFT, UI_MARGIN, -UI_MARGIN);
    lv_obj_set_style_bg_color(btnMute, lv_color_hex(UI_COLOR_SECONDARY), 0);
    lv_obj_add_event_cb(btnMute, btnMuteCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblMute = lv_label_create(btnMute);
    lv_label_set_text(lblMute, LV_SYMBOL_MUTE);
    lv_obj_center(lblMute);

    // Volume down button
    lv_obj_t* btnVolDown = lv_btn_create(screen);
    lv_obj_set_size(btnVolDown, btnWidth, 40);
    lv_obj_align(btnVolDown, LV_ALIGN_BOTTOM_MID, -btnWidth/2 - UI_MARGIN/2, -UI_MARGIN);
    lv_obj_set_style_bg_color(btnVolDown, lv_color_hex(UI_COLOR_SECONDARY), 0);
    lv_obj_add_event_cb(btnVolDown, btnVolDownCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblVolDown = lv_label_create(btnVolDown);
    lv_label_set_text(lblVolDown, LV_SYMBOL_VOLUME_MID "-");
    lv_obj_center(lblVolDown);

    // Volume up button
    lv_obj_t* btnVolUp = lv_btn_create(screen);
    lv_obj_set_size(btnVolUp, btnWidth, 40);
    lv_obj_align(btnVolUp, LV_ALIGN_BOTTOM_RIGHT, -UI_MARGIN, -UI_MARGIN);
    lv_obj_set_style_bg_color(btnVolUp, lv_color_hex(UI_COLOR_SECONDARY), 0);
    lv_obj_add_event_cb(btnVolUp, btnVolUpCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblVolUp = lv_label_create(btnVolUp);
    lv_label_set_text(lblVolUp, LV_SYMBOL_VOLUME_MAX "+");
    lv_obj_center(lblVolUp);

    return screen;
}

void updateDuration(unsigned long durationSec) {
    if (lblDuration) {
        char durText[16];
        snprintf(durText, sizeof(durText), "%02lu:%02lu",
                 durationSec / 60, durationSec % 60);
        lv_label_set_text(lblDuration, durText);
    }
}

void updateAudioLevels(uint8_t micLevel, uint8_t spkLevel) {
    if (barMicLevel) {
        lv_bar_set_value(barMicLevel, micLevel, LV_ANIM_OFF);
    }
    if (barSpkLevel) {
        lv_bar_set_value(barSpkLevel, spkLevel, LV_ANIM_OFF);
    }
}

} // namespace ScreenActive
