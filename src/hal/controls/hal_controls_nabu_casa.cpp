/**
 * Nabu Casa Controls HAL Implementation
 *
 * Physical controls for Nabu Casa Voice PE:
 * - Action button with click/long-press/double-click
 * - Rotary encoder for volume control
 * - Hardware mute switch
 */

#include "hal/controls/hal_controls_nabu_casa.h"
#include "devices/nabu_casa/device_config.h"
#include "devices/nabu_casa/device_pins.h"
#include <Arduino.h>

// Static instance pointer for interrupt handlers
HALControlsNabuCasa* HALControlsNabuCasa::instance_ = nullptr;

HALControlsNabuCasa::HALControlsNabuCasa()
    : callback_(nullptr)
    , enabled_(true)
    , volumeLevel_(50)
    , encoderPosition_(0)
    , muteState_(false)
    , lastMuteState_(false)
    , buttonMuteEnabled_(false)
    , actionButton_(nullptr)
    , lastEncoded_(0) {

    // Set singleton instance for static callbacks
    instance_ = this;
}

HALControlsNabuCasa::~HALControlsNabuCasa() {
    if (actionButton_) {
        delete actionButton_;
        actionButton_ = nullptr;
    }

    // Detach interrupts
    detachInterrupt(digitalPinToInterrupt(PIN_ROTARY_CLK));
    detachInterrupt(digitalPinToInterrupt(PIN_ROTARY_DT));
    detachInterrupt(digitalPinToInterrupt(PIN_MUTE_SWITCH));

    instance_ = nullptr;
}

bool HALControlsNabuCasa::begin() {
    Serial.println("[HAL-Controls-NabuCasa] Initializing physical controls...");

    // Configure action button
    pinMode(PIN_BUTTON_ACTION, INPUT_PULLUP);
    actionButton_ = new OneButton(PIN_BUTTON_ACTION, true, true);  // Active LOW, enable pull-up

    // Attach OneButton callbacks
    actionButton_->attachClick(onButtonClick);
    actionButton_->attachLongPressStart(onButtonLongPress);
    actionButton_->attachDoubleClick(onButtonDoubleClick);

    // Set timing for button gestures
    actionButton_->setDebounceTicks(50);      // 50ms debounce
    actionButton_->setClickTicks(400);         // 400ms click threshold
    actionButton_->setPressTicks(1000);        // 1000ms long press threshold

    Serial.println("[HAL-Controls-NabuCasa] ✓ Action button configured");

    // Configure rotary encoder
    pinMode(PIN_ROTARY_CLK, INPUT_PULLUP);
    pinMode(PIN_ROTARY_DT, INPUT_PULLUP);

    // Read initial encoder state
    uint8_t a = digitalRead(PIN_ROTARY_CLK);
    uint8_t b = digitalRead(PIN_ROTARY_DT);
    lastEncoded_ = (a << 1) | b;

    // Attach encoder interrupts
    attachInterrupt(digitalPinToInterrupt(PIN_ROTARY_CLK), handleRotaryA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ROTARY_DT), handleRotaryB, CHANGE);

    Serial.println("[HAL-Controls-NabuCasa] ✓ Rotary encoder configured");

    // Configure mute switch
    pinMode(PIN_MUTE_SWITCH, INPUT_PULLUP);
    muteState_ = digitalRead(PIN_MUTE_SWITCH) == LOW;  // Active LOW
    lastMuteState_ = muteState_;

    // Attach mute switch interrupt
    attachInterrupt(digitalPinToInterrupt(PIN_MUTE_SWITCH), handleMuteSwitch, CHANGE);

    Serial.printf("[HAL-Controls-NabuCasa] ✓ Mute switch configured (initial: %s)\n",
                  muteState_ ? "MUTED" : "UNMUTED");

    Serial.println("[HAL-Controls-NabuCasa] ✓ Physical controls initialized");
    return true;
}

void HALControlsNabuCasa::update() {
    if (!enabled_) {
        return;
    }

    // Update OneButton (handles debouncing and gesture detection)
    if (actionButton_) {
        actionButton_->tick();
    }

    // Check for mute switch state changes
    checkMuteSwitch();
}

void HALControlsNabuCasa::registerCallback(ControlCallback callback) {
    callback_ = callback;
}

bool HALControlsNabuCasa::hasTouchScreen() const {
    return DEVICE_HAS_TOUCHSCREEN;
}

bool HALControlsNabuCasa::hasPhysicalControls() const {
    return DEVICE_HAS_PHYSICAL_BUTTONS;
}

void HALControlsNabuCasa::setEnabled(bool enabled) {
    enabled_ = enabled;
}

int HALControlsNabuCasa::getVolumeDial() const {
    return volumeLevel_;
}

bool HALControlsNabuCasa::getMuteSwitch() const {
    return muteState_;
}

// ============================================================================
// Static Button Callbacks (OneButton)
// ============================================================================

void HALControlsNabuCasa::onButtonClick() {
    if (!instance_ || !instance_->callback_ || !instance_->enabled_) {
        return;
    }

    // Toggle mute on single click
    instance_->buttonMuteEnabled_ = !instance_->buttonMuteEnabled_;

    Serial.printf("[HAL-Controls-NabuCasa] Button: Click - Mute %s\n",
                  instance_->buttonMuteEnabled_ ? "ON" : "OFF");

    // Send mute toggle event
    ControlEventData event(ControlEvent::MUTE_TOGGLE, instance_->buttonMuteEnabled_ ? 1 : 0);
    instance_->callback_(event);
}

void HALControlsNabuCasa::onButtonLongPress() {
    if (!instance_ || !instance_->callback_ || !instance_->enabled_) {
        return;
    }

    Serial.println("[HAL-Controls-NabuCasa] Button: Long Press");

    // Long press = Hang up / Cancel
    ControlEventData event(ControlEvent::HANG_UP);
    instance_->callback_(event);
}

void HALControlsNabuCasa::onButtonDoubleClick() {
    if (!instance_ || !instance_->callback_ || !instance_->enabled_) {
        return;
    }

    Serial.println("[HAL-Controls-NabuCasa] Button: Double Click");

    // Double click = Settings / AI query
    ControlEventData event(ControlEvent::AI_QUERY_START);
    instance_->callback_(event);
}

// ============================================================================
// Static Interrupt Handlers (Rotary Encoder)
// ============================================================================

void IRAM_ATTR HALControlsNabuCasa::handleRotaryA() {
    if (!instance_) return;
    instance_->updateEncoder();
}

void IRAM_ATTR HALControlsNabuCasa::handleRotaryB() {
    if (!instance_) return;
    instance_->updateEncoder();
}

void HALControlsNabuCasa::updateEncoder() {
    // Read current encoder state
    uint8_t a = digitalRead(PIN_ROTARY_CLK);
    uint8_t b = digitalRead(PIN_ROTARY_DT);
    uint8_t encoded = (a << 1) | b;

    // State machine for quadrature encoder
    // Gray code: 00 -> 01 -> 11 -> 10 -> 00 (CW)
    //            00 -> 10 -> 11 -> 01 -> 00 (CCW)

    uint8_t sum = (lastEncoded_ << 2) | encoded;

    if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
        // Clockwise rotation
        encoderPosition_++;

        // Update volume (0-100)
        if (volumeLevel_ < 100) {
            volumeLevel_++;

            // Generate volume change event (in ISR, just set flag - process in update())
            // For now, we'll call the callback directly (keep it fast!)
            if (callback_ && enabled_) {
                ControlEventData event(ControlEvent::VOLUME_SET, volumeLevel_);
                callback_(event);
            }
        }
    }
    else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
        // Counter-clockwise rotation
        encoderPosition_--;

        // Update volume (0-100)
        if (volumeLevel_ > 0) {
            volumeLevel_--;

            if (callback_ && enabled_) {
                ControlEventData event(ControlEvent::VOLUME_SET, volumeLevel_);
                callback_(event);
            }
        }
    }

    lastEncoded_ = encoded;
}

// ============================================================================
// Mute Switch Handling
// ============================================================================

void IRAM_ATTR HALControlsNabuCasa::handleMuteSwitch() {
    if (!instance_) return;

    // Read switch state (active LOW)
    instance_->muteState_ = digitalRead(PIN_MUTE_SWITCH) == LOW;
}

void HALControlsNabuCasa::checkMuteSwitch() {
    // Check if mute state changed
    if (muteState_ != lastMuteState_) {
        lastMuteState_ = muteState_;

        Serial.printf("[HAL-Controls-NabuCasa] Mute switch: %s\n",
                      muteState_ ? "MUTED" : "UNMUTED");

        // Generate mute toggle event
        if (callback_) {
            ControlEventData event(ControlEvent::MUTE_TOGGLE, muteState_ ? 1 : 0);
            callback_(event);
        }
    }
}
