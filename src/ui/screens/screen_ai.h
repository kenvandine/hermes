#ifndef SCREEN_AI_H
#define SCREEN_AI_H

#include <Arduino.h>
#include <lvgl.h>

/**
 * AI Assistant Screen
 *
 * Displays:
 * - Header with AI icon
 * - User query text
 * - AI response or loading animation
 * - Scrollable response area
 * - Cancel and Home buttons
 *
 * Layout (368x448):
 * ┌──────────────────────┐
 * │  🤖 AI Assistant     │ ← Header (50px)
 * ├──────────────────────┤
 * │ Q: What's the        │ ← User query (60px)
 * │    weather?          │   (scrollable)
 * ├──────────────────────┤
 * │                      │ ← Response area
 * │ [Loading...]         │   (258px)
 * │   OR                 │   Scrollable
 * │ The weather today... │
 * │                      │
 * ├──────────────────────┤
 * │ [Cancel]   [Home]    │ ← Footer (80px)
 * └──────────────────────┘
 */
namespace ScreenAI {
    /**
     * Create AI screen
     */
    lv_obj_t* create(void* uiManager);

    /**
     * Show loading animation
     */
    void showLoading(lv_obj_t* screen);

    /**
     * Show response text
     */
    void showResponse(lv_obj_t* screen, const String& query, const String& response);

    /**
     * Update query text
     */
    void updateQuery(lv_obj_t* screen, const String& query);

    /**
     * Show error message
     */
    void showError(lv_obj_t* screen, const String& error);

    /**
     * Clear screen
     */
    void clear(lv_obj_t* screen);
}

#endif // SCREEN_AI_H
