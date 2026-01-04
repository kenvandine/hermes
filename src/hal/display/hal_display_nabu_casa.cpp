/**
 * Nabu Casa Display HAL Implementation (STUB)
 */

#include "hal/display/hal_display_nabu_casa.h"
#include "devices/nabu_casa/device_config.h"
#include "devices/nabu_casa/device_pins.h"
#include <Arduino.h>

HALDisplayNabuCasa::HALDisplayNabuCasa()
    : brightness_(128) {

    // Initialize capabilities for LED ring
    capabilities_.hasFullUI = false;
    capabilities_.hasLEDRing = true;
    capabilities_.hasTouchScreen = false;
    capabilities_.width = 0;
    capabilities_.height = 0;
    capabilities_.ledCount = DEVICE_LED_COUNT;
}

bool HALDisplayNabuCasa::begin() {
    Serial.println("[HAL-Display-NabuCasa] WARNING: Stub implementation");
    Serial.println("[HAL-Display-NabuCasa] TODO: Implement WS2812B LED ring support");

    // TODO: Initialize WS2812B LED ring (12 LEDs)
    // TODO: Set up LED color/animation patterns for different states
    // TODO: Consider using FastLED or Adafruit_NeoPixel library

    return false;  // Not implemented yet
}

void HALDisplayNabuCasa::update() {
    // TODO: Update LED animations
}

void HALDisplayNabuCasa::setBrightness(uint8_t brightness) {
    brightness_ = brightness;
    // TODO: Update LED ring brightness
}

void HALDisplayNabuCasa::showState(AppState state) {
    (void)state;  // Unused in stub

    // TODO: Map states to LED patterns:
    // - IDLE: Soft breathing white/blue
    // - LISTENING: Pulsing blue
    // - PROCESSING: Spinning animation
    // - CALLING: Green ring
    // - RINGING: Blinking yellow/green
    // - ERROR: Red flash
}

void HALDisplayNabuCasa::showMessage(const String& message, uint32_t durationMs) {
    // LED ring can't show text, just log it
    Serial.printf("[HAL-Display-NabuCasa] Message: %s\n", message.c_str());
    // Could map certain keywords to LED patterns
}

void HALDisplayNabuCasa::showError(const String& error) {
    Serial.printf("[HAL-Display-NabuCasa] Error: %s\n", error.c_str());
    // TODO: Show red error pattern on LED ring
}

DisplayCapabilities HALDisplayNabuCasa::getCapabilities() const {
    return capabilities_;
}

void HALDisplayNabuCasa::updateDeviceList() {
    // No UI to update on LED ring device
}

void HALDisplayNabuCasa::updateCallInfo() {
    // No UI to update on LED ring device
}
