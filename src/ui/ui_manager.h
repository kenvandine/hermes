#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include "config.h"

#include "../core/state_machine.h"
#ifndef DISABLE_AUDIO_TEMP
#include "../call/call_session.h"
#endif
#include "../call/device_registry.h"

// Forward declarations
#ifndef DISABLE_AUDIO_TEMP
class CallManager;
class AudioPipeline;
#endif

/**
 * UIManager
 *
 * Manages all UI screens and transitions for the intercom system.
 * Coordinates LVGL rendering with application state.
 *
 * Screen flow:
 * - IDLE → Idle screen (device list)
 * - LISTENING → Listening screen (voice command)
 * - CALLING → Calling screen (outgoing)
 * - RINGING → Ringing screen (incoming with Accept/Reject)
 * - ACTIVE_CALL → Active call screen (with Hang Up)
 * - HANGING_UP → Brief transition message
 * - ERROR → Error message
 */
class UIManager {
public:
    UIManager(StateMachine* stateMachine,
              DeviceRegistry* deviceRegistry,
#ifndef DISABLE_AUDIO_TEMP
              CallManager* callManager,
              AudioPipeline* audioPipeline
#else
              void* callManager = nullptr,
              void* audioPipeline = nullptr
#endif
    );
    ~UIManager();

    /**
     * Initialize UI system
     * @param display Arduino_GFX display object
     * @param touchI2C Touch I2C bus
     * @return true if successful
     */
    bool begin(Arduino_GFX* display, TwoWire* touchI2C);

    /**
     * Update UI (call from UI task loop)
     * Handles LVGL updates and touch processing
     */
    void update();

    /**
     * Handle state change
     * Called by state machine callback
     */
    void onStateChanged(AppState oldState, AppState newState);

    /**
     * Update device list (when new device discovered)
     */
    void updateDeviceList();

    /**
     * Update call info (during active call)
     */
    void updateCallInfo();

    /**
     * Show error message
     */
    void showError(const String& message);

    /**
     * Set display brightness (0-255)
     */
    void setBacklight(uint8_t brightness);

    /**
     * Set audio volume (0-100)
     */
    void setVolume(uint8_t volume);

    /**
     * Get LVGL display driver
     */
    lv_disp_t* getDisplay() { return lvDisplay; }

    /**
     * Show settings screen
     */
    void showSettings();

    /**
     * Show idle/home screen
     */
    void showIdle();

    /**
     * Cancel listening mode and return to idle
     */
    void cancelListening();

private:
    StateMachine* stateMachine;
    DeviceRegistry* deviceRegistry;
#ifndef DISABLE_AUDIO_TEMP
    CallManager* callManager;
    AudioPipeline* audioPipeline;
#else
    void* callManager;
    void* audioPipeline;
#endif

    // LVGL objects
    lv_disp_t* lvDisplay;
    lv_indev_t* lvInput;
    lv_disp_draw_buf_t lvDrawBuf;
    lv_color_t* lvBuf1;
    lv_color_t* lvBuf2;

    // Screen objects
    lv_obj_t* screenIdle;
    lv_obj_t* screenListening;
    lv_obj_t* screenCalling;
    lv_obj_t* screenRinging;
    lv_obj_t* screenActiveCall;
    lv_obj_t* screenSettings;
    lv_obj_t* screenAI;
    lv_obj_t* screenError;

    // Current active screen
    lv_obj_t* currentScreen;

    // Touch driver
    TwoWire* touchBus;

    // Display driver
    Arduino_GFX* gfx;

    // Screen creation functions
    void createIdleScreen();
    void createListeningScreen();
    void createCallingScreen();
    void createRingingScreen();
    void createActiveCallScreen();
    void createSettingsScreen();
    void createAIScreen();
    void createErrorScreen();

    // Screen loading functions
    void loadScreen(lv_obj_t* screen);

    // LVGL callbacks
    static void displayFlushCallback(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
    static void touchReadCallback(lv_indev_drv_t* drv, lv_indev_data_t* data);

    // Button callbacks
    static void btnCallCallback(lv_event_t* e);
    static void btnAcceptCallback(lv_event_t* e);
    static void btnRejectCallback(lv_event_t* e);
    static void btnHangupCallback(lv_event_t* e);
    static void btnCancelCallback(lv_event_t* e);
    static void btnSettingsCallback(lv_event_t* e);
    static void btnBackCallback(lv_event_t* e);

    // Helper functions
    lv_obj_t* createButton(lv_obj_t* parent, const char* text, lv_event_cb_t callback,
                           lv_color_t color, int y);
    lv_obj_t* createLabel(lv_obj_t* parent, const char* text, int fontSize,
                         lv_align_t align, int x, int y);
    void updateStatusBar(lv_obj_t* screen);
};

#endif // UI_MANAGER_H
