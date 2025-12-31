#include "../ui_manager.h"
#include "config.h"

/**
 * Calling Screen
 *
 * Shown when outgoing call is being placed.
 *
 * Layout:
 * ┌──────────────────────┐
 * │                      │
 * │   Calling...         │ ← Status
 * │                      │
 * │     ┌────────┐       │
 * │     │  [🏠]  │       │ ← Room icon
 * │     └────────┘       │
 * │                      │
 * │   Living Room        │ ← Target room
 * │                      │
 * │   Waiting for        │ ← Status message
 * │   answer...          │
 * │                      │
 * │   [Cancel]           │ ← Cancel button
 * └──────────────────────┘
 */

namespace ScreenCalling {

static void btnCancelCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // Cancel outgoing call
}

lv_obj_t* create(void* uiManager) {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    // Status
    lv_obj_t* lblStatus = lv_label_create(screen);
    lv_label_set_text(lblStatus, "Calling...");
    lv_obj_set_style_text_font(lblStatus, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblStatus, lv_color_hex(UI_COLOR_CALLING), 0);
    lv_obj_align(lblStatus, LV_ALIGN_TOP_MID, 0, 40);

    // Room icon
    lv_obj_t* lblIcon = lv_label_create(screen);
    lv_label_set_text(lblIcon, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(lblIcon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lblIcon, lv_color_hex(UI_COLOR_CALLING), 0);
    lv_obj_align(lblIcon, LV_ALIGN_CENTER, 0, -40);

    // Target room name (will be updated dynamically)
    lv_obj_t* lblRoom = lv_label_create(screen);
    lv_label_set_text(lblRoom, "Unknown");
    lv_obj_set_style_text_font(lblRoom, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblRoom, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_align(lblRoom, LV_ALIGN_CENTER, 0, 40);

    // Waiting message
    lv_obj_t* lblWaiting = lv_label_create(screen);
    lv_label_set_text(lblWaiting, "Waiting for answer...");
    lv_obj_set_style_text_font(lblWaiting, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblWaiting, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_align(lblWaiting, LV_ALIGN_CENTER, 0, 80);

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

} // namespace ScreenCalling
