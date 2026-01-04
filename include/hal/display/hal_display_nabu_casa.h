#ifndef HAL_DISPLAY_NABU_CASA_H
#define HAL_DISPLAY_NABU_CASA_H

#include "hal/hal_display.h"

/**
 * Nabu Casa Display HAL Implementation (STUB)
 *
 * TODO: Implement for 12-LED WS2812B ring
 * - No full UI (LED ring only)
 * - Visual state indicators (idle, listening, processing, error)
 * - Animations for different states
 */
class HALDisplayNabuCasa : public HALDisplay {
public:
    HALDisplayNabuCasa();
    virtual ~HALDisplayNabuCasa() = default;

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
    DisplayCapabilities capabilities_;
    uint8_t brightness_;
};

#endif // HAL_DISPLAY_NABU_CASA_H
