#include "../ui_manager.h"
#include "config.h"

/**
 * Settings Screen
 *
 * Configuration options.
 *
 * Layout:
 * ┌──────────────────────┐
 * │   Settings           │ ← Header
 * ├──────────────────────┤
 * │  Room Name:          │
 * │  [Kitchen      ]     │ ← Text input
 * │                      │
 * │  Volume:        80%  │
 * │  [=========---]      │ ← Slider
 * │                      │
 * │  Brightness:    70%  │
 * │  [========----]      │ ← Slider
 * │                      │
 * │  Wake Word:     ON   │
 * │  [Toggle]            │ ← Switch
 * │                      │
 * │  [  Back  ]          │ ← Back button
 * └──────────────────────┘
 */

namespace ScreenSettings {

static lv_obj_t* sliderVolume = nullptr;
static lv_obj_t* sliderBrightness = nullptr;
static lv_obj_t* lblVolumeValue = nullptr;
static lv_obj_t* lblBrightnessValue = nullptr;

static void btnBackCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui) {
        ui->showIdle();
    }
}

static void sliderVolumeCallback(lv_event_t* e) {
    int value = lv_slider_get_value(sliderVolume);
    if (lblVolumeValue) {
        char text[8];
        snprintf(text, sizeof(text), "%d%%", value);
        lv_label_set_text(lblVolumeValue, text);
    }
    // Update audio volume
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui) {
        ui->setVolume(value);
    }
}

static void sliderBrightnessCallback(lv_event_t* e) {
    int value = lv_slider_get_value(sliderBrightness);
    if (lblBrightnessValue) {
        char text[8];
        snprintf(text, sizeof(text), "%d%%", value);
        lv_label_set_text(lblBrightnessValue, text);
    }
    // Update AMOLED brightness (0-100 -> 0-255)
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui) {
        ui->setBacklight(map(value, 0, 100, 0, 255));
    }
}

lv_obj_t* create(void* uiManager) {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    // Header
    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_set_size(header, DISPLAY_WIDTH, UI_HEADER_HEIGHT);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);

    lv_obj_t* lblHeader = lv_label_create(header);
    lv_label_set_text(lblHeader, "Settings");
    lv_obj_set_style_text_font(lblHeader, &lv_font_montserrat_24, 0);
    lv_obj_center(lblHeader);

    int y = UI_HEADER_HEIGHT + 20;

    // Volume control
    lv_obj_t* lblVolume = lv_label_create(screen);
    lv_label_set_text(lblVolume, "Volume:");
    lv_obj_set_style_text_font(lblVolume, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lblVolume, UI_MARGIN, y);

    lblVolumeValue = lv_label_create(screen);
    lv_label_set_text(lblVolumeValue, "80%");
    lv_obj_set_style_text_font(lblVolumeValue, &lv_font_montserrat_18, 0);
    lv_obj_align(lblVolumeValue, LV_ALIGN_TOP_RIGHT, -UI_MARGIN, y);

    y += 30;

    sliderVolume = lv_slider_create(screen);
    lv_obj_set_size(sliderVolume, DISPLAY_WIDTH - 2 * UI_MARGIN, 10);
    lv_obj_set_pos(sliderVolume, UI_MARGIN, y);
    lv_slider_set_range(sliderVolume, 0, 100);
    lv_slider_set_value(sliderVolume, 80, LV_ANIM_OFF);
    lv_obj_add_event_cb(sliderVolume, sliderVolumeCallback, LV_EVENT_VALUE_CHANGED, nullptr);

    y += 40;

    // Brightness control
    lv_obj_t* lblBrightness = lv_label_create(screen);
    lv_label_set_text(lblBrightness, "Brightness:");
    lv_obj_set_style_text_font(lblBrightness, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lblBrightness, UI_MARGIN, y);

    lblBrightnessValue = lv_label_create(screen);
    lv_label_set_text(lblBrightnessValue, "70%");
    lv_obj_set_style_text_font(lblBrightnessValue, &lv_font_montserrat_18, 0);
    lv_obj_align(lblBrightnessValue, LV_ALIGN_TOP_RIGHT, -UI_MARGIN, y);

    y += 30;

    sliderBrightness = lv_slider_create(screen);
    lv_obj_set_size(sliderBrightness, DISPLAY_WIDTH - 2 * UI_MARGIN, 10);
    lv_obj_set_pos(sliderBrightness, UI_MARGIN, y);
    lv_slider_set_range(sliderBrightness, 10, 100);
    lv_slider_set_value(sliderBrightness, 70, LV_ANIM_OFF);
    lv_obj_add_event_cb(sliderBrightness, sliderBrightnessCallback, LV_EVENT_VALUE_CHANGED, nullptr);

    y += 40;

    // Wake word toggle
    lv_obj_t* lblWakeWord = lv_label_create(screen);
    lv_label_set_text(lblWakeWord, "Wake Word:");
    lv_obj_set_style_text_font(lblWakeWord, &lv_font_montserrat_18, 0);
    lv_obj_set_pos(lblWakeWord, UI_MARGIN, y);

    lv_obj_t* switchWakeWord = lv_switch_create(screen);
    lv_obj_align(switchWakeWord, LV_ALIGN_TOP_RIGHT, -UI_MARGIN, y - 5);
    lv_obj_add_state(switchWakeWord, LV_STATE_CHECKED);  // Default ON

    // Back button
    lv_obj_t* btnBack = lv_btn_create(screen);
    lv_obj_set_size(btnBack, DISPLAY_WIDTH - 2 * UI_MARGIN, UI_BUTTON_HEIGHT);
    lv_obj_align(btnBack, LV_ALIGN_BOTTOM_MID, 0, -UI_MARGIN);
    lv_obj_set_style_bg_color(btnBack, lv_color_hex(UI_COLOR_SECONDARY), 0);
    lv_obj_add_event_cb(btnBack, btnBackCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblBack = lv_label_create(btnBack);
    lv_label_set_text(lblBack, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(lblBack, &lv_font_montserrat_18, 0);
    lv_obj_center(lblBack);

    return screen;
}

} // namespace ScreenSettings
