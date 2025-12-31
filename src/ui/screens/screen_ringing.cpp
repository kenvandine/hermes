#include "../ui_manager.h"
#include "config.h"

/**
 * Ringing Screen
 *
 * Shown when incoming call is received.
 *
 * Layout:
 * ┌──────────────────────┐
 * │                      │
 * │   Incoming Call      │ ← Header
 * │                      │
 * │     ┌────────┐       │
 * │     │  [🏠]  │       │ ← Caller icon
 * │     └────────┘       │
 * │                      │
 * │   Living Room        │ ← Caller name
 * │   calling...         │
 * │                      │
 * │   [  Accept  ]       │ ← Accept (green)
 * │                      │
 * │   [  Reject  ]       │ ← Reject (red)
 * └──────────────────────┘
 */

namespace ScreenRinging {

static void btnAcceptCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // Accept incoming call
}

static void btnRejectCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // Reject incoming call
}

lv_obj_t* create(void* uiManager) {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    // Header
    lv_obj_t* lblHeader = lv_label_create(screen);
    lv_label_set_text(lblHeader, "Incoming Call");
    lv_obj_set_style_text_font(lblHeader, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblHeader, lv_color_hex(UI_COLOR_RINGING), 0);
    lv_obj_align(lblHeader, LV_ALIGN_TOP_MID, 0, 40);

    // Caller icon (pulsing animation possible)
    lv_obj_t* lblIcon = lv_label_create(screen);
    lv_label_set_text(lblIcon, LV_SYMBOL_CALL);
    lv_obj_set_style_text_font(lblIcon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lblIcon, lv_color_hex(UI_COLOR_RINGING), 0);
    lv_obj_align(lblIcon, LV_ALIGN_CENTER, 0, -60);

    // Caller room name (will be updated)
    lv_obj_t* lblCaller = lv_label_create(screen);
    lv_label_set_text(lblCaller, "Unknown");
    lv_obj_set_style_text_font(lblCaller, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblCaller, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_align(lblCaller, LV_ALIGN_CENTER, 0, 20);

    // Calling message
    lv_obj_t* lblMessage = lv_label_create(screen);
    lv_label_set_text(lblMessage, "calling...");
    lv_obj_set_style_text_font(lblMessage, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblMessage, lv_color_hex(UI_COLOR_TEXT_DIM), 0);
    lv_obj_align(lblMessage, LV_ALIGN_CENTER, 0, 50);

    // Accept button
    lv_obj_t* btnAccept = lv_btn_create(screen);
    lv_obj_set_size(btnAccept, DISPLAY_WIDTH - 2 * UI_MARGIN, UI_BUTTON_HEIGHT);
    lv_obj_align(btnAccept, LV_ALIGN_BOTTOM_MID, 0, -UI_MARGIN - UI_BUTTON_HEIGHT - UI_BUTTON_SPACING);
    lv_obj_set_style_bg_color(btnAccept, lv_color_hex(UI_COLOR_ACTIVE), 0);
    lv_obj_add_event_cb(btnAccept, btnAcceptCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblAccept = lv_label_create(btnAccept);
    lv_label_set_text(lblAccept, LV_SYMBOL_OK " Accept");
    lv_obj_set_style_text_font(lblAccept, &lv_font_montserrat_18, 0);
    lv_obj_center(lblAccept);

    // Reject button
    lv_obj_t* btnReject = lv_btn_create(screen);
    lv_obj_set_size(btnReject, DISPLAY_WIDTH - 2 * UI_MARGIN, UI_BUTTON_HEIGHT);
    lv_obj_align(btnReject, LV_ALIGN_BOTTOM_MID, 0, -UI_MARGIN);
    lv_obj_set_style_bg_color(btnReject, lv_color_hex(UI_COLOR_ERROR), 0);
    lv_obj_add_event_cb(btnReject, btnRejectCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblReject = lv_label_create(btnReject);
    lv_label_set_text(lblReject, LV_SYMBOL_CLOSE " Reject");
    lv_obj_set_style_text_font(lblReject, &lv_font_montserrat_18, 0);
    lv_obj_center(lblReject);

    return screen;
}

} // namespace ScreenRinging
