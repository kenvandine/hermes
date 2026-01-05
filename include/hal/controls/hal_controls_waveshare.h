#ifndef HAL_CONTROLS_WAVESHARE_H
#define HAL_CONTROLS_WAVESHARE_H

#include "hal/hal_controls.h"
#include "ui/ui_manager.h"
#include <memory>

/**
 * Waveshare Controls HAL Implementation
 *
 * Handles touch screen input via UIManager integration
 * (Touch events are processed by LVGL/UIManager)
 */
class HALControlsWaveshare : public HALControls {
public:
    HALControlsWaveshare();
    virtual ~HALControlsWaveshare() = default;

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
};

#endif // HAL_CONTROLS_WAVESHARE_H
