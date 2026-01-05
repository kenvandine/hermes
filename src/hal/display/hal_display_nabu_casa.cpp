/**
 * Nabu Casa Display HAL Implementation
 *
 * Uses FastLED to drive 12-LED WS2812B ring for visual state feedback
 */

#ifdef DEVICE_NABU_CASA

#include "hal/display/hal_display_nabu_casa.h"
#include "devices/nabu_casa/device_config.h"
#include "devices/nabu_casa/device_pins.h"
#include "core/state_machine.h"
#include <Arduino.h>

HALDisplayNabuCasa::HALDisplayNabuCasa()
    : brightness_(128)
    , currentState_(AppState::IDLE)
    , animationPhase_(0)
    , lastUpdateMs_(0)
    , showingVolume_(false)
    , displayVolume_(50)
    , displayMuted_(false)
    , volumeDisplayEndMs_(0) {

    // Initialize capabilities for LED ring
    capabilities_.hasFullUI = false;
    capabilities_.hasLEDRing = true;
    capabilities_.hasTouchScreen = false;
    capabilities_.width = 0;
    capabilities_.height = 0;
    capabilities_.ledCount = DEVICE_LED_COUNT;
}

bool HALDisplayNabuCasa::begin() {
    Serial.println("[HAL-Display-NabuCasa] Initializing LED ring...");
    Serial.printf("[HAL-Display-NabuCasa] LED count: %d, Pin: %d\n", DEVICE_LED_COUNT, PIN_LED_RING_DATA);

    // Initialize FastLED for WS2812B LED ring
    FastLED.addLeds<WS2812B, PIN_LED_RING_DATA, GRB>(leds_, DEVICE_LED_COUNT);
    FastLED.setBrightness(brightness_);
    FastLED.clear();
    FastLED.show();

    Serial.println("[HAL-Display-NabuCasa] ✓ LED ring initialized");

    // Show startup animation
    for (int i = 0; i < DEVICE_LED_COUNT; i++) {
        leds_[i] = CRGB::Blue;
        FastLED.show();
        delay(50);
    }
    delay(200);
    FastLED.clear();
    FastLED.show();

    Serial.println("[HAL-Display-NabuCasa] ✓ LED startup animation complete");
    return true;
}

void HALDisplayNabuCasa::update() {
    updateAnimation();
}

void HALDisplayNabuCasa::setBrightness(uint8_t brightness) {
    brightness_ = brightness;
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void HALDisplayNabuCasa::showState(AppState state) {
    if (state != currentState_) {
        Serial.printf("[HAL-Display-NabuCasa] State change: %d -> %d\n",
                      static_cast<int>(currentState_), static_cast<int>(state));
        currentState_ = state;
        animationPhase_ = 0;
        lastUpdateMs_ = millis();
    }
}

void HALDisplayNabuCasa::showMessage(const String& message, uint32_t durationMs) {
    // LED ring can't show text, just log it
    Serial.printf("[HAL-Display-NabuCasa] Message: %s\n", message.c_str());
    (void)durationMs;
}

void HALDisplayNabuCasa::showError(const String& error) {
    Serial.printf("[HAL-Display-NabuCasa] Error: %s\n", error.c_str());

    // Flash red for errors
    setAllLEDs(CRGB::Red);
    FastLED.show();
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

void HALDisplayNabuCasa::showVolume(uint8_t volume, bool muted) {
    showingVolume_ = true;
    displayVolume_ = volume;
    displayMuted_ = muted;
    volumeDisplayEndMs_ = millis() + 2000;  // Show for 2 seconds

    Serial.printf("[HAL-Display-NabuCasa] Volume display: %d%% %s\n",
                  volume, muted ? "(MUTED)" : "");
}

// ============================================================================
// Animation Helpers
// ============================================================================

void HALDisplayNabuCasa::updateAnimation() {
    uint32_t now = millis();
    uint32_t elapsed = now - lastUpdateMs_;

    // Update animation at ~30 FPS
    if (elapsed < 33) {
        return;
    }

    lastUpdateMs_ = now;
    animationPhase_++;

    // Check if volume display timeout has expired
    if (showingVolume_ && now >= volumeDisplayEndMs_) {
        showingVolume_ = false;
        Serial.println("[HAL-Display-NabuCasa] Volume display timeout - returning to state display");
    }

    // Show volume display if active
    if (showingVolume_) {
        // Clear all LEDs first
        setAllLEDs(CRGB::Black);

        // Calculate how many LEDs to light (0-12 based on 0-100%)
        int ledsToLight = (displayVolume_ * DEVICE_LED_COUNT + 50) / 100;  // Round to nearest
        if (ledsToLight > DEVICE_LED_COUNT) ledsToLight = DEVICE_LED_COUNT;

        // Choose color based on mute state
        CRGB volumeColor;
        if (displayMuted_) {
            volumeColor = CRGB::Red;  // Red when muted
        } else if (displayVolume_ > 80) {
            volumeColor = CRGB::Orange;  // Orange for high volume
        } else {
            volumeColor = CRGB::Cyan;  // Cyan for normal volume
        }

        // Light up the appropriate number of LEDs
        for (int i = 0; i < ledsToLight; i++) {
            leds_[i] = volumeColor;
        }

        FastLED.show();
        return;  // Skip state animation while showing volume
    }

    // Normal state animation
    switch (currentState_) {
        case AppState::IDLE:
            // Soft breathing blue
            {
                uint8_t brightness = beatsin8(30, 50, 255, 0, animationPhase_);
                CRGB color = CRGB(0, 0, brightness);
                setAllLEDs(color);
            }
            break;

        case AppState::LISTENING:
            // Solid green (listening/active)
            setAllLEDs(CRGB::Green);
            break;

        case AppState::AI_QUERY:
        case AppState::AI_RESPONSE:
            // Pulsing green for AI processing
            {
                uint8_t brightness = beatsin8(60, 128, 255, 0, animationPhase_);
                CRGB color = CRGB(0, brightness, 0);
                setAllLEDs(color);
            }
            break;

        case AppState::CALLING:
        case AppState::HANGING_UP:
            // Solid green
            setAllLEDs(CRGB::Green);
            break;

        case AppState::ACTIVE_CALL:
            // Solid green (in call)
            setAllLEDs(CRGB::Green);
            break;

        case AppState::RINGING:
            // Blinking yellow/green
            {
                CRGB color = (animationPhase_ % 20 < 10) ? CRGB::Yellow : CRGB::Green;
                setAllLEDs(color);
            }
            break;

        case AppState::ERROR:
            // Red flash
            {
                CRGB color = (animationPhase_ % 10 < 5) ? CRGB::Red : CRGB::Black;
                setAllLEDs(color);
            }
            break;

        default:
            // Unknown state - dim white
            setAllLEDs(CRGB(64, 64, 64));
            break;
    }

    FastLED.show();
}

void HALDisplayNabuCasa::setAllLEDs(CRGB color) {
    for (int i = 0; i < DEVICE_LED_COUNT; i++) {
        leds_[i] = color;
    }
}

void HALDisplayNabuCasa::setLED(uint8_t index, CRGB color) {
    if (index < DEVICE_LED_COUNT) {
        leds_[index] = color;
    }
}

CRGB HALDisplayNabuCasa::getStateColor(AppState state) {
    switch (state) {
        case AppState::IDLE:        return CRGB::Blue;
        case AppState::LISTENING:   return CRGB::Cyan;
        case AppState::AI_QUERY:    return CRGB::Purple;
        case AppState::AI_RESPONSE: return CRGB::Purple;
        case AppState::CALLING:     return CRGB::Green;
        case AppState::ACTIVE_CALL: return CRGB::Green;
        case AppState::HANGING_UP:  return CRGB::Orange;
        case AppState::RINGING:     return CRGB::Yellow;
        case AppState::ERROR:       return CRGB::Red;
        default:                    return CRGB::White;
    }
}

#endif // DEVICE_NABU_CASA
