#ifndef HAL_CONTROLS_NABU_CASA_H
#define HAL_CONTROLS_NABU_CASA_H

#include "hal/hal_controls.h"

/**
 * Nabu Casa Controls HAL Implementation (STUB)
 *
 * TODO: Implement for physical controls:
 * - Action button (with debouncing, press/long-press detection)
 * - Rotary encoder (for volume control)
 * - Hardware mute switch
 */
class HALControlsNabuCasa : public HALControls {
public:
    HALControlsNabuCasa();
    virtual ~HALControlsNabuCasa() = default;

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
    int volumeLevel_;
    bool muteState_;

    // Debouncing state
    unsigned long lastButtonPress_;
    unsigned long lastRotaryChange_;
};

#endif // HAL_CONTROLS_NABU_CASA_H
