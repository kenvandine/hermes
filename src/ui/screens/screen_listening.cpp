#include "../ui_manager.h"
#include "config.h"

/**
 * Listening Screen
 *
 * Shown when wake word detected and system is listening for voice command.
 *
 * Layout:
 * ┌──────────────────────┐
 * │                      │
 * │      🎤              │ ← Mic icon
 * │                      │
 * │   Listening...       │ ← Status text
 * │                      │
 * │   Say a command:     │
 * │   • "Drop in on..."  │ ← Help text
 * │   • "Call..."        │
 * │   • "Cancel"         │
 * │                      │
 * │   [Cancel]           │ ← Cancel button
 * └──────────────────────┘
 */

namespace ScreenListening {

static void btnCancelCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui) {
        Serial.println("[UI] Cancel listening pressed");
        ui->cancelListening();
    }
}

lv_obj_t* create(void* uiManager) {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    // Microphone icon (using symbol)
    lv_obj_t* lblIcon = lv_label_create(screen);
    lv_label_set_text(lblIcon, LV_SYMBOL_AUDIO);
    lv_obj_set_style_text_font(lblIcon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lblIcon, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_align(lblIcon, LV_ALIGN_CENTER, 0, -120);

    // Status text
    lv_obj_t* lblStatus = lv_label_create(screen);
    lv_label_set_text(lblStatus, "Listening...");
    lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblStatus, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_align(lblStatus, LV_ALIGN_CENTER, 0, -50);

    // Help text
    lv_obj_t* lblHelp = lv_label_create(screen);
    lv_label_set_text(lblHelp,
        "Say a command:\n\n"
        "• \"Drop in on [room]\"\n"
        "• \"Call [room]\"\n"
        "• \"Hang up\"\n"
        "• \"Cancel\"");
    lv_obj_set_style_text_font(lblHelp, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblHelp, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_set_style_text_align(lblHelp, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_align(lblHelp, LV_ALIGN_CENTER, 0, 50);

    // Cancel button
    lv_obj_t* btnCancel = lv_btn_create(screen);
    lv_obj_set_size(btnCancel, DISPLAY_WIDTH - 2 * UI_MARGIN, UI_BUTTON_HEIGHT);
    lv_obj_align(btnCancel, LV_ALIGN_BOTTOM_MID, 0, -UI_MARGIN);
    lv_obj_set_style_bg_color(btnCancel, lv_color_hex(UI_COLOR_ERROR), 0);
    lv_obj_add_event_cb(btnCancel, btnCancelCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblCancel = lv_label_create(btnCancel);
    lv_label_set_text(lblCancel, "Cancel");
    lv_obj_set_style_text_font(lblCancel, &lv_font_montserrat_18, 0);
    lv_obj_center(lblCancel);

    return screen;
}

} // namespace ScreenListening
