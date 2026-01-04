#ifndef HAL_DISPLAY_WAVESHARE_H
#define HAL_DISPLAY_WAVESHARE_H

#include "hal/hal_display.h"
#include "ui/ui_manager.h"
#include <memory>

/**
 * Waveshare Display HAL Implementation
 *
 * Wraps UIManager for the Waveshare ESP32-S3 1.8" AMOLED display
 */
class HALDisplayWaveshare : public HALDisplay {
public:
    HALDisplayWaveshare();
    virtual ~HALDisplayWaveshare() = default;

    // HALDisplay interface implementation
    bool begin() override;
    void update() override;
    void setBrightness(uint8_t brightness) override;
    void showState(AppState state) override;
    void showMessage(const String& message, uint32_t durationMs = 0) override;
    void showError(const String& error) override;
    DisplayCapabilities getCapabilities() const override;
    void updateDeviceList() override;
    void updateCallInfo() override;

private:
    std::unique_ptr<UIManager> uiManager_;
    DisplayCapabilities capabilities_;
};

#endif // HAL_DISPLAY_WAVESHARE_H
