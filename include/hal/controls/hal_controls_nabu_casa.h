#ifndef HAL_CONTROLS_NABU_CASA_H
#define HAL_CONTROLS_NABU_CASA_H

#include "hal/hal_controls.h"
#include <OneButton.h>

/**
 * Nabu Casa Controls HAL Implementation
 *
 * Implements physical controls for Nabu Casa Voice PE:
 * - Action button (OneButton library for debouncing/gestures)
 * - Rotary encoder (volume control with interrupts)
 * - Hardware mute switch (digital input with pullup)
 */
class HALControlsNabuCasa : public HALControls {
public:
    HALControlsNabuCasa();
    virtual ~HALControlsNabuCasa();

    // HALControls interface implementation
    bool begin() override;
    void update() override;
    void registerCallback(ControlCallback callback) override;
    bool hasTouchScreen() const override;
    bool hasPhysicalControls() const override;
    void setEnabled(bool enabled) override;
    int getVolumeDial() const override;
    bool getMuteSwitch() const override;

private:
    ControlCallback callback_;
    bool enabled_;

    // Volume tracking
    volatile int volumeLevel_;       // Current volume (0-100)
    volatile int encoderPosition_;   // Raw encoder position

    // Mute state (hardware mute switch on GPIO3)
    volatile bool muteState_;
    bool lastMuteState_;

    // Button-based mute toggle (center button on GPIO0)
    bool buttonMuteEnabled_;

    // OneButton for action button
    OneButton* actionButton_;

    // Rotary encoder state
    volatile uint8_t lastEncoded_;

    // Button callbacks (static for OneButton)
    static void onButtonClick();
    static void onButtonLongPress();
    static void onButtonDoubleClick();

    // Interrupt handlers (static for attachInterrupt)
    static void IRAM_ATTR handleRotaryA();
    static void IRAM_ATTR handleRotaryB();
    static void IRAM_ATTR handleMuteSwitch();

    // Instance pointer for static callbacks
    static HALControlsNabuCasa* instance_;

    // Helper methods
    void updateEncoder();
    void checkMuteSwitch();
};

#endif // HAL_CONTROLS_NABU_CASA_H
