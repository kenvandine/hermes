/**
 * Waveshare Display HAL Implementation
 */

#include "hal/display/hal_display_waveshare.h"
#include "devices/waveshare/device_config.h"
#include <Arduino.h>

HALDisplayWaveshare::HALDisplayWaveshare()
    : uiManager_(nullptr) {

    // Initialize capabilities
    capabilities_.hasFullUI = true;
    capabilities_.hasLEDRing = false;
    capabilities_.hasTouchScreen = true;
    capabilities_.width = DEVICE_DISPLAY_WIDTH;
    capabilities_.height = DEVICE_DISPLAY_HEIGHT;
    capabilities_.ledCount = 0;
}

bool HALDisplayWaveshare::begin() {
    Serial.println("[HAL-Display-Waveshare] HAL foundation - full integration pending");

    // TODO: Full HAL integration requires refactoring UIManager to accept
    // dependency injection for StateMachine, DeviceRegistry, etc.
    // For now, the display is initialized directly in main.cpp

    Serial.println("[HAL-Display-Waveshare] ✓ Display HAL stub initialized");
    return true;
}

void HALDisplayWaveshare::update() {
    // TODO: Call uiManager_->update() when HAL is fully integrated
}

void HALDisplayWaveshare::setBrightness(uint8_t brightness) {
    (void)brightness;
    // TODO: Implement brightness control
}

void HALDisplayWaveshare::showState(AppState state) {
    (void)state;
    // TODO: Update display based on state
}

void HALDisplayWaveshare::showMessage(const String& message, uint32_t durationMs) {
    (void)message;
    (void)durationMs;
    // TODO: Show message on display
}

void HALDisplayWaveshare::showError(const String& error) {
    (void)error;
    // TODO: Show error on display
}

DisplayCapabilities HALDisplayWaveshare::getCapabilities() const {
    return capabilities_;
}

void HALDisplayWaveshare::updateDeviceList() {
    // TODO: Update device list on UI
}

void HALDisplayWaveshare::updateCallInfo() {
    // TODO: Update call info on UI
}
