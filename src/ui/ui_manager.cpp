#include "ui_manager.h"
#include "config.h"
#include "Arduino_SH8601.h"
#ifndef DISABLE_AUDIO_TEMP
#include "../call/call_manager.h"
#include "../audio/audio_pipeline.h"
#endif
#include <Arduino_DriveBus_Library.h>

// Screen implementations
#include "screens/screen_idle.h"
#include "screens/screen_ai.h"

// Forward declarations for other screens
namespace ScreenListening { lv_obj_t* create(void* uiManager); }
namespace ScreenCalling { lv_obj_t* create(void* uiManager); }
namespace ScreenRinging { lv_obj_t* create(void* uiManager); }
namespace ScreenActive {
    lv_obj_t* create(void* uiManager);
    void updateDuration(unsigned long durationSec);
    void updateAudioLevels(uint8_t micLevel, uint8_t spkLevel);
}
namespace ScreenSettings { lv_obj_t* create(void* uiManager); }

// Global pointer for LVGL callbacks
static UIManager* g_uiManager = nullptr;

// Touch controller instance
static std::shared_ptr<Arduino_IIC_DriveBus> IIC_Bus = nullptr;
static std::unique_ptr<Arduino_IIC> FT3168 = nullptr;

UIManager::UIManager(StateMachine* sm, DeviceRegistry* dr,
#ifndef DISABLE_AUDIO_TEMP
    CallManager* cm, AudioPipeline* ap
#else
    void* cm, void* ap
#endif
)
    : stateMachine(sm),
      deviceRegistry(dr),
      callManager(cm),
      audioPipeline(ap),
      lvDisplay(nullptr),
      lvInput(nullptr),
      lvBuf1(nullptr),
      lvBuf2(nullptr),
      screenIdle(nullptr),
      screenListening(nullptr),
      screenCalling(nullptr),
      screenRinging(nullptr),
      screenActiveCall(nullptr),
      screenSettings(nullptr),
      screenAI(nullptr),
      screenError(nullptr),
      currentScreen(nullptr),
      touchBus(nullptr),
      gfx(nullptr) {
    g_uiManager = this;
}

UIManager::~UIManager() {
    if (lvBuf1) free(lvBuf1);
    if (lvBuf2) free(lvBuf2);
    // FT3168 unique_ptr cleans itself up automatically
    g_uiManager = nullptr;
}

bool UIManager::begin(Arduino_GFX* display, TwoWire* touchI2C) {
    Serial.println("[UI] Initializing UI Manager...");

    gfx = display;
    touchBus = touchI2C;

    // Initialize LVGL
    lv_init();

    // Allocate LVGL display buffers (1/10 of screen size)
    size_t bufSize = (DISPLAY_WIDTH * DISPLAY_HEIGHT / 10);
    lvBuf1 = (lv_color_t*)heap_caps_malloc(bufSize * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lvBuf2 = (lv_color_t*)heap_caps_malloc(bufSize * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!lvBuf1 || !lvBuf2) {
        Serial.println("[UI] ERROR: Failed to allocate LVGL buffers");
        return false;
    }

    lv_disp_draw_buf_init(&lvDrawBuf, lvBuf1, lvBuf2, bufSize);

    // Initialize display driver
    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res = DISPLAY_WIDTH;
    dispDrv.ver_res = DISPLAY_HEIGHT;
    dispDrv.flush_cb = displayFlushCallback;
    dispDrv.draw_buf = &lvDrawBuf;
    dispDrv.user_data = this;
    lvDisplay = lv_disp_drv_register(&dispDrv);

    // Initialize touch driver (FT3168) - Optional, display will work without it
    IIC_Bus = std::make_shared<Arduino_HWIIC>(TOUCH_SDA, TOUCH_SCL, touchBus);
    FT3168 = std::unique_ptr<Arduino_IIC>(new Arduino_FT3x68(IIC_Bus, 0x38 /* FT3168_DEVICE_ADDRESS */));
    bool touchOk = FT3168->begin();
    if (!touchOk) {
        Serial.println("[UI] WARNING: Touch init failed - display will work without touch input");
        FT3168.reset();
    } else {
        Serial.printf("[UI] ✓ Touch initialized: FT3168 (ID: 0x%X)\n", (int32_t)FT3168->IIC_Read_Device_ID());
    }

    if (touchOk) {
        static lv_indev_drv_t inputDrv;
        lv_indev_drv_init(&inputDrv);
        inputDrv.type = LV_INDEV_TYPE_POINTER;
        inputDrv.read_cb = touchReadCallback;
        inputDrv.user_data = this;
        lvInput = lv_indev_drv_register(&inputDrv);
    } else {
        Serial.println("[UI] Display will operate without touch input");
        lvInput = nullptr;
    }

    // Create all screens
    createIdleScreen();
    createListeningScreen();
    createCallingScreen();
    createRingingScreen();
    createActiveCallScreen();
    createSettingsScreen();
    createAIScreen();
    createErrorScreen();

    // Load idle screen by default
    loadScreen(screenIdle);

    Serial.println("[UI] ✓ UI Manager initialized");
    return true;
}

void UIManager::update() {
    // Track elapsed time for LVGL
    static unsigned long lastTick = 0;
    unsigned long now = millis();
    unsigned long elapsed = now - lastTick;

    if (elapsed > 0) {
        lv_tick_inc(elapsed);  // Tell LVGL how much time has passed
        lastTick = now;
    }

    // Update LVGL timer (process scheduled tasks)
    lv_timer_handler();

    // Update call info if in active call
    if (stateMachine && stateMachine->getState() == AppState::ACTIVE_CALL) {
        updateCallInfo();
    }
}

void UIManager::onStateChanged(AppState oldState, AppState newState) {
    Serial.printf("[UI] State changed: %s -> %s\n",
                  StateMachine::stateToString(oldState).c_str(),
                  StateMachine::stateToString(newState).c_str());

    // Load appropriate screen based on new state
    switch (newState) {
        case AppState::IDLE:
            updateDeviceList();  // Refresh device list
            loadScreen(screenIdle);
            break;

        case AppState::LISTENING:
            loadScreen(screenListening);
            break;

        case AppState::CALLING:
            loadScreen(screenCalling);
            break;

        case AppState::RINGING:
            loadScreen(screenRinging);
            break;

        case AppState::ACTIVE_CALL:
            loadScreen(screenActiveCall);
            break;

        case AppState::HANGING_UP:
            // Brief message, then auto-return to idle
            showError("Ending call...");
            break;

        case AppState::AI_QUERY:
            loadScreen(screenAI);
            ScreenAI::showLoading(screenAI);
            break;

        case AppState::AI_RESPONSE:
            // Screen should already be loaded, just update content
            // Response will be updated via AIManager
            break;

        case AppState::ERROR:
            showError("An error occurred");
            break;
    }
}

void UIManager::loadScreen(lv_obj_t* screen) {
    if (screen && screen != currentScreen) {
        Serial.printf("[UI] Loading screen %p (current was %p)\n", screen, currentScreen);
        lv_scr_load(screen);
        currentScreen = screen;
        Serial.println("[UI] Screen loaded, forcing refresh");
        lv_obj_invalidate(screen);  // Force redraw
    }
}

// ============================================================================
// LVGL Callbacks
// ============================================================================

void UIManager::displayFlushCallback(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    UIManager* ui = (UIManager*)disp->user_data;
    if (!ui || !ui->gfx) {
        lv_disp_flush_ready(disp);
        return;
    }

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // Debug: Log first few flushes
    static int flushCount = 0;
    if (flushCount < 3) {
        Serial.printf("[UI_FLUSH] #%d: x=%d-%d, y=%d-%d, w=%d, h=%d, pixels=%d\n",
                      flushCount, area->x1, area->x2, area->y1, area->y2, w, h, w*h);
        flushCount++;
    }

    // Draw LVGL buffer to display
    ui->gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)color_p, w, h);

    lv_disp_flush_ready(disp);
}

void UIManager::touchReadCallback(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    UIManager* ui = (UIManager*)drv->user_data;
    if (!ui || !FT3168) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint8_t fingers = FT3168->IIC_Read_Device_Value(Arduino_IIC_Touch::Value_Information::TOUCH_FINGER_NUMBER);
    if (fingers > 0) {
        int32_t x = FT3168->IIC_Read_Device_Value(Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_X);
        int32_t y = FT3168->IIC_Read_Device_Value(Arduino_IIC_Touch::Value_Information::TOUCH_COORDINATE_Y);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

lv_obj_t* UIManager::createButton(lv_obj_t* parent, const char* text, lv_event_cb_t callback,
                                   lv_color_t color, int y) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, DISPLAY_WIDTH - 2 * UI_MARGIN, UI_BUTTON_HEIGHT);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_set_style_bg_color(btn, color, 0);
    lv_obj_add_event_cb(btn, callback, LV_EVENT_CLICKED, this);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_center(label);

    return btn;
}

lv_obj_t* UIManager::createLabel(lv_obj_t* parent, const char* text, int fontSize,
                                  lv_align_t align, int x, int y) {
    lv_obj_t* label = lv_label_create(parent);
    lv_label_set_text(label, text);

    // Select font based on size
    const lv_font_t* font = &lv_font_montserrat_18;
    if (fontSize >= 24) font = &lv_font_montserrat_24;
    else if (fontSize <= 14) font = &lv_font_montserrat_14;

    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(UI_COLOR_TEXT), 0);
    lv_obj_align(label, align, x, y);

    return label;
}

void UIManager::updateStatusBar(lv_obj_t* screen) {
    // TODO: Add WiFi signal, battery, time
    // For now, this is a placeholder
}

void UIManager::setBacklight(uint8_t brightness) {
    // Control AMOLED brightness via display command (SH8601 uses command 0x51)
    if (gfx) {
        ((Arduino_SH8601*)gfx)->Display_Brightness(brightness);
    }
}

// ============================================================================
// Screen Creation Functions
// ============================================================================

void UIManager::createIdleScreen() {
    screenIdle = ScreenIdle::create(this);
}

void UIManager::createListeningScreen() {
    screenListening = ScreenListening::create(this);
}

void UIManager::createCallingScreen() {
    screenCalling = ScreenCalling::create(this);
}

void UIManager::createRingingScreen() {
    screenRinging = ScreenRinging::create(this);
}

void UIManager::createActiveCallScreen() {
    screenActiveCall = ScreenActive::create(this);
}

void UIManager::createSettingsScreen() {
    screenSettings = ScreenSettings::create(this);
}

void UIManager::createAIScreen() {
    screenAI = ScreenAI::create(this);
}

void UIManager::createErrorScreen() {
    screenError = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screenError, lv_color_hex(UI_COLOR_BACKGROUND), 0);
}

void UIManager::updateDeviceList() {
    if (screenIdle && deviceRegistry) {
        ScreenIdle::updateDeviceList(screenIdle, deviceRegistry);
        ScreenIdle::updateStatusBar(screenIdle);
    }
}

#ifndef DISABLE_AUDIO_TEMP
void UIManager::updateCallInfo() {
    if (!screenActiveCall || !callManager) return;

    CallSession* session = callManager->getCurrentSession();
    if (session && session->isActive()) {
        // Update call duration
        unsigned long durationSec = session->getDuration() / 1000;
        ScreenActive::updateDuration(durationSec);

        // Update audio levels
        uint8_t micLevel = audioPipeline ? audioPipeline->getMicrophoneLevel() : 0;
        uint8_t spkLevel = audioPipeline ? audioPipeline->getSpeakerLevel() : 0;
        ScreenActive::updateAudioLevels(micLevel, spkLevel);
    }
}
#else
void UIManager::updateCallInfo() {
    // No-op when audio is disabled
}
#endif

#ifndef DISABLE_AUDIO_TEMP
void UIManager::setVolume(uint8_t volume) {
    if (audioPipeline) {
        audioPipeline->setVolume(volume);
        Serial.printf("[UI] Volume set to %d%%\n", volume);
    }
}
#else
void UIManager::setVolume(uint8_t volume) {
    // No-op when audio is disabled
}
#endif

void UIManager::showError(const String& message) {
    if (!screenError) return;

    lv_obj_clean(screenError);
    lv_obj_set_style_bg_color(screenError, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    createLabel(screenError, message.c_str(), 18, LV_ALIGN_CENTER, 0, 0);

    loadScreen(screenError);

    // Auto-return to idle after 3 seconds
    // TODO: Use LVGL timer
}

void UIManager::showSettings() {
    if (screenSettings) {
        loadScreen(screenSettings);
    }
}

void UIManager::showIdle() {
    if (screenIdle) {
        loadScreen(screenIdle);
    }
}

// ============================================================================
// Button Callbacks (Placeholders)
// ============================================================================

void UIManager::btnCallCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    // TODO: Initiate call
}

void UIManager::btnAcceptCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
#ifndef DISABLE_AUDIO_TEMP
    if (ui && ui->callManager) {
        ui->callManager->acceptCall();
    }
#endif
}

void UIManager::btnRejectCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
#ifndef DISABLE_AUDIO_TEMP
    if (ui && ui->callManager) {
        ui->callManager->rejectCall();
    }
#endif
}

void UIManager::btnHangupCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
#ifndef DISABLE_AUDIO_TEMP
    if (ui && ui->callManager) {
        ui->callManager->hangupCall();
    }
#endif
}

void UIManager::btnCancelCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui && ui->stateMachine) {
        ui->stateMachine->setState(AppState::IDLE);
    }
}

void UIManager::btnSettingsCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui && ui->screenSettings) {
        ui->loadScreen(ui->screenSettings);
    }
}

void UIManager::btnBackCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui && ui->stateMachine) {
        ui->stateMachine->setState(AppState::IDLE);
    }
}
