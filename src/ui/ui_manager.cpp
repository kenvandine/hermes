#include "ui_manager.h"
#include "config.h"
#include "../call/call_manager.h"
#include "../audio/audio_pipeline.h"
#include "TouchDrvCSTXXX.hpp"

// Screen implementations
#include "screens/screen_idle.h"

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
static TouchDrvCSTXXX* touch = nullptr;

UIManager::UIManager(StateMachine* sm, DeviceRegistry* dr, CallManager* cm, AudioPipeline* ap)
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
      screenError(nullptr),
      currentScreen(nullptr),
      touchBus(nullptr),
      gfx(nullptr) {
    g_uiManager = this;
}

UIManager::~UIManager() {
    if (lvBuf1) free(lvBuf1);
    if (lvBuf2) free(lvBuf2);
    if (touch) delete touch;
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

    // Initialize touch driver (CST816S)
    touch = new TouchDrvCSTXXX();
    touch->setPins(TOUCH_RST, TOUCH_IRQ);
    if (!touch->begin(*touchBus, CST816_SLAVE_ADDRESS, TOUCH_SDA, TOUCH_SCL)) {
        Serial.println("[UI] WARNING: Touch init failed, retrying...");
        delay(100);
        if (!touch->begin(*touchBus, CST816_SLAVE_ADDRESS, TOUCH_SDA, TOUCH_SCL)) {
            Serial.println("[UI] ERROR: Touch init failed");
            return false;
        }
    }
    Serial.println("[UI] Touch initialized: CST816S");

    static lv_indev_drv_t inputDrv;
    lv_indev_drv_init(&inputDrv);
    inputDrv.type = LV_INDEV_TYPE_POINTER;
    inputDrv.read_cb = touchReadCallback;
    inputDrv.user_data = this;
    lvInput = lv_indev_drv_register(&inputDrv);

    // Create all screens
    createIdleScreen();
    createListeningScreen();
    createCallingScreen();
    createRingingScreen();
    createActiveCallScreen();
    createSettingsScreen();
    createErrorScreen();

    // Load idle screen by default
    loadScreen(screenIdle);

    Serial.println("[UI] ✓ UI Manager initialized");
    return true;
}

void UIManager::update() {
    // Update LVGL timer
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

        case AppState::ERROR:
            showError("An error occurred");
            break;
    }
}

void UIManager::loadScreen(lv_obj_t* screen) {
    if (screen && screen != currentScreen) {
        lv_scr_load(screen);
        currentScreen = screen;
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

    ui->gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t*)color_p, w, h);

    lv_disp_flush_ready(disp);
}

void UIManager::touchReadCallback(lv_indev_drv_t* drv, lv_indev_data_t* data) {
    UIManager* ui = (UIManager*)drv->user_data;
    if (!ui || !touch) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    int16_t x, y;
    uint8_t touched = touch->getPoint(&x, &y, 1);
    if (touched > 0) {
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
    // Control AMOLED backlight via PWM on TFT_BL pin
    analogWrite(TFT_BL, brightness);
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

void UIManager::showError(const String& message) {
    if (!screenError) return;

    lv_obj_clean(screenError);
    lv_obj_set_style_bg_color(screenError, lv_color_hex(UI_COLOR_BACKGROUND), 0);

    createLabel(screenError, message.c_str(), 18, LV_ALIGN_CENTER, 0, 0);

    loadScreen(screenError);

    // Auto-return to idle after 3 seconds
    // TODO: Use LVGL timer
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
    if (ui && ui->callManager) {
        ui->callManager->acceptCall();
    }
}

void UIManager::btnRejectCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui && ui->callManager) {
        ui->callManager->rejectCall();
    }
}

void UIManager::btnHangupCallback(lv_event_t* e) {
    UIManager* ui = (UIManager*)lv_event_get_user_data(e);
    if (ui && ui->callManager) {
        ui->callManager->hangupCall();
    }
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
