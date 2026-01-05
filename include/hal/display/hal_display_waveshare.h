#ifndef HAL_DISPLAY_WAVESHARE_H
#define HAL_DISPLAY_WAVESHARE_H

#include "hal/hal_display.h"
#include <Arduino_GFX_Library.h>

/**
 * Waveshare Display HAL Implementation
 *
 * Handles hardware initialization for Waveshare ESP32-S3 1.8" AMOLED display
 * - TCA9554 GPIO expander (power control)
 * - QSPI display interface
 * - SH8601 AMOLED controller
 * - FT3168 touch controller
 */
class HALDisplayWaveshare : public HALDisplay {
public:
    HALDisplayWaveshare();
    virtual ~HALDisplayWaveshare();

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

    /**
     * Get initialized display driver (for UIManager)
     * @return Arduino_GFX* display driver (valid after begin() succeeds)
     */
    Arduino_GFX* getDisplayDriver() { return gfx_; }

private:
    Arduino_GFX* gfx_;
    DisplayCapabilities capabilities_;
    uint8_t brightness_;
};

#endif // HAL_DISPLAY_WAVESHARE_H
