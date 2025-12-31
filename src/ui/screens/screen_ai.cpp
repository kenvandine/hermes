#include "screen_ai.h"
#include "config.h"
#include "../ui_manager.h"

/**
 * AI Assistant Screen Implementation
 */

namespace ScreenAI {

// Static references to UI elements (for updates)
static lv_obj_t* lblQuery = nullptr;
static lv_obj_t* txtResponse = nullptr;
static lv_obj_t* spinner = nullptr;
static lv_obj_t* lblError = nullptr;

// Button callbacks
static void btnCancelCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui) {
        // Cancel AI query, return to idle
        // TODO: Call AIManager->cancel()
    }
}

static void btnHomeCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui) {
        // Return to idle
        // ui->getStateMachine()->setState(AppState::IDLE);
    }
}

lv_obj_t* create(void* uiManager) {
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    // Header
    lv_obj_t* lblHeader = lv_label_create(screen);
    lv_label_set_text(lblHeader, LV_SYMBOL_SETTINGS " AI Assistant");  // Robot icon
    lv_obj_set_style_text_font(lblHeader, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lblHeader, lv_color_hex(UI_COLOR_PRIMARY), 0);
    lv_obj_align(lblHeader, LV_ALIGN_TOP_MID, 0, UI_MARGIN);

    // Query label (scrollable container)
    lv_obj_t* queryContainer = lv_obj_create(screen);
    lv_obj_set_size(queryContainer, DISPLAY_WIDTH - 2 * UI_MARGIN, 70);
    lv_obj_align(queryContainer, LV_ALIGN_TOP_MID, 0, UI_HEADER_HEIGHT);
    lv_obj_set_style_bg_color(queryContainer, lv_color_hex(0x1a1a1a), 0);
    lv_obj_set_style_border_width(queryContainer, 1, 0);
    lv_obj_set_style_border_color(queryContainer, lv_color_hex(UI_COLOR_SECONDARY), 0);
    lv_obj_set_scrollbar_mode(queryContainer, LV_SCROLLBAR_MODE_AUTO);

    lblQuery = lv_label_create(queryContainer);
    lv_label_set_text(lblQuery, "Q: ");
    lv_obj_set_style_text_font(lblQuery, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblQuery, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_label_set_long_mode(lblQuery, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lblQuery, DISPLAY_WIDTH - 4 * UI_MARGIN);
    lv_obj_align(lblQuery, LV_ALIGN_TOP_LEFT, UI_MARGIN / 2, UI_MARGIN / 2);

    // Response area (scrollable text area)
    int responseY = UI_HEADER_HEIGHT + 70 + UI_MARGIN;
    int responseHeight = DISPLAY_HEIGHT - responseY - UI_FOOTER_HEIGHT - UI_MARGIN;

    lv_obj_t* responseContainer = lv_obj_create(screen);
    lv_obj_set_size(responseContainer, DISPLAY_WIDTH - 2 * UI_MARGIN, responseHeight);
    lv_obj_align(responseContainer, LV_ALIGN_TOP_MID, 0, responseY);
    lv_obj_set_style_bg_color(responseContainer, lv_color_hex(0x0a0a0a), 0);
    lv_obj_set_style_border_width(responseContainer, 1, 0);
    lv_obj_set_style_border_color(responseContainer, lv_color_hex(UI_COLOR_SECONDARY), 0);
    lv_obj_set_scrollbar_mode(responseContainer, LV_SCROLLBAR_MODE_AUTO);

    // Response text
    txtResponse = lv_label_create(responseContainer);
    lv_label_set_text(txtResponse, "");
    lv_obj_set_style_text_font(txtResponse, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(txtResponse, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_label_set_long_mode(txtResponse, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(txtResponse, DISPLAY_WIDTH - 4 * UI_MARGIN);
    lv_obj_align(txtResponse, LV_ALIGN_TOP_LEFT, UI_MARGIN / 2, UI_MARGIN / 2);

    // Spinner (hidden by default)
    spinner = lv_spinner_create(responseContainer, 1000, 60);
    lv_obj_set_size(spinner, 60, 60);
    lv_obj_center(spinner);
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);

    // Error label (hidden by default)
    lblError = lv_label_create(responseContainer);
    lv_label_set_text(lblError, "");
    lv_obj_set_style_text_font(lblError, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lblError, lv_color_hex(UI_COLOR_ERROR), 0);
    lv_label_set_long_mode(lblError, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lblError, DISPLAY_WIDTH - 4 * UI_MARGIN);
    lv_obj_center(lblError);
    lv_obj_add_flag(lblError, LV_OBJ_FLAG_HIDDEN);

    // Footer buttons
    int footerY = DISPLAY_HEIGHT - UI_FOOTER_HEIGHT + UI_MARGIN;

    // Cancel button (left)
    lv_obj_t* btnCancel = lv_btn_create(screen);
    lv_obj_set_size(btnCancel, (DISPLAY_WIDTH - 3 * UI_MARGIN) / 2, UI_BUTTON_HEIGHT);
    lv_obj_set_pos(btnCancel, UI_MARGIN, footerY);
    lv_obj_set_style_bg_color(btnCancel, lv_color_hex(UI_COLOR_ERROR), 0);
    lv_obj_add_event_cb(btnCancel, btnCancelCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblCancel = lv_label_create(btnCancel);
    lv_label_set_text(lblCancel, "Cancel");
    lv_obj_set_style_text_font(lblCancel, &lv_font_montserrat_18, 0);
    lv_obj_center(lblCancel);

    // Home button (right)
    lv_obj_t* btnHome = lv_btn_create(screen);
    lv_obj_set_size(btnHome, (DISPLAY_WIDTH - 3 * UI_MARGIN) / 2, UI_BUTTON_HEIGHT);
    lv_obj_set_pos(btnHome, DISPLAY_WIDTH / 2 + UI_MARGIN / 2, footerY);
    lv_obj_set_style_bg_color(btnHome, lv_color_hex(UI_COLOR_SECONDARY), 0);
    lv_obj_add_event_cb(btnHome, btnHomeCallback, LV_EVENT_CLICKED, uiManager);

    lv_obj_t* lblHome = lv_label_create(btnHome);
    lv_label_set_text(lblHome, LV_SYMBOL_HOME " Home");
    lv_obj_set_style_text_font(lblHome, &lv_font_montserrat_18, 0);
    lv_obj_center(lblHome);

    return screen;
}

void showLoading(lv_obj_t* screen) {
    if (!spinner || !txtResponse || !lblError) return;

    // Hide response text and error
    lv_obj_add_flag(txtResponse, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lblError, LV_OBJ_FLAG_HIDDEN);

    // Show spinner
    lv_obj_clear_flag(spinner, LV_OBJ_FLAG_HIDDEN);
}

void showResponse(lv_obj_t* screen, const String& query, const String& response) {
    if (!lblQuery || !txtResponse || !spinner || !lblError) return;

    // Update query
    String queryText = "Q: " + query;
    lv_label_set_text(lblQuery, queryText.c_str());

    // Update response
    lv_label_set_text(txtResponse, response.c_str());

    // Hide spinner and error
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lblError, LV_OBJ_FLAG_HIDDEN);

    // Show response text
    lv_obj_clear_flag(txtResponse, LV_OBJ_FLAG_HIDDEN);
}

void updateQuery(lv_obj_t* screen, const String& query) {
    if (!lblQuery) return;

    String queryText = "Q: " + query;
    lv_label_set_text(lblQuery, queryText.c_str());
}

void showError(lv_obj_t* screen, const String& error) {
    if (!lblError || !spinner || !txtResponse) return;

    // Update error text
    lv_label_set_text(lblError, error.c_str());

    // Hide spinner and response
    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(txtResponse, LV_OBJ_FLAG_HIDDEN);

    // Show error
    lv_obj_clear_flag(lblError, LV_OBJ_FLAG_HIDDEN);
}

void clear(lv_obj_t* screen) {
    if (!lblQuery || !txtResponse || !spinner || !lblError) return;

    lv_label_set_text(lblQuery, "Q: ");
    lv_label_set_text(txtResponse, "");
    lv_label_set_text(lblError, "");

    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(lblError, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(txtResponse, LV_OBJ_FLAG_HIDDEN);
}

} // namespace ScreenAI
